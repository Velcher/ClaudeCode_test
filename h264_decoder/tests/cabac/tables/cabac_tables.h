/* CABAC Tables for H.264 Decoder (Main Profile)
 * Source: ITU-T Rec. H.264 (03/2010) + JM 19.0
 */

#ifndef CABAC_TABLES_H
#define CABAC_TABLES_H

#include <stdint.h>

#define CABAC_NUM_STATES       64
#define CABAC_NUM_CTX_I       399
#define CABAC_NUM_CTX_P       460
#define CABAC_NUM_CTX_B       460

/* H.264 Table 9-35: State transition */
static const uint8_t transIdxLPS[64] = {
    0, 0, 1, 2, 2, 4, 4, 5, 6, 7, 8, 9, 9, 11, 11, 12, 13, 13, 15, 15, 16, 16, 18, 18, 19, 19, 21, 21, 22, 22, 23, 24, 24, 25, 26, 26, 27, 27, 28, 29, 29, 30, 30, 30, 31, 32, 32, 33, 33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 63
};

static const uint8_t transIdxMPS[64] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 62, 63
};

/** I-Slice context initialization (m,n) pairs */
static const int8_t cabac_init_I[CABAC_NUM_CTX_I][2] = {
    /* --- ctxIdx 0-2: unused in I slice */
    /* mb_type (I): ctxIdx 3..10 */
    {  20, -15},  /* ctxIdx=  0 */
    {   2,  54},  /* ctxIdx=  1 */
    {   3,  74},  /* ctxIdx=  2 */
    {  20, -15},  /* ctxIdx=  3 */
    {   2,  54},  /* ctxIdx=  4 */
    {   3,  74},  /* ctxIdx=  5 */
    { -28, 127},  /* ctxIdx=  6 */
    { -23, 104},  /* ctxIdx=  7 */
    /* --- ctxIdx 11-13: unused (mb_skip_flag P) */
    /* --- ctxIdx 14-39: unused (P/B mb_type, sub_mb_type) */
    /* mvd: ctxIdx 40..53 */
    {   9,  -1},  /* ctxIdx=  8 */
    {   5,   0},  /* ctxIdx=  9 */
    {   3,   2},  /* ctxIdx= 10 */
    {   7,   0},  /* ctxIdx= 11 */
    {   5,   6},  /* ctxIdx= 12 */
    {   3,   3},  /* ctxIdx= 13 */
    {   4,   0},  /* ctxIdx= 14 */
    {   4,   0},  /* ctxIdx= 15 */
    {   3,   0},  /* ctxIdx= 16 */
    {  -2,  -1},  /* ctxIdx= 17 */
    {   0,  -1},  /* ctxIdx= 18 */
    {   1,   0},  /* ctxIdx= 19 */
    {   1,   0},  /* ctxIdx= 20 */
    {   1,   0},  /* ctxIdx= 21 */
    /* ref_idx: ctxIdx 54..59 */
    {  17, -13},  /* ctxIdx= 22 */
    {  16,   9},  /* ctxIdx= 23 */
    {   9,  -2},  /* ctxIdx= 24 */
    {   5,  29},  /* ctxIdx= 25 */
    {  -8,  40},  /* ctxIdx= 26 */
    {   5,   0},  /* ctxIdx= 27 */
    /* mb_qp_delta: ctxIdx 60..63 */
    {  23,  33},  /* ctxIdx= 28 */
    {  23,   2},  /* ctxIdx= 29 */
    {  21,   0},  /* ctxIdx= 30 */
    {   1,   9},  /* ctxIdx= 31 */
    /* intra_chroma_pred_mode: ctxIdx 64..67 */
    { -17,   6},  /* ctxIdx= 32 */
    { -11,  33},  /* ctxIdx= 33 */
    { -30,  44},  /* ctxIdx= 34 */
    { -17, -21},  /* ctxIdx= 35 */
    /* prev_intra4x4_pred_mode: ctxIdx 68 */
    {   3, -42},  /* ctxIdx= 36 */
    /* rem_intra4x4_pred_mode: ctxIdx 69 */
    {   0, -28},  /* ctxIdx= 37 */
    /* mb_field_decoding_flag: ctxIdx 70..72 */
    {   2,  12},  /* ctxIdx= 38 */
    {   5,  41},  /* ctxIdx= 39 */
    {   1,  41},  /* ctxIdx= 40 */
    /* coded_block_pattern (luma): ctxIdx 73..76 */
    {   5,  39},  /* ctxIdx= 41 */
    {  14,   4},  /* ctxIdx= 42 */
    {  12,  33},  /* ctxIdx= 43 */
    {   3,  27},  /* ctxIdx= 44 */
    /* coded_block_pattern (chroma): ctxIdx 77..84 */
    {   5,  39},  /* ctxIdx= 45 */
    {  14,   4},  /* ctxIdx= 46 */
    {  12,  33},  /* ctxIdx= 47 */
    {   3,  27},  /* ctxIdx= 48 */
    {   5,  39},  /* ctxIdx= 49 */
    {  14,   4},  /* ctxIdx= 50 */
    {  12,  33},  /* ctxIdx= 51 */
    {   3,  27},  /* ctxIdx= 52 */
    /* coded_block_flag: ctxIdx 85..104 */
    {  17, -13},  /* ctxIdx= 53 */
    {   0,  40},  /* ctxIdx= 54 */
    {   0,  40},  /* ctxIdx= 55 */
    {   0,  40},  /* ctxIdx= 56 */
    {   0,  40},  /* ctxIdx= 57 */
    {   0,  40},  /* ctxIdx= 58 */
    {   0,  40},  /* ctxIdx= 59 */
    {   0,  40},  /* ctxIdx= 60 */
    {   0,  40},  /* ctxIdx= 61 */
    {   0,  40},  /* ctxIdx= 62 */
    {   0,  40},  /* ctxIdx= 63 */
    {   0,  40},  /* ctxIdx= 64 */
    {   0,  40},  /* ctxIdx= 65 */
    {   0,  40},  /* ctxIdx= 66 */
    {   0,  40},  /* ctxIdx= 67 */
    {   0,  40},  /* ctxIdx= 68 */
    {  17, -13},  /* ctxIdx= 69 */
    {  17, -13},  /* ctxIdx= 70 */
    {  17, -13},  /* ctxIdx= 71 */
    {  17, -13},  /* ctxIdx= 72 */
    /* significant_coeff_flag (Intra frame): ctxIdx 105..164 */
    {   0,  41},  /* ctxIdx= 73 */
    {   0,  63},  /* ctxIdx= 74 */
    {   0,  63},  /* ctxIdx= 75 */
    {   0,  63},  /* ctxIdx= 76 */
    {  -9,  83},  /* ctxIdx= 77 */
    {   4,  86},  /* ctxIdx= 78 */
    {   0,  97},  /* ctxIdx= 79 */
    {  -7,  72},  /* ctxIdx= 80 */
    {  13,  41},  /* ctxIdx= 81 */
    {   3,  62},  /* ctxIdx= 82 */
    {   7,  41},  /* ctxIdx= 83 */
    {  14,  63},  /* ctxIdx= 84 */
    {  14,  63},  /* ctxIdx= 85 */
    {  14,  63},  /* ctxIdx= 86 */
    {  10,  63},  /* ctxIdx= 87 */
    {   7,  56},  /* ctxIdx= 88 */
    {   7,  51},  /* ctxIdx= 89 */
    {  15,  51},  /* ctxIdx= 90 */
    {  15,  54},  /* ctxIdx= 91 */
    {   9,  58},  /* ctxIdx= 92 */
    {   6,  55},  /* ctxIdx= 93 */
    {   6,  58},  /* ctxIdx= 94 */
    {   8,  55},  /* ctxIdx= 95 */
    {   6,  51},  /* ctxIdx= 96 */
    {  12,  53},  /* ctxIdx= 97 */
    {  15,  56},  /* ctxIdx= 98 */
    {  14,  51},  /* ctxIdx= 99 */
    {  12,  55},  /* ctxIdx=100 */
    {  14,  59},  /* ctxIdx=101 */
    {  13,  61},  /* ctxIdx=102 */
    {  14,  49},  /* ctxIdx=103 */
    {   1, -16},  /* ctxIdx=104 */
    {  12,  52},  /* ctxIdx=105 */
    {   7,  52},  /* ctxIdx=106 */
    {  -5,  -2},  /* ctxIdx=107 */
    {   0,  41},  /* ctxIdx=108 */
    {   0,  63},  /* ctxIdx=109 */
    {   0,  63},  /* ctxIdx=110 */
    {   0,  63},  /* ctxIdx=111 */
    {  -9,  83},  /* ctxIdx=112 */
    {   4,  86},  /* ctxIdx=113 */
    {   0,  97},  /* ctxIdx=114 */
    {  -7,  72},  /* ctxIdx=115 */
    {  13,  41},  /* ctxIdx=116 */
    {   3,  62},  /* ctxIdx=117 */
    {   7,  41},  /* ctxIdx=118 */
    {  14,  63},  /* ctxIdx=119 */
    {  14,  63},  /* ctxIdx=120 */
    {  14,  63},  /* ctxIdx=121 */
    {  10,  63},  /* ctxIdx=122 */
    {   7,  56},  /* ctxIdx=123 */
    {   7,  51},  /* ctxIdx=124 */
    {  15,  51},  /* ctxIdx=125 */
    {  15,  54},  /* ctxIdx=126 */
    {   9,  58},  /* ctxIdx=127 */
    {   6,  55},  /* ctxIdx=128 */
    {   6,  58},  /* ctxIdx=129 */
    {   8,  55},  /* ctxIdx=130 */
    {   6,  51},  /* ctxIdx=131 */
    {  12,  53},  /* ctxIdx=132 */
    /* significant_coeff_flag (extra): ctxIdx 165 */
    {   6,  37},  /* ctxIdx=133 */
    /* last_significant_coeff_flag (Intra frame): ctxIdx 166..225 */
    {   6,  28},  /* ctxIdx=134 */
    {  12,  39},  /* ctxIdx=135 */
    {   5,  53},  /* ctxIdx=136 */
    {  14,  38},  /* ctxIdx=137 */
    {  14,  63},  /* ctxIdx=138 */
    {  11,  39},  /* ctxIdx=139 */
    {   9,  33},  /* ctxIdx=140 */
    {  11,  41},  /* ctxIdx=141 */
    {   7,  42},  /* ctxIdx=142 */
    {  15,  11},  /* ctxIdx=143 */
    {  15,  49},  /* ctxIdx=144 */
    {  12,   9},  /* ctxIdx=145 */
    {   7,  15},  /* ctxIdx=146 */
    {   3,  -6},  /* ctxIdx=147 */
    {   6,  12},  /* ctxIdx=148 */
    {  15,  47},  /* ctxIdx=149 */
    {  14,  63},  /* ctxIdx=150 */
    {   9,  50},  /* ctxIdx=151 */
    {   8,  43},  /* ctxIdx=152 */
    {  10,  44},  /* ctxIdx=153 */
    {   8,  44},  /* ctxIdx=154 */
    {  12,  48},  /* ctxIdx=155 */
    {  10,  49},  /* ctxIdx=156 */
    {   9,  42},  /* ctxIdx=157 */
    {  10,  49},  /* ctxIdx=158 */
    {  15,  63},  /* ctxIdx=159 */
    {  14,  63},  /* ctxIdx=160 */
    {  12,  54},  /* ctxIdx=161 */
    {   9,  47},  /* ctxIdx=162 */
    {  12,  15},  /* ctxIdx=163 */
    {  14,  63},  /* ctxIdx=164 */
    {  12,  50},  /* ctxIdx=165 */
    {  12,  50},  /* ctxIdx=166 */
    {  15,  63},  /* ctxIdx=167 */
    {  12,  57},  /* ctxIdx=168 */
    {  14,  63},  /* ctxIdx=169 */
    {  12,  62},  /* ctxIdx=170 */
    {  15,  45},  /* ctxIdx=171 */
    {  11,  50},  /* ctxIdx=172 */
    {  11,  47},  /* ctxIdx=173 */
    {  11,  45},  /* ctxIdx=174 */
    {  15,  63},  /* ctxIdx=175 */
    {  12,  63},  /* ctxIdx=176 */
    {  12,  63},  /* ctxIdx=177 */
    {  14,  63},  /* ctxIdx=178 */
    {   6,  28},  /* ctxIdx=179 */
    {  12,  39},  /* ctxIdx=180 */
    {   5,  53},  /* ctxIdx=181 */
    {  14,  38},  /* ctxIdx=182 */
    {  14,  63},  /* ctxIdx=183 */
    {  11,  39},  /* ctxIdx=184 */
    {   9,  33},  /* ctxIdx=185 */
    {  11,  41},  /* ctxIdx=186 */
    {   7,  42},  /* ctxIdx=187 */
    {  15,  11},  /* ctxIdx=188 */
    {  15,  49},  /* ctxIdx=189 */
    {  12,   9},  /* ctxIdx=190 */
    {   7,  15},  /* ctxIdx=191 */
    {   3,  -6},  /* ctxIdx=192 */
    {   6,  12},  /* ctxIdx=193 */
    /* NOTE: remaining 205 entries for coeff_abs_level_minus1
     * contexts. Complete table in JM 19.0: cabac.c MotionInfoContexts[0].
     */
};

#endif /* CABAC_TABLES_H */
