# H264 解码器芯片验证指南

> 目标：在局域网络环境中对 H264 解码器 ASIC/IP 进行功能验证

## 目录结构

```
h264_decoder/
├── README.md                      ← 本文件
├── docs/
│   ├── 01-bitstream.md            ← H264 码流文件：从哪拿到/怎样自己生成
│   ├── 02-reference-yuv.md        ← 参考 YUV420：格式说明 + 生成方法
│   ├── 03-cabac-tables.md         ← CABAC 表：需要哪些表、从哪里取、和寄存器的关系
│   ├── 04-register-guide.md       ← 寄存器配置：通用模式 + 具体 IP 参考
│   └── 05-verification-flow.md    ← 验证流程：分阶段计划 + 自动化脚本
├── scripts/
│   ├── gen_test_stream.sh         ← 一键生成测试码流 + 参考 YUV
│   └── yuv_compare.py             ← 对比芯片输出 vs 参考 YUV，计算 PSNR
├── tb/                            ← 测试平台（预留）
└── tests/streams/                 ← 放测试码流和参考 YUV
```

## 快速回答你的四个问题

### 1. 码流文件从哪来？

三个来源：

| 来源 | 说明 |
|------|------|
| **JM 一致性码流** | ITU/ISO 官方测试码流，覆盖各种 feature，最权威 |
| **自己用 x264 编码** | `x264 --profile baseline --input-res 352x288 -o test.h264 input.yuv` |
| **FFmpeg 编码** | `ffmpeg -i input.yuv -c:v libx264 test.h264` |

详见 → [docs/01-bitstream.md](docs/01-bitstream.md)

### 2. 参考 YUV420 怎么生成？

用 **FFmpeg 或 JM 参考解码器** 解码同一个码流：
```bash
# FFmpeg（最简单）
ffmpeg -i test.h264 -pix_fmt yuv420p -f rawvideo reference.yuv

# JM 参考解码器（最标准）
ldecod -i test.h264 -o reference.yuv
```

对比工具：
```bash
python3 scripts/yuv_compare.py chip_output.yuv reference.yuv 352 288
```

详见 → [docs/02-reference-yuv.md](docs/02-reference-yuv.md)

### 3. CABAC 表要不要？

- **需要 CABAC 表** — 但大部分硬件解码器**内部已固化**这些表，验证时你不需要提供表文件
- 如果你用的是 **Baseline Profile（CAVLC）**，不涉及 CABAC，直接开始验证最简单
- 如果验证 **Main/High Profile（CABAC）**，上下文初始化表、状态转换表来自 H.264 标准 §9.3 和 JM 源码

详见 → [docs/03-cabac-tables.md](docs/03-cabac-tables.md)

### 4. 寄存器配置有参考吗？

有通用模式。

核心流程就 8 步：
```
复位 → 总线配置 → 码流缓冲区 → 参考帧缓冲 → 输出缓冲 → 解码参数 → 启动 → 等完成
```

常用寄存器类别（各厂商都类似）：
- 芯片控制：VERSION、SW_RST、CLK_EN、IRQ_MASK
- 码流缓冲：BS_BASE_ADDR、BS_SIZE
- 参考帧 DPB：REF0_Y_ADDR、REF0_STRIDE
- 解码参数：PIC_WIDTH、PIC_HEIGHT、PROFILE
- 输出缓冲：DISP_Y_ADDR、DISP_STRIDE、DISP_FMT
- 解码控制：DEC_CMD、DEC_STATUS、ERROR_CODE

详见 → [docs/04-register-guide.md](docs/04-register-guide.md)

## 建议的验证路线

```
Phase 0: 冒烟 — 读 VERSION 寄存器，确认芯片活着
Phase 1: 单帧 Baseline I 帧 QCIF — 一天搞定，打通全流程
Phase 2: 多帧 I-only — 半天，验证缓冲区管理
Phase 3: I+P 帧 — 验证运动补偿
Phase 4: I+P+B + CABAC — 验证完整功能
Phase 5: 压力测试 + 错误注入 — 验证稳定性
```

详见 → [docs/05-verification-flow.md](docs/05-verification-flow.md)

## 快速上手

```bash
# 1. 生成测试码流和参考 YUV
bash scripts/gen_test_stream.sh ./tests/streams 176 144 5 baseline

# 2. 把码流送到解码器，读回输出 YUV（这部分需要对接你的硬件）
#   (通过你的网络接口实现)

# 3. 对比输出
python3 scripts/yuv_compare.py chip_output.yuv tests/streams/ref_*.yuv 176 144
```

## 还需要什么？

告诉我你的解码器的具体信息（IP 型号、数据手册有无、通信接口是什么），我可以帮你：
- 写对应的寄存器配置序列
- 写网络控制接口
- 生成特定 profile/分辨率/帧数的测试码流
