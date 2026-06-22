# 3. CABAC 表

## 3.1 需要哪些表？

CABAC 解码**需要以下全部**（H.264 标准 §9.3 定义）：

### (a) 上下文初始化表 — ctxIdxOffset

每个 slice 开始时要初始化上下文模型。两个表：

| 表名 | 码流类型 | 用途 |
|------|----------|------|
| **Table 9-4** | I Slice | 帧内编码 slice |
| **Table 9-5** | P Slice | 帧间编码 slice |
| **Table 9-6** | B Slice | 双向预测 slice |

### (b) 初始化值对 — initValue

每个 ctxIdx 对应一对 (m, n)，用于推导初始状态：

```
pStateIdx = preCtxState = Clip3(1, 126, ((m * SliceQPY) >> 4) + n)
valMPS    = (pStateIdx <= 63) ? 0 : 1           // LPS = valMPS ^ 1
pStateIdx = (pStateIdx <= 63) ? (63 - pStateIdx) : (pStateIdx - 64)
```

m 和 n 从标准的 initValue 表查，共 **460 个上下文**（H.264 High Profile）。

### (c) 二值化表 (Binarization)

不同语法元素用不同的二值化方式：

| 语法元素 | 二值化方式 |
|----------|-----------|
| mb_type | 截断一元码 (TU) |
| mb_qp_delta | 一元码 |
| coeff_abs_level_minus1 | 指数哥伦布 (Exp-Golomb) |
| 运动矢量差 | 截断一元码 |
| coded_block_pattern | 截断一元码 + 定长 |

### (d) 上下文模型索引

每个语法元素的每个 bin，对应哪个 ctxIdx，见标准 §9.3.3。

### (e) 状态转换表 (LPS/MPS)

这个 H.264 解码器**必须内置**，见标准 Table 9-35：

```
// pStateIdx → nextState 转换表（460 条）
// 有 transIdxLPS[pStateIdx] 和 transIdxMPS[pStateIdx]

如果当前 bin == MPS:
    pStateIdx = transIdxMPS[pStateIdx]
否则 (bin == LPS):
    valMPS ^= (pStateIdx == 0) ? 1 : 0
    pStateIdx = transIdxLPS[pStateIdx]
```

## 3.2 对验证的影响

**大部分硬件解码器内部已固化这些表**，验证时你不需要提供表文件。但你需要确认：

1. **解码器支持哪些 profile** — Baseline 用 CAVLC，不需要 CABAC；Main/High 才用 CABAC
2. **如果没有 CABAC 表**（如调试模式），解码器应该报错还是 fallback 到 CAVLC？
3. **验证 CABAC 解码是否正确** → 用熵解码后的语法元素值对比 JM 输出的 trace 文件

## 3.3 完整的表数据来源

最佳来源是 **JM 参考软件的源代码**：

```
JM/ldecod/src/
├── cabac.c          ← initValue 表 (ctx_tables.c/MotionInfoContexts3[3])
├── ctx_tables.h     ← 上下文模型索引
└── biariencode.c    ← 状态转换表
```

关键全局变量（在 cabac.c 中）：
```c
// JM 中 ctx_init 结构
typedef struct {
    int m;
    int n;
} MotionInfoContexts[3][460];
```

## 3.4 寄存器和 CABAC 的关系

硬件解码器中，CABAC 引擎通常通过寄存器控制：

```
典型寄存器位域：
  [0]    CABAC_EN       — 使能 CABAC 解码
  [1]    CABAC_INIT     — 触发上下文初始化
  [2:0]  SLICE_TYPE     — 上下文初始化用哪个表(I/P/B)
  [7:3]  SLICE_QP_Y     — 亮度量化参数（参与 init 推导）
```

## 3.5 验证时用 CAVLC 还是 CABAC？

**建议验证顺序：**
1. 先用 **Baseline Profile（CAVLC）** — 简单，没有算术编码
2. 验证通过后再上 **Main Profile（CABAC）** — 逐步增加复杂度
3. 最后 **High Profile** — 8×8 变换，自定义量化矩阵

```bash
# 生成 Baseline 码流（没有 CABAC）
x264 --profile baseline --no-cabac -o test_baseline.h264 input.yuv

# 生成 Main 码流（有 CABAC）
x264 --profile main --cabac -o test_main.h264 input.yuv
```

## 3.6 CAVLC 对照表

如果你用的是 CAVLC（Baseline），需要的是 **VLC 码表**而非 CABAC 表：

| 表 | H.264 标准 | 用途 |
|----|-----------|------|
| coeff_token 表 | Table 9-5 | 非零系数数量和拖尾1数量 |
| total_zeros | Table 9-7 / 9-8 | 总零系数数 |
| run_before | Table 9-10 | 零游程分布 |
| level 前缀表 | Table 9-6 | 系数幅值 |

这些也在 JM 源码的 `vlc.c` 中。
