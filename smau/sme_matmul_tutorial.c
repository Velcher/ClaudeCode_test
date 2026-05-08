/**
 * sme_matmul_tutorial.c — ARM SME 8×8 矩阵乘法 入门教程
 *
 *   C = A × B    (三个 8×8 单精度浮点矩阵, 纯乘法, 无累加)
 *
 * 本文件面向初学者, 用纯内联汇编实现 SME 外积法矩阵乘法。
 * 每一行汇编都有中文解释, 配合寄存器映射图理解数据流。
 *
 * 前置条件: svcntw = 8 (SVE 向量长度 ≥ 256 bits, 即 VL ≥ 256)
 *
 * Linux 编译运行:
 *   gcc -O2 -march=armv9-a+sme -o sme_tutorial sme_matmul_tutorial.c
 *   ./sme_tutorial
 *
 * QEMU bare-metal:
 *   aarch64-none-elf-gcc -O2 -march=armv9-a+sme -ffreestanding -nostdlib \
 *       -DBAREMETAL -Tlinker.ld -o sme_tutorial.elf sme_matmul_tutorial.c
 *   qemu-system-aarch64 -M virt -cpu max -nographic -semihosting \
 *       -kernel sme_tutorial.elf
 */

#include <stdint.h>

#define N 8

/* 8×8 单精度矩阵类型 */
typedef float mat_t[N][N];

/*============================================================================
 * 算法原理 (必读)
 *============================================================================
 *
 * ┌─────────────────────────────────────────────────────────────────┐
 * │  矩阵乘法:  C[i][j] = Σ(k=0..7) A[i][k] × B[k][j]              │
 * │                                                                  │
 * │  SME 做法 — 外积分解 (outer product):                            │
 * │   把矩阵乘法拆成 8 次外积:                                        │
 * │     C = Σ(k=0..7) A_col_k ⊗ B_row_k                             │
 * │   其中:                                                          │
 * │     A_col_k = A 的第 k 列  (8×1 列向量)                          │
 * │     B_row_k = B 的第 k 行  (1×8 行向量)                          │
 * │     外积:    (8×1) × (1×8) → (8×8), 直接累加到 ZA tile          │
 * │                                                                  │
 * │  关键 SME 指令 FMOPA:                                            │
 * │    fmopa za.s[0], p0/m, z0.s, z1.s                              │
 * │    含义: ZA += z0[i] × z1[j]    (i=0..7, j=0..7)               │
 * │    ZA.S[0] 是 ZA tile 0 的名字, p0 是谓词控制有效元素            │
 * └─────────────────────────────────────────────────────────────────┘
 *
 * ┌─────────────────────────────────────────────────────────────────┐
 * │  寄存器映射 (8 个元素 × 32-bit = 256 bits = VL8)                │
 * │                                                                  │
 * │  ZA tile (ZA.S tile 0):   8×8 浮点阵列, 共 64 个元素            │
 * │    ┌───────────────────────────────┐                             │
 * │    │ ZA.S[0] → 第 0 行  8 个 float │                             │
 * │    │ ZA.S[1] → 第 1 行  8 个 float │                             │
 * │    │ ZA.S[2] → 第 2 行  8 个 float │                             │
 * │    │   ...                         │                             │
 * │    │ ZA.S[7] → 第 7 行  8 个 float │                             │
 * │    └───────────────────────────────┘                             │
 * │                                                                  │
 * │  Z 寄存器 (向量寄存器, 512-bit 物理, 256-bit 有效):              │
 * │    z0  — A 的列向量 / ZA 行读出暂存                              │
 * │    z1  — B 的行向量                                              │
 * │    z2  — gather 偏移索引: [0, 8, 16, 24, 32, 40, 48, 56]        │
 * │                                                                  │
 * │  通用寄存器:                                                     │
 * │    x0/x1/x2 — 函数参数 A, B, C 的指针                            │
 * │    x3       — k 循环计数器 (0→7)                                 │
 * │    x4       — A 当前 k 列基地址 = &A[0][k]                       │
 * │    x6       — B 当前 k 行基地址 = &B[k][0]                       │
 * │                                                                  │
 * │  谓词寄存器:                                                     │
 * │    p0       — VL8 全真谓词, 激活全部 8 个 32-bit 通道            │
 * └─────────────────────────────────────────────────────────────────┘
 *
 * ┌─────────────────────────────────────────────────────────────────┐
 * │  gather 偏移原理 (为什么 z2 = [0,8,16,24,32,40,48,56]?)         │
 * │                                                                  │
 * │  A 矩阵 row-major 存储: A[i][k] 的地址 = &A[0][k] + i*N*4       │
 * │  即 &A[i][k] = base + i*32                                      │
 * │                                                                  │
 * │  INDEX 指令生成 z2.s[i] = i × 8   (i = 0..7)                    │
 * │  再配合 UXTW #2: 地址 = base + z2[i] × 4 = base + i×32 ✓        │
 * │                                                                  │
 * │  同理, B 矩阵的同一行连续存储, 直接 LD1W 连续加载即可            │
 * └─────────────────────────────────────────────────────────────────┘
 */

/*============================================================================
 * 核心: 纯汇编 8×8 矩阵乘法  C = A × B
 *
 * 函数签名:
 *   sme_matmul_8x8(mat_t C, const mat_t A, const mat_t B)
 *
 * 数据流:
 *   1. 进入 SME streaming 模式
 *   2. 清零 ZA 阵列
 *   3. 生成 gather 偏移向量
 *   4. K 循环 ×8:  加载 A 列 → 加载 B 行 → FMOPA 外积累加
 *   5. 读出 ZA 8 行 → 写回 C
 *   6. 退出 SME streaming 模式
 *
 * 注意: 参数顺序是 (C, A, B), 对应寄存器 x0=C, x1=A, x2=B
 *============================================================================*/
__attribute__((noinline))
static void sme_matmul_8x8(mat_t C, const mat_t A, const mat_t B) {
    __asm__ volatile (

        /*===== 第 1 步: 进入 streaming SVE 模式 =====*/
        /*
         * SMSTART 开启 SME + Streaming SVE 模式。
         * 在此模式下:
         *   - ZA 寄存器阵列可用
         *   - Z 寄存器长度由 SVL 决定 (这里 SVL=256 bits = 8×f32)
         *   - 不能调用标准函数, 不能访问 NEON 寄存器
         */
        "smstart\n\t"

        /*===== 第 2 步: 生成全真谓词 =====*/
        /*
         * PTRUE p0.s, vl8
         * p0 = [T, T, T, T, T, T, T, T]    (8 个 32-bit 通道全部激活)
         *
         * 所有的 LD1W / FMOPA / MOV 都依赖 p0 控制哪些通道参与计算。
         * 用 "vl8" 显式指定 8 个元素, 确保和 8×8 矩阵尺寸一致。
         * 用 ".s" 指定 32-bit 浮点字 (single-word)。
         */
        "ptrue  p0.s, vl8\n\t"

        /*===== 第 3 步: 清零 ZA 阵列 =====*/
        /*
         * ZERO {ZA} — 将整个 ZA 存储阵列清零。
         * 纯乘法 C = A×B 必须从零开始, 如果有初始 C 就是 C += A×B。
         */
        "zero   {za}\n\t"

        /*===== 第 4 步: 生成 gather 偏移向量 =====*/
        /*
         * INDEX z2.s, #0, #8
         *
         *   通道:  [0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]
         *   z2.s =  0    8   16   24   32   40   48   56
         *
         * 配合 UXTW #2: 内存地址偏移 = z2[i] × 4
         *   = 0, 32, 64, 96, 128, 160, 192, 224
         *
         * 这正是 row-major 矩阵中同一列、不同行的元素间距:
         *   &A[i][k] - &A[0][k] = i × 8 × 4 = i × 32  ✓
         */
        "index  z2.s, #0, #8\n\t"

        /*===== 第 5 步: K 外积循环 (k = 0..7) =====*/
        /*
         * 每次循环取 A 的一列 + B 的一行, 做一个外积累加。
         * 循环 8 次后, ZA 中就是完整的 C = A × B。
         */
        "mov    x3, #0\n\t"
        ".Lk_loop:\n\t"

        /*--- 5a. 加载 A 的第 k 列到 z0 ---*/
        /*
         * x4 = &A[0][k]
         *   每列间隔 4 字节 (一个 float), 所以 offset = k*4
         *
         * LD1W {z0.s}, p0/z, [x4, z2.s, uxtw #2]
         *   "p0/z" — 未激活通道填 0 (此处全激活, 无所谓)
         *   "[x4, z2.s, uxtw #2]" — 寻址模式:
         *     基地址 x4 (= &A[0][k])
         *     + 索引 z2[i] × 4  (UXTW #2 = 左移 2 bit = ×4)
         *
         *   加载结果: z0[i] = A[i][k]
         *   z0 = [A[0][k], A[1][k], A[2][k], ..., A[7][k]]
         *   即 A 的第 k 列。
         */
        "add    x4, %[A], x3, lsl #2\n\t"       // x4 = A + k*4
        "ld1w   {z0.s}, p0/z, [x4, z2.s, uxtw #2]\n\t"

        /*--- 5b. 加载 B 的第 k 行到 z1 ---*/
        /*
         * x6 = &B[k][0]
         *   B 的同一行在内存中连续存放 (row-major)
         *   每行 8 个 float = 32 字节, 所以 offset = k*32
         *
         * LD1W {z1.s}, p0/z, [x6]
         *   连续加载 8 个 float: z1[j] = B[k][j]
         *   z1 = [B[k][0], B[k][1], B[k][2], ..., B[k][7]]
         *   即 B 的第 k 行。
         */
        "add    x6, %[B], x3, lsl #5\n\t"       // x6 = B + k*32
        "ld1w   {z1.s}, p0/z, [x6]\n\t"

        /*--- 5c. 外积累加 ---*/
        /*
         * FMOPA za.s[0], p0/m, z0.s, z1.s
         *
         *   对每个 (i,j):  ZA[i][j] += z0[i] × z1[j]
         *
         *   "p0/m" — 合并谓词 (merging):
         *     p0 为真的通道执行乘累加, 为假的通道保持 ZA 原值
         *
         *   这是整个算法的核心! 一条指令完成了 64 次乘加运算。
         *
         *   直观理解:
         *     列向量 [a0]        行向量 [b0 b1 ... b7]
         *            [a1]
         *            [...]
         *            [a7]
         *
         *     外积 =  a0×b0  a0×b1  ...  a0×b7     ← 累加到 ZA 第 0 行
         *            a1×b0  a1×b1  ...  a1×b7     ← 累加到 ZA 第 1 行
         *              ...     ...   ...   ...
         *            a7×b0  a7×b1  ...  a7×b7     ← 累加到 ZA 第 7 行
         */
        "fmopa  za.s[0], p0/m, z0.s, z1.s\n\t"

        /*--- 5d. 循环控制 ---*/
        "add    x3, x3, #1\n\t"
        "cmp    x3, #8\n\t"
        "b.ne   .Lk_loop\n\t"

        /*===== 第 6 步: 将 ZA 8 行写回 C 矩阵 =====*/
        /*
         * MOV z0.s, p0/m, za.s[N]
         *   将 ZA tile 的第 N 行读到向量寄存器 z0
         *
         * ST1W {z0.s}, p0, [%[C], #N, mul vl]
         *   将 z0 存到 C 的第 N 行
         *   "#N, mul vl" = offset = N × VL (字节)
         *   VL = 256 bits = 32 bytes, 所以每行间距 = 32 字节
         *
         * 注意: 写回时用 "p0" 而不是 "p0/z", 因为
         *   - ST1W 没有 zeroing predicate, 直接用 p0 控制写哪些通道
         */

        /* 第 0 行: 读出 ZA.S[0] → z0 → C[0][0..7] */
        "mov    z0.s, p0/m, za.s[0]\n\t"
        "st1w   {z0.s}, p0, [%[C], #0, mul vl]\n\t"

        /* 第 1 行 */
        "mov    z0.s, p0/m, za.s[1]\n\t"
        "st1w   {z0.s}, p0, [%[C], #1, mul vl]\n\t"

        /* 第 2 行 */
        "mov    z0.s, p0/m, za.s[2]\n\t"
        "st1w   {z0.s}, p0, [%[C], #2, mul vl]\n\t"

        /* 第 3 行 */
        "mov    z0.s, p0/m, za.s[3]\n\t"
        "st1w   {z0.s}, p0, [%[C], #3, mul vl]\n\t"

        /* 第 4 行 */
        "mov    z0.s, p0/m, za.s[4]\n\t"
        "st1w   {z0.s}, p0, [%[C], #4, mul vl]\n\t"

        /* 第 5 行 */
        "mov    z0.s, p0/m, za.s[5]\n\t"
        "st1w   {z0.s}, p0, [%[C], #5, mul vl]\n\t"

        /* 第 6 行 */
        "mov    z0.s, p0/m, za.s[6]\n\t"
        "st1w   {z0.s}, p0, [%[C], #6, mul vl]\n\t"

        /* 第 7 行 */
        "mov    z0.s, p0/m, za.s[7]\n\t"
        "st1w   {z0.s}, p0, [%[C], #7, mul vl]\n\t"

        /*===== 第 7 步: 退出 streaming SVE 模式 =====*/
        /*
         * SMSTOP — 关闭 SME, 恢复为标准 SVE/NEON 模式。
         * 此后不能再访问 ZA, 但可以正常使用 NEON 和标准 SVE。
         */
        "smstop\n\t"

        : /* 无 C 输出操作数 (通过指针间接写回) */
        : [A] "r" (A), [B] "r" (B), [C] "r" (C)
          /* 输入操作数: 矩阵指针 */
        : "x3", "x4", "x6",
          "p0", "z0", "z1", "z2", "za", "memory"
          /* clobber 列表:
           *   x3 — k 循环计数器
           *   x4 — A 列基地址
           *   x6 — B 行基地址
           *   p0 — 谓词寄存器
           *   z0,z1,z2 — 向量寄存器
           *   za  — 整个 ZA 阵列
           *   memory — 通过指针访问内存, 通知编译器 */
    );
}

/*============================================================================
 * 以下是验证代码, 不是 SME 教程的核心部分
 *============================================================================*/

/* 纯 C 参考实现 (和汇编逐位对比用) */
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
    mat_t A, B, C_ref, C_sme;

    /* 初始化测试数据: A[i][j] = i*8+j+1,  B = A 的转置 */
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            A[i][j] = (float)(i * N + j + 1);      /* 1..64 */
            B[i][j] = (float)(j * N + i + 1);      /* 转置 */
        }

    printf("===== ARM SME 8×8 矩阵乘法 教程 =====\n\n");

    print_matrix("A (8×8 顺序值)", A);
    print_matrix("B (A 的转置)",   B);

    /* 参考实现 */
    matmul_ref(C_ref, A, B);
    /* SME 实现 */
    sme_matmul_8x8(C_sme, A, B);

    print_matrix("C_ref (纯 C 参考结果)", C_ref);
    print_matrix("C_sme (SME 汇编结果)",   C_sme);

    int err = compare(C_ref, C_sme, 1e-3f);
    if (err == 0)
        printf("PASS: SME 结果与参考完全一致!\n");
    else
        printf("FAIL: 有 %d 个元素误差超标\n", err);

    return 0;
}
