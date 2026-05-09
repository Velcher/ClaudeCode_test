/**
 * sme_double_test.c — ARM SME 8×8 双精度矩阵乘法 (C = A^T × B)
 *
 * 特点:
 *   - 双精度 (float64), 无 gather (C = A^T × B, A 的列 = A 的行, 连续加载)
 *   - VL=256 → svcntd=4 → ZA.d 为 4×4, 需 4 趟分别计算 8×8 的四个象限
 *   - 纯内联汇编, 每条指令有中文注释
 *
 * Linux:
 *   gcc -O2 -march=armv9-a+sme -o sme_double sme_double_test.c && ./sme_double
 *
 * QEMU bare-metal:
 *   aarch64-none-elf-gcc -O2 -march=armv9-a+sme -ffreestanding -nostdlib \
 *       -DBAREMETAL -Tlinker.ld -o sme_double.elf sme_double_test.c
 *   qemu-system-aarch64 -M virt -cpu max -nographic -semihosting \
 *       -kernel sme_double.elf
 */

#include <stdint.h>

#define N 8

typedef double mat_t[N][N];

/*============================================================================
 * 算法推导
 *============================================================================
 *
 * C = A^T × B, 其中 A^T[i][k] = A[k][i]
 *
 * C[i][j] = Σ(k=0..7) A[k][i] × B[k][j]
 *
 * SME 外积法: C = Σ(k) A_row_k ⊗ B_row_k
 *   A_row_k = A[k][0..7]  (A 的第 k 行, 连续存储, LD1D 直接加载)
 *   B_row_k = B[k][0..7]  (B 的第 k 行, 连续存储, LD1D 直接加载)
 *
 * 双精度约束: VL=256 → svcntd=4
 *   ZA.d 为 4×4, 一个 FMOPA tile 覆盖 4×4 doubles
 *   8×8 需拆成 4 个象限 (每个 4×4), 分 4 趟:
 *
 *   ┌──────┬──────┐
 *   │ 趟 0 │ 趟 1 │  ← i=0..3
 *   ├──────┼──────┤
 *   │ 趟 2 │ 趟 3 │  ← i=4..7
 *   └──────┴──────┘
 *     j=0..3  j=4..7
 *
 * 每趟:
 *   1. zero {za}
 *   2. K 循环 ×8: 加载 A[k][qi*4..qi*4+3], B[k][qj*4..qj*4+3], fmopa
 *   3. ZA 4 行 → C 对应象限
 *
 * 寄存器映射:
 *   p0   — VL4 全真谓词 (4 doubles)
 *   z0   — A 第 k 行的一半 (4 doubles)
 *   z1   — B 第 k 行的一半 (4 doubles)
 *   z0.d — 提示汇编器按 double 解释 z0
 *   x3   — K 循环计数器 (0→7)
 *   x4   — &A[k][qi*4]
 *   x5   — &B[k][qj*4]
 *   x7   — C 写回指针
 *
 * 注: 无 INDEX / UXTW / z2, 因 A 的行在内存中连续, 无需 gather。
 */

__attribute__((noinline))
static void sme_matmul_double(mat_t C, const mat_t A, const mat_t B) {
    __asm__ volatile (

        /*===== 0. 进入 SME streaming 模式 =====*/
        "smstart\n\t"

        /*===== 1. 生成全真谓词 p0, VL4 = 4 × f64 =====*/
        "ptrue  p0.d, vl4\n\t"

        /*==========================================================
         * 趟 0 — 计算 C[0:3][0:3]
         *   z0 = A[k][0..3] (A 行前半)
         *   z1 = B[k][0..3] (B 行前半)
         *==========================================================*/
        "zero   {za}\n\t"
        "mov    x3, #0\n\t"
        ".Lq00_loop:\n\t"

        /* 加载 A[k][0..3] */
        "add    x4, %[A], x3, lsl #6\n\t"       // x4 = A + k*64
        "ld1d   {z0.d}, p0/z, [x4, #0, mul vl]\n\t"  // z0 = A[k][0..3]

        /* 加载 B[k][0..3] */
        "add    x5, %[B], x3, lsl #6\n\t"       // x5 = B + k*64
        "ld1d   {z1.d}, p0/z, [x5, #0, mul vl]\n\t"  // z1 = B[k][0..3]

        /* 外积累加: ZA[i][j] += A[k][i] × B[k][j]  (i,j=0..3) */
        "fmopa  za.d[0], p0/m, p0/m, z0.d, z1.d\n\t"

        "add    x3, x3, #1\n\t"
        "cmp    x3, #8\n\t"
        "b.ne   .Lq00_loop\n\t"

        /* 写回象限 0 → C[0:3][0:3] */
        "mov    x7, %[C]\n\t"
        "mov    z0.d, p0/m, za.d[0]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"   // C[0][0..3]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[1]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"   // C[1][0..3]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[2]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"   // C[2][0..3]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[3]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"   // C[3][0..3]

        /*==========================================================
         * 趟 1 — 计算 C[0:3][4:7]
         *   z0 = A[k][0..3] (A 行前半)
         *   z1 = B[k][4..7] (B 行后半)
         *==========================================================*/
        "zero   {za}\n\t"
        "mov    x3, #0\n\t"
        ".Lq01_loop:\n\t"

        "add    x4, %[A], x3, lsl #6\n\t"
        "ld1d   {z0.d}, p0/z, [x4, #0, mul vl]\n\t"  // A[k][0..3]
        "add    x5, %[B], x3, lsl #6\n\t"
        "ld1d   {z1.d}, p0/z, [x5, #1, mul vl]\n\t"  // B[k][4..7]
        "fmopa  za.d[0], p0/m, p0/m, z0.d, z1.d\n\t"

        "add    x3, x3, #1\n\t"
        "cmp    x3, #8\n\t"
        "b.ne   .Lq01_loop\n\t"

        /* 写回象限 1 → C[0:3][4:7] */
        "add    x7, %[C], #32\n\t"                       // C + 32B (跳过前 4 doubles)
        "mov    z0.d, p0/m, za.d[0]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[0][4..7]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[1]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[1][4..7]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[2]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[2][4..7]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[3]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[3][4..7]

        /*==========================================================
         * 趟 2 — 计算 C[4:7][0:3]
         *   z0 = A[k][4..7] (A 行后半)
         *   z1 = B[k][0..3] (B 行前半)
         *==========================================================*/
        "zero   {za}\n\t"
        "mov    x3, #0\n\t"
        ".Lq10_loop:\n\t"

        "add    x4, %[A], x3, lsl #6\n\t"
        "ld1d   {z0.d}, p0/z, [x4, #1, mul vl]\n\t"  // A[k][4..7]
        "add    x5, %[B], x3, lsl #6\n\t"
        "ld1d   {z1.d}, p0/z, [x5, #0, mul vl]\n\t"  // B[k][0..3]
        "fmopa  za.d[0], p0/m, p0/m, z0.d, z1.d\n\t"

        "add    x3, x3, #1\n\t"
        "cmp    x3, #8\n\t"
        "b.ne   .Lq10_loop\n\t"

        /* 写回象限 2 → C[4:7][0:3] */
        "add    x7, %[C], #256\n\t"                      // C + 256B (跳过前 4 行)
        "mov    z0.d, p0/m, za.d[0]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[4][0..3]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[1]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[5][0..3]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[2]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[6][0..3]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[3]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[7][0..3]

        /*==========================================================
         * 趟 3 — 计算 C[4:7][4:7]
         *   z0 = A[k][4..7] (A 行后半)
         *   z1 = B[k][4..7] (B 行后半)
         *==========================================================*/
        "zero   {za}\n\t"
        "mov    x3, #0\n\t"
        ".Lq11_loop:\n\t"

        "add    x4, %[A], x3, lsl #6\n\t"
        "ld1d   {z0.d}, p0/z, [x4, #1, mul vl]\n\t"  // A[k][4..7]
        "add    x5, %[B], x3, lsl #6\n\t"
        "ld1d   {z1.d}, p0/z, [x5, #1, mul vl]\n\t"  // B[k][4..7]
        "fmopa  za.d[0], p0/m, p0/m, z0.d, z1.d\n\t"

        "add    x3, x3, #1\n\t"
        "cmp    x3, #8\n\t"
        "b.ne   .Lq11_loop\n\t"

        /* 写回象限 3 → C[4:7][4:7] */
        "add    x7, %[C], #288\n\t"                      // C + 256 + 32
        "mov    z0.d, p0/m, za.d[0]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[4][4..7]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[1]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[5][4..7]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[2]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[6][4..7]
        "add    x7, x7, #64\n\t"
        "mov    z0.d, p0/m, za.d[3]\n\t"
        "st1d   {z0.d}, p0, [x7, #0, mul vl]\n\t"       // C[7][4..7]

        /*===== 退出 SME streaming 模式 =====*/
        "smstop\n\t"

        : /* 无输出操作数 (通过 [C] 指针间接写回) */
        : [A] "r" (A), [B] "r" (B), [C] "r" (C)
        : "x3", "x4", "x5", "x7",
          "p0", "z0", "z1", "za", "memory"
    );
}

/*============================================================================
 * 纯 C 参考实现 (用于验证)
 *============================================================================*/
static void matmul_at_b_ref(mat_t C, const mat_t A, const mat_t B) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            C[i][j] = 0.0;
            for (int k = 0; k < N; k++)
                C[i][j] += A[k][i] * B[k][j];   // A^T[i][k] = A[k][i]
        }
}

/*============================================================================
 * I/O & 验证
 *============================================================================*/
#ifdef BAREMETAL

static void putc_(char c) {
    __asm__ volatile (
        "mov x1, %[p]\n\t"
        "mov x0, #3\n\t"
        "hlt #0xF000\n\t"
        : : [p] "r" (&c) : "x0", "x1", "memory"
    );
}
static void puts_(const char *s) {
    __asm__ volatile (
        "mov x1, %[p]\n\t"
        "mov x0, #4\n\t"
        "hlt #0xF000\n\t"
        : : [p] "r" (s) : "x0", "x1"
    );
}
static void exit_(int code) {
    uint64_t block[2] = { 0x20026, code };
    __asm__ volatile (
        "mov x1, %[p]\n\t"
        "mov x0, #0x18\n\t"
        "hlt #0xF000\n\t"
        : : [p] "r" (block) : "x0", "x1", "memory"
    );
    while (1) { __asm__ volatile ("wfi"); }
}

__attribute__((naked, section(".text._start")))
void _start(void) {
    __asm__ volatile ("ldr x0, =__stack_top\n\tmov sp, x0\n\tbl main\n\t");
}

#else
#include <stdio.h>
#include <stdlib.h>
static void putc_(char c) { putchar(c); }
static void puts_(const char *s) { fputs(s, stdout); }
static void exit_(int code) { exit(code); }
#endif

static void print_double(double v) {
    if (v < 0) { putc_('-'); v = -v; }
    union { double f; uint64_t u; } fu = { .f = v };
    uint64_t exp  = (fu.u >> 52) & 0x7FF;
    uint64_t mant = fu.u & 0xFFFFFFFFFFFFFULL;
    if (exp == 0x7FF) { puts_(mant ? "nan" : "inf"); return; }
    long long int_part = (long long)v;
    if (int_part == 0) { putc_('0'); }
    else {
        char tmp[32]; int j = 0; long long n = int_part;
        if (n < 0) n = -n;
        while (n) { tmp[j++] = '0' + (n % 10); n /= 10; }
        while (j) putc_(tmp[--j]);
    }
    double frac = v - (double)int_part;
    if (frac < 0) frac = -frac;
    if (frac > 1e-12) {
        putc_('.');
        for (int d = 0; d < 6; d++) {
            frac *= 10.0;
            int digit = (int)frac;
            putc_('0' + digit);
            frac -= (double)digit;
        }
    }
}

static void print_matrix(const char *name, const mat_t m) {
    puts_(name); puts_(":\n");
    for (int i = 0; i < N; i++) {
        putc_('[');
        for (int j = 0; j < N; j++) {
            putc_(' '); print_double(m[i][j]);
        }
        puts_(" ]\n");
    }
    putc_('\n');
}

static int compare(const mat_t ref, const mat_t sme, double eps) {
    int errors = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            double d = ref[i][j] - sme[i][j];
            if (d < 0) d = -d;
            if (d > eps) errors++;
        }
    return errors;
}

int main(void) {
    mat_t A, B, C_ref, C_sme;

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            A[i][j] = (double)(i * N + j + 1);            /* 1.0 ~ 64.0 */
            B[i][j] = (double)((i * N + j + 1) * 0.5);    /* 0.5 ~ 32.0 */
        }

    puts_("===== ARM SME 8x8 Double-Precision Matrix Multiply =====\n");
    puts_("  C = A^T x B  (no gather, 4-pass quadrant method)\n");
    puts_("  VL=256, svcntd=4, ZA.d = 4x4\n\n");

    print_matrix("A (input)", A);
    print_matrix("B (input)", B);

    /* 参考实现 */
    matmul_at_b_ref(C_ref, A, B);
    print_matrix("C_ref  (C reference)", C_ref);

    /* SME 实现 */
    sme_matmul_double(C_sme, A, B);
    print_matrix("C_sme  (SME assembly)", C_sme);

    int err = compare(C_ref, C_sme, 1e-12);
    if (err == 0)
        puts_("PASS: SME double-precision result matches reference!\n");
    else {
        puts_("FAIL: element errors = ");
        print_double((double)err);
        putc_('\n');
    }

    puts_("===== Done =====\n");
    exit_(0);
    return 0;
}
