/**
 * mat_test.c — ARM64 NEON Test Stimulus: 8x8 Float Matrix Multiply-Add
 *
 *   C = A x B + C
 *
 * Uses ARM NEON inline assembly (AArch64). Built-in reference comparison.
 *
 * Build for Linux (AArch64):
 *   gcc -O2 -o mat_test mat_test.c && ./mat_test
 *
 * Build for bare-metal (QEMU ARM64):
 *   aarch64-none-elf-gcc -O2 -mcpu=cortex-a72 -ffreestanding -nostdlib \
 *       -DBAREMETAL -Tlinker.ld -o mat_test.elf mat_test.c
 *   qemu-system-aarch64 -M virt -cpu max -nographic -semihosting \
 *       -kernel mat_test.elf
 */

#include <stdint.h>

#define N 8

/* 8x8 float matrix */
typedef float mat_t[N][N];

/*============================================================================
 * Bare-metal startup — Minimal AArch64 entry point
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

/*============================================================================
 * Bare-metal I/O — ARM Semihosting (AArch64)
 *============================================================================*/

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
#include <string.h>

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
    uint32_t exp = (fu.u >> 23) & 0xFF;
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

static void print_matrix(const char *name, mat_t m) {
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
 *============================================================================*/
static void mat_mul_add_ref(mat_t A, mat_t B, mat_t C) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];
}

/*============================================================================
 * NEON inline assembly implementation
 *
 * Strategy: process each row of A with two column-groups of C (j=0..3, 4..7).
 * For each (i, j) group, unroll the k loop (0..7) inside a single asm block.
 *
 *   v0  — accumulator (C[i][j:j+3])
 *   v1  — A[i][k] broadcast via LD1R
 *   v2  — B[k][j:j+3] via LD1
 *   fmla v0.4s, v2.4s, v1.4s  →  v0 += B[k][j..j+3] * A[i][k]
 *============================================================================*/
static void mat_mul_add_neon(mat_t A, mat_t B, mat_t C) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j += 4) {
            __asm__ volatile (
                "ld1   {v0.4s}, [%[c]]\n\t"          /* load C[i][j:j+3] */

                "ld1r  {v1.4s}, [%[a0]]\n\t"
                "ld1   {v2.4s}, [%[b0]]\n\t"
                "fmla  v0.4s, v2.4s, v1.4s\n\t"

                "ld1r  {v1.4s}, [%[a1]]\n\t"
                "ld1   {v2.4s}, [%[b1]]\n\t"
                "fmla  v0.4s, v2.4s, v1.4s\n\t"

                "ld1r  {v1.4s}, [%[a2]]\n\t"
                "ld1   {v2.4s}, [%[b2]]\n\t"
                "fmla  v0.4s, v2.4s, v1.4s\n\t"

                "ld1r  {v1.4s}, [%[a3]]\n\t"
                "ld1   {v2.4s}, [%[b3]]\n\t"
                "fmla  v0.4s, v2.4s, v1.4s\n\t"

                "ld1r  {v1.4s}, [%[a4]]\n\t"
                "ld1   {v2.4s}, [%[b4]]\n\t"
                "fmla  v0.4s, v2.4s, v1.4s\n\t"

                "ld1r  {v1.4s}, [%[a5]]\n\t"
                "ld1   {v2.4s}, [%[b5]]\n\t"
                "fmla  v0.4s, v2.4s, v1.4s\n\t"

                "ld1r  {v1.4s}, [%[a6]]\n\t"
                "ld1   {v2.4s}, [%[b6]]\n\t"
                "fmla  v0.4s, v2.4s, v1.4s\n\t"

                "ld1r  {v1.4s}, [%[a7]]\n\t"
                "ld1   {v2.4s}, [%[b7]]\n\t"
                "fmla  v0.4s, v2.4s, v1.4s\n\t"

                "st1   {v0.4s}, [%[c]]\n\t"          /* store C[i][j:j+3] */

                : /* no C output operands */
                : [c]  "r" (&C[i][j]),
                  [a0] "r" (&A[i][0]), [b0] "r" (&B[0][j]),
                  [a1] "r" (&A[i][1]), [b1] "r" (&B[1][j]),
                  [a2] "r" (&A[i][2]), [b2] "r" (&B[2][j]),
                  [a3] "r" (&A[i][3]), [b3] "r" (&B[3][j]),
                  [a4] "r" (&A[i][4]), [b4] "r" (&B[4][j]),
                  [a5] "r" (&A[i][5]), [b5] "r" (&B[5][j]),
                  [a6] "r" (&A[i][6]), [b6] "r" (&B[6][j]),
                  [a7] "r" (&A[i][7]), [b7] "r" (&B[7][j])
                : "v0", "v1", "v2", "memory"
            );
        }
    }
}

/*============================================================================
 * Verification
 *============================================================================*/
static int compare_matrices(mat_t ref, mat_t result, float eps) {
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
 *============================================================================*/
static void init_data(mat_t A, mat_t B, mat_t C_ref, mat_t C_neon) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j]       = (float)(i * N + j + 1);      /* 1..64 */
            B[i][j]       = (float)(j * N + i + 1);      /* transpose of A */
            C_ref[i][j]   = 1.0f;
            C_neon[i][j]  = 1.0f;
        }
    }
}

/*============================================================================
 * Main
 *============================================================================*/
int main(void) {
    mat_t A, B, C_ref, C_neon;

    init_data(A, B, C_ref, C_neon);

    puts_("===== ARM64 NEON Matrix Multiply-Add Test =====\n");
    puts_("  C = A x B + C    (8x8 float)\n\n");

    print_matrix("A (input)", A);
    print_matrix("B (input)", B);
    print_matrix("C (initial)", C_ref);

    /* Reference */
    mat_mul_add_ref(A, B, C_ref);

    /* NEON inline assembly */
    mat_mul_add_neon(A, B, C_neon);

    print_matrix("C_ref  (C implementation)", C_ref);
    print_matrix("C_neon (NEON inline asm)",  C_neon);

    /* Verify */
    int err = compare_matrices(C_ref, C_neon, 1e-3f);
    if (err == 0) {
        puts_("PASS: NEON result matches reference.\n");
    } else {
        puts_("FAIL: NEON result differs from reference!  Errors: ");
        print_int(err);
        putc_('\n');
    }

    puts_("===== Done =====\n");
    exit_(0);
    return 0;
}
