/**
 * sme_matmul_at.c — ARM SME: C = A^T × B  (A先转置再乘)
 *
 * 与 sme_matmul_tutorial.c 的关键区别:
 *   - 原版: C = A × B, 需 gather 取 A 的列 (非连续, 需要 INDEX + UXTW)
 *   - 本版: C = A^T × B, A^T 的列 = A 的行 (连续存储, 直接 LD1W)
 *
 * 省去: INDEX 指令、UXTW #2 寻址、z2 寄存器
 * 结果: 1 个寄存器, 2 条指令, 更简洁的外积循环
 *
 * Linux:
 *   gcc -O2 -march=armv9-a+sme -o sme_at sme_matmul_at.c && ./sme_at
 */

#include <stdint.h>

#define N 8

typedef float mat_t[N][N];

/*============================================================================
 * 算法推导
 *============================================================================
 *
 * C = A^T × B, 其中 A^T[i][k] = A[k][i]
 *
 * C[i][j] = Σ(k=0..7) A^T[i][k] × B[k][j]
 *         = Σ(k=0..7) A[k][i] × B[k][j]
 *
 * SME 外积法:
 *   C = Σ(k=0..7) A_row_k ⊗ B_row_k
 *   其中 A_row_k = A 的第 k 行: [A[k][0], A[k][1], ..., A[k][7]]
 *       B_row_k = B 的第 k 行: [B[k][0], B[k][1], ..., B[k][7]]
 *
 * 第 k 次迭代:
 *   z0 = A[k][0..7]    (A 第 k 行, 连续加载)
 *   z1 = B[k][0..7]    (B 第 k 行, 连续加载)
 *   FMOPA: ZA[r][s] += z0[r] × z1[s] = A[k][r] × B[k][s]
 *
 * 8 次迭代后:
 *   ZA[r][s] = Σ(k) A[k][r] × B[k][s] = (A^T × B)[r][s]  ✓
 *
 * 寄存器映射:
 *   p0   — VL8 全真谓词
 *   z0   — A 的第 k 行
 *   z1   — B 的第 k 行
 *   x3   — 循环计数器 k (0→7)
 *   x4   — &A[k][0]
 *   x5   — &B[k][0]
 *
 * 注: 不再需要 z2 gather 偏移向量
 */

__attribute__((noinline))
static void sme_matmul_at_b(mat_t C, const mat_t A, const mat_t B) {
    __asm__ volatile (

        /*===== 1. 进入 SME streaming 模式 =====*/
        "smstart\n\t"

        /*===== 2. 生成全真谓词 p0, VL8 = 8 × f32 =====*/
        "ptrue  p0.s, vl8\n\t"

        /*===== 3. 清零 ZA 阵列 =====*/
        "zero   {za}\n\t"

        /*===== 4. K 外积循环 (k = 0..7) =====*/
        "mov    x3, #0\n\t"
        ".Lat_loop:\n\t"

        /* 加载 A 的第 k 行 (连续, row-major 天然连续) */
        "add    x4, %[A], x3, lsl #5\n\t"       // x4 = A + k*32
        "ld1w   {z0.s}, p0/z, [x4]\n\t"         // z0 = A[k][0..7]

        /* 加载 B 的第 k 行 (连续) */
        "add    x5, %[B], x3, lsl #5\n\t"       // x5 = B + k*32
        "ld1w   {z1.s}, p0/z, [x5]\n\t"         // z1 = B[k][0..7]

        /* 外积累加: ZA[r][s] += A[k][r] × B[k][s] */
        "fmopa  za0.s, p0/m, p0/m, z0.s, z1.s\n\t"

        /* 循环控制 */
        "add    x3, x3, #1\n\t"
        "cmp    x3, #8\n\t"
        "b.ne   .Lat_loop\n\t"

        /*===== 5. 将 ZA 8 行写回 C (LLVM SME2 STR) =====*/
        "mov    x7, %[C]\n\t"

        "mov    w12, #0\n\t"
        "str    za[w12, 0], [x7]\n\t"
        "add    x7, x7, #32\n\t"

        "mov    w12, #4\n\t"
        "str    za[w12, 0], [x7]\n\t"
        "add    x7, x7, #32\n\t"

        "mov    w12, #8\n\t"
        "str    za[w12, 0], [x7]\n\t"
        "add    x7, x7, #32\n\t"

        "mov    w12, #12\n\t"
        "str    za[w12, 0], [x7]\n\t"
        "add    x7, x7, #32\n\t"

        "mov    w12, #16\n\t"
        "str    za[w12, 0], [x7]\n\t"
        "add    x7, x7, #32\n\t"

        "mov    w12, #20\n\t"
        "str    za[w12, 0], [x7]\n\t"
        "add    x7, x7, #32\n\t"

        "mov    w12, #24\n\t"
        "str    za[w12, 0], [x7]\n\t"
        "add    x7, x7, #32\n\t"

        "mov    w12, #28\n\t"
        "str    za[w12, 0], [x7]\n\t"

        /*===== 6. 退出 SME streaming 模式 =====*/
        "smstop\n\t"

        : /* 无输出操作数 (通过 [C] 指针间接写回) */
        : [A] "r" (A), [B] "r" (B), [C] "r" (C)
        : "x3", "x4", "x5", "x7",
          "w12",
          "p0", "z0", "z1", "za", "memory"
    );
}

/*============================================================================
 * 参考实现 (纯 C)
 *============================================================================*/
static void matmul_at_b_ref(mat_t C, const mat_t A, const mat_t B) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            C[i][j] = 0.0f;
            for (int k = 0; k < N; k++)
                C[i][j] += A[k][i] * B[k][j];   // A^T[i][k] = A[k][i]
        }
}

/*============================================================================
 * 原版 C = A × B 参考实现 (用于对比)
 *============================================================================*/
static void matmul_ref(mat_t C, const mat_t A, const mat_t B) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            C[i][j] = 0.0f;
            for (int k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];
        }
}

#include <stdio.h>
#include <stdlib.h>

static void print_matrix(const char *name, const mat_t m) {
    printf("%s:\n", name);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++)
            printf("%7.1f ", m[i][j]);
        printf("\n");
    }
    printf("\n");
}

static int compare(const mat_t ref, const mat_t sme, float eps) {
    int errors = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            float d = ref[i][j] - sme[i][j];
            if (d < 0) d = -d;
            if (d > eps) errors++;
        }
    return errors;
}

int main(void) {
    mat_t A, B, C_ref, C_sme, C_old;

    /* A 和 B 都是独立矩阵 (B 不再是 A 的转置) */
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            A[i][j] = (float)(i * N + j + 1);           /* 1..64 */
            B[i][j] = (float)((i * N + j + 1) * 0.5f);  /* 0.5, 1.0, ... 32.0 */
        }

    printf("===== ARM SME: C = A^T × B  (A先转置再乘) =====\n\n");

    print_matrix("A (8×8, 原始)", A);
    print_matrix("B (8×8, 原始)", B);

    /* 1. 参考: C = A^T × B */
    matmul_at_b_ref(C_ref, A, B);
    print_matrix("C_ref = A^T × B (C参考)", C_ref);

    /* 2. SME: A先转置再乘 (即 C = A^T × B) */
    sme_matmul_at_b(C_sme, A, B);
    print_matrix("C_sme = A^T × B (SME汇编)", C_sme);

    int err = compare(C_ref, C_sme, 1e-3f);
    if (err == 0)
        printf("PASS: SME A^T × B 结果完全一致!\n\n");
    else
        printf("FAIL: 有 %d 个元素误差超标\n\n", err);

    /* 3. 对照: 不做转置的原版 C = A × B */
    matmul_ref(C_old, A, B);
    print_matrix("C_old = A × B (不转置, 仅对比)", C_old);

    printf("对比: C_ref (转置后乘) vs C_old (直接乘)\n");
    int diff = compare(C_ref, C_old, 1e-3f);
    printf("  差异元素数: %d (应为非零, 因为 A^T×B ≠ A×B)\n\n", diff);

    /* 4. 汇编核心代码行数对比 */
    printf("===== 代码简洁度对比 =====\n");
    printf("  原版 (A×B): 需要 INDEX + gather [x4, z2.s, uxtw #2], clobber z2\n");
    printf("  本版 (A^T×B): 两条普通 LD1W 连续加载, 仅 z0/z1\n");
    printf("  节省: 1 条指令(index) + 1 个 Z 寄存器 + 复杂寻址模式\n");

    return 0;
}
