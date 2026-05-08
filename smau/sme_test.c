/**
 * sme_test.c — ARM SME Test Stimulus: 8×8 Float Matrix Multiply-Add
 *
 *   C = A × B + C    (three 8×8 single-precision matrices)
 *
 * Uses ARM SME FMOPA (outer product) via inline assembly.
 * ZA tile: 8×8 × float32 — requires svcntw = 8 (SVE VL = 256 bits).
 *
 * 算法概述 (Algorithm):
 *   1. 将 C 加载到 ZA 阵列 (8行 → 8个 tile slice)
 *   2. K 循环 (k = 0..7):
 *      a. Z0 = A[:,k] — SVE gather load (row-major → 列向量)
 *      b. Z1 = B[k,:] — SVE 连续加载 (row-major → 行向量)
 *      c. FMOPA → ZA += outer_product(Z0, Z1)
 *   3. 将 ZA 存回 C
 *
 * Build for bare-metal (QEMU virt):
 *   aarch64-none-elf-gcc -O2 -march=armv9-a+sme -ffreestanding -nostdlib \
 *       -DBAREMETAL -Tlinker.ld -o sme_test.elf sme_test.c
 *   aarch64-none-elf-objcopy -O binary sme_test.elf sme_test.bin
 *   qemu-system-aarch64 -M virt -cpu max -nographic -semihosting \
 *       -kernel sme_test.bin
 *
 * Build for Linux (AArch64, requires HW SME support):
 *   gcc -O2 -march=armv9-a+sme -o sme_test sme_test.c
 *   ./sme_test
 */

#include <stdint.h>

#define N 8

/* 8×8 single-precision matrix */
typedef float mat_t[N][N];

/*============================================================================
 * Bare-metal startup — minimal AArch64 entry point (QEMU virt)
 *============================================================================*/
#ifdef BAREMETAL
__attribute__((naked, section(".text._start")))
void _start(void) {
    __asm__ volatile (
        "ldr x0, =__stack_top\n\t"
        "mov sp, x0\n\t"
        "bl main\n\t"          /* never returns — exit_() loops */
    );
}

/* ARM Semihosting — putchar via SYS_WRITEC */
static void putc_(char c) {
    __asm__ volatile (
        "mov x1, %[p]\n\t"
        "mov x0, #3\n\t"       /* SYS_WRITEC */
        "hlt #0xF000\n\t"
        :
        : [p] "r" (&c)
        : "x0", "x1", "memory"
    );
}

/* ARM Semihosting — puts via SYS_WRITE0 */
static void puts_(const char *s) {
    __asm__ volatile (
        "mov x1, %[p]\n\t"
        "mov x0, #4\n\t"       /* SYS_WRITE0 */
        "hlt #0xF000\n\t"
        :
        : [p] "r" (s)
        : "x0", "x1"
    );
}

/* ARM Semihosting — exit via SYS_REPORTEXCEPTION */
static void exit_(int code) {
    uint64_t block[2] = { 0x20026, code };
    __asm__ volatile (
        "mov x1, %[p]\n\t"
        "mov x0, #0x18\n\t"    /* SYS_REPORTEXCEPTION */
        "hlt #0xF000\n\t"
        :
        : [p] "r" (block)
        : "x0", "x1", "memory"
    );
    while (1) { __asm__ volatile ("wfi"); }
}

#else /* Linux userspace */

#include <stdio.h>
#include <stdlib.h>

static void putc_(char c) { putchar(c); }
static void puts_(const char *s) { fputs(s, stdout); }
static void exit_(int code) { exit(code); }

#endif

/*============================================================================
 * Minimal formatted output (no stdlib dependency)
 *============================================================================*/
static void print_int(int v) {
    char buf[16];
    int i = 0;
    if (v < 0) { putc_('-'); v = -v; }
    if (v == 0) { putc_('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i) putc_(buf[--i]);
}

static void print_float(float v) {
    if (v < 0) { putc_('-'); v = -v; }
    /* Check for inf/nan via bit pattern */
    union { float f; uint32_t u; } fu = { .f = v };
    uint32_t exp  = (fu.u >> 23) & 0xFF;
    uint32_t mant = fu.u & 0x7FFFFF;
    if (exp == 0xFF) {
        puts_(mant ? "nan" : "inf");
        return;
    }
    /* Integer part */
    int int_part = (int)v;
    if (int_part == 0) {
        putc_('0');
    } else {
        char tmp[16];
        int j = 0;
        while (int_part) { tmp[j++] = '0' + (int_part % 10); int_part /= 10; }
        while (j) putc_(tmp[--j]);
    }
    /* Fractional part (4 digits) */
    float frac = v - (int)v;
    if (frac < 0) frac = -frac;
    if (frac > 1e-6f) {
        putc_('.');
        for (int d = 0; d < 4; d++) {
            frac *= 10.0f;
            int digit = (int)frac;
            putc_('0' + digit);
            frac -= digit;
        }
    }
}

static void print_matrix(const char *name, const mat_t m) {
    puts_(name); puts_(":\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            print_float(m[i][j]);
            putc_(' ');
        }
        putc_('\n');
    }
    putc_('\n');
}

/*============================================================================
 * Reference C implementation (for verification)
 *
 * C[i][j] += sum_k A[i][k] * B[k][j]
 *============================================================================*/
static void mat_mul_add_ref(const mat_t A, const mat_t B, mat_t C) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];
}

/*============================================================================
 * SME FMOPA implementation (inline assembly)
 *
 * ┌──────────────────────────────────────────────────────────────────┐
 * │  ZA tile layout (ZA.S tile 0):                                   │
 * │    svcntw × svcntw = 8×8 单精度浮点                              │
 * │    ZA.S[0..7] — 每个 slice 对应 tile 的一行                      │
 * │                                                                  │
 * │  寄存器映射 (register map):                                      │
 * │    p0 — VL8 全真谓词 (8 个元素全部激活)                          │
 * │    z0 — A 列向量 / C 行暂存                                       │
 * │    z1 — B 行向量                                                  │
 * │    z2 — gather 偏移向量 [0,8,16,...,56]                           │
 * │    x3 — 循环计数器 k                                              │
 * │    x4 — A gather 基地址 (A + k*4)                                 │
 * │    x6 — B 行基地址 (B + k*32)                                     │
 * │    x7 — C 基地址                                                  │
 * └──────────────────────────────────────────────────────────────────┘
 *
 *  关键 SME 指令:
 *    SMSTART/SMSTOP         — 进入/退出 streaming SVE 模式
 *    ZERO {ZA}              — 清零 ZA 阵列
 *    MOV ZA.S[n], Pg/M, Zn  — 矢量 → ZA tile slice
 *    MOV Zn.S, Pg/M, ZA.S[n] — ZA tile slice → 矢量
 *    FMOPA ZA.S[0], ...     — 外积累加到 ZA tile 0
 *    INDEX Zd.T, #imm, #imm — 生成等差序列矢量
 *============================================================================*/
static void mat_mul_add_sme(const mat_t A, const mat_t B, mat_t C) {
    __asm__ volatile (
        /* ═══ 进入 streaming SVE 模式 ═══ */
        "smstart\n\t"

        /* VL8 谓词: 激活前 8 个 32-bit 元素 */
        "ptrue  p0.s, vl8\n\t"

        /* 清零整个 ZA 阵列 */
        "zero   {za}\n\t"

        /* ═══ 将 C 加载到 ZA tile (8行 → 8 slices) ═══ */
        "mov    x7, %[C]\n\t"
        "ld1w   {z0.s}, p0/z, [x7, #0, mul vl]\n\t"
        "mov    za.s[0], p0/m, z0.s\n\t"
        "ld1w   {z0.s}, p0/z, [x7, #1, mul vl]\n\t"
        "mov    za.s[1], p0/m, z0.s\n\t"
        "ld1w   {z0.s}, p0/z, [x7, #2, mul vl]\n\t"
        "mov    za.s[2], p0/m, z0.s\n\t"
        "ld1w   {z0.s}, p0/z, [x7, #3, mul vl]\n\t"
        "mov    za.s[3], p0/m, z0.s\n\t"
        "ld1w   {z0.s}, p0/z, [x7, #4, mul vl]\n\t"
        "mov    za.s[4], p0/m, z0.s\n\t"
        "ld1w   {z0.s}, p0/z, [x7, #5, mul vl]\n\t"
        "mov    za.s[5], p0/m, z0.s\n\t"
        "ld1w   {z0.s}, p0/z, [x7, #6, mul vl]\n\t"
        "mov    za.s[6], p0/m, z0.s\n\t"
        "ld1w   {z0.s}, p0/z, [x7, #7, mul vl]\n\t"
        "mov    za.s[7], p0/m, z0.s\n\t"

        /* ═══ 生成 gather 偏移矢量 ═══ */
        /* z2 = [0, 8, 16, 24, 32, 40, 48, 56]
         * 每个元素 = 行号 × N (行间距)
         * 搭配 UXTW #2 使用: byte_offset = Z2[i] × 4 = i × 32
         * 对 A[:,k] 做 gather: A[i][k] = *(A + k*4 + i*32) */
        "index  z2.s, #0, #8\n\t"

        /* ═══ 外积 K 循环 (k = 0..7) ═══ */
        "mov    x3, #0\n\t"
        ".Lk_loop:\n\t"

        /* 加载 A[:,k] — SVE gather (row-major → 列向量) */
        "add    x4, %[A], x3, lsl #2\n\t"    /* x4 = &A[0][k] */
        "ld1w   {z0.s}, p0/z, [x4, z2.s, uxtw #2]\n\t"

        /* 加载 B[k,:] — SVE 连续加载 */
        "add    x6, %[B], x3, lsl #5\n\t"    /* x6 = &B[k][0] */
        "ld1w   {z1.s}, p0/z, [x6]\n\t"

        /* ZA tile 0 += outer_product(Z0, Z1)
         *   ZA[i][j] += Z0[i] × Z1[j]   for all i,j */
        "fmopa  za.s[0], p0/m, z0.s, z1.s\n\t"

        "add    x3, x3, #1\n\t"
        "cmp    x3, #8\n\t"
        "b.ne   .Lk_loop\n\t"

        /* ═══ 将 ZA 存回 C ═══ */
        "mov    x7, %[C]\n\t"
        "mov    z0.s, p0/m, za.s[0]\n\t"
        "st1w   {z0.s}, p0, [x7, #0, mul vl]\n\t"
        "mov    z0.s, p0/m, za.s[1]\n\t"
        "st1w   {z0.s}, p0, [x7, #1, mul vl]\n\t"
        "mov    z0.s, p0/m, za.s[2]\n\t"
        "st1w   {z0.s}, p0, [x7, #2, mul vl]\n\t"
        "mov    z0.s, p0/m, za.s[3]\n\t"
        "st1w   {z0.s}, p0, [x7, #3, mul vl]\n\t"
        "mov    z0.s, p0/m, za.s[4]\n\t"
        "st1w   {z0.s}, p0, [x7, #4, mul vl]\n\t"
        "mov    z0.s, p0/m, za.s[5]\n\t"
        "st1w   {z0.s}, p0, [x7, #5, mul vl]\n\t"
        "mov    z0.s, p0/m, za.s[6]\n\t"
        "st1w   {z0.s}, p0, [x7, #6, mul vl]\n\t"
        "mov    z0.s, p0/m, za.s[7]\n\t"
        "st1w   {z0.s}, p0, [x7, #7, mul vl]\n\t"

        /* ═══ 退出 streaming SVE 模式 ═══ */
        "smstop\n\t"

        : /* no C output operands */
        : [A] "r" (A), [B] "r" (B), [C] "r" (C)
        : "x3", "x4", "x6", "x7",
          "p0", "z0", "z1", "z2", "za", "memory"
    );
}

/*============================================================================
 * Verification — compare with reference (tolerance = 1e-3)
 *============================================================================*/
static int compare_matrices(const mat_t ref, const mat_t result, float eps) {
    int errors = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float diff = ref[i][j] - result[i][j];
            if (diff < 0) diff = -diff;
            if (diff > eps) errors++;
        }
    }
    return errors;
}

/*============================================================================
 * Test data initialization
 *
 *   A[i][j] = i*N + j + 1   (1 .. 64)
 *   B[i][j] = j*N + i + 1   (A 的转置)
 *   C[i][j] = 1.0
 *============================================================================*/
static void init_data(mat_t A, mat_t B, mat_t C_ref, mat_t C_sme) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j]       = (float)(i * N + j + 1);
            B[i][j]       = (float)(j * N + i + 1);
            C_ref[i][j]   = 1.0f;
            C_sme[i][j]   = 1.0f;
        }
    }
}

/*============================================================================
 * Main
 *============================================================================*/
int main(void) {
    mat_t A, B, C_ref, C_sme;

    init_data(A, B, C_ref, C_sme);

    puts_("===== ARM SME Matrix Multiply-Add Test =====\n");
    puts_("  C = A x B + C    (8x8 float, FMOPA outer product)\n");
    puts_("  Requires svcntw = 8 (SVE VL >= 256 bits)\n\n");

    print_matrix("A (input)", A);
    print_matrix("B (input)", B);
    print_matrix("C (initial)", C_ref);

    /* Reference implementation (scalar C) */
    mat_mul_add_ref(A, B, C_ref);

    /* SME FMOPA implementation (inline assembly) */
    mat_mul_add_sme(A, B, C_sme);

    print_matrix("C_ref  (C implementation)",  C_ref);
    print_matrix("C_sme  (SME FMOPA inline)",  C_sme);

    /* 验证结果 */
    int err = compare_matrices(C_ref, C_sme, 1e-3f);
    if (err == 0) {
        puts_("PASS: SME FMOPA result matches reference.\n");
    } else {
        puts_("FAIL: SME result differs from reference!  Total element errors: ");
        print_int(err);
        putc_('\n');
    }

    puts_("===== Done =====\n");
    exit_(0);
    return 0;
}
