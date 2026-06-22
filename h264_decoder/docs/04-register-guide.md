# 4. H264 解码器寄存器配置参考

## 4.0 先确认你的解码器

⚠️ 不同厂商的 H264 解码器 IP 寄存器定义差异很大。请先确认：

- **解码器 IP 型号**：如 Chips&Media WAVE4/WAVE5、Allegro AL-D320、VeriSilicon Hantro G1、自研 IP 等
- **数据手册 (Databook/TRM)**：寄存器基地址、位域定义从这里来
- **参考驱动**：厂商通常提供 Linux V4L2 驱动，可作为配置参考

下面给出**通用模式**，各厂商都类似：

## 4.1 解码器工作流程（寄存器视角）

```
┌─────────────────────────────────────────────────────┐
│ 1. 复位 & 时钟配置                                    │
│    └→ SOFTRST / CLK_GATE                             │
├─────────────────────────────────────────────────────┤
│ 2. 总线配置                                           │
│    └→ AXI/AHB 基地址、IRQ 使能                       │
├─────────────────────────────────────────────────────┤
│ 3. 码流缓冲区设置                                     │
│    └→ BITSTREAM_ADDR[HI/LO], BITSTREAM_SIZE          │
├─────────────────────────────────────────────────────┤
│ 4. 参考帧缓冲区设置                                    │
│    └→ REF_PIC[0..N]_ADDR, REF_PIC_STRIDE             │
├─────────────────────────────────────────────────────┤
│ 5. 输出帧缓冲区设置                                    │
│    └→ DISPLAY_BUF_ADDR, DISPLAY_STRIDE                │
├─────────────────────────────────────────────────────┤
│ 6. 解码参数配置                                       │
│    └→ PIC_WIDTH, PIC_HEIGHT, PROFILE, ...             │
├─────────────────────────────────────────────────────┤
│ 7. 启动解码                                           │
│    └→ DEC_START / SW_CMD                             │
├─────────────────────────────────────────────────────┤
│ 8. 等待完成                                           │
│    └→ 轮询 STATUS / 等待 IRQ                         │
├─────────────────────────────────────────────────────┤
│ 9. 检查结果                                           │
│    └→ DEC_SUCCESS / ERROR_CODE                       │
└─────────────────────────────────────────────────────┘
```

## 4.2 通用寄存器分类

### 第一类：芯片控制寄存器

```
典型地址    寄存器名          说明
0x0000      VERSION          硬件版本号（只读）
0x0004      SW_RST           软件复位 [0]=1 复位, 自清 0
0x0008      CLK_EN           时钟使能 [0]=DEC core clk
0x000C      BUS_CFG          AXI 突发长度, outstanding 数
0x0010      IRQ_MASK         中断屏蔽 [0]=DONE, [1]=ERROR
0x0014      IRQ_STATUS       中断状态（写1清0）
```

### 第二类：码流缓冲区

```
0x0100      BS_BASE_L        码流基址低 32 位
0x0104      BS_BASE_H        码流基址高 32 位（64位系统）
0x0108      BS_SIZE          码流大小（字节）
0x010C      BS_RD_PTR        当前码流读指针（通常只读）
0x0110      BS_ENDIAN        端序控制 [0]=LE [1]=BE
0x0114      BS_START_CODE    [0]=1 检测起始码, [1]=1 跳过起始码
```

### 第三类：参考帧 / DPB 缓冲区

```
0x0200      REF0_Y_ADDR      参考帧 0 Y 平面地址
0x0204      REF0_C_ADDR      参考帧 0 CbCr 平面地址
0x0208      REF0_STRIDE_Y    Y 平面行跨度（字节）
0x020C      REF0_STRIDE_C    C 平面行跨度

// DPB 最多 16 帧（Level 4.1），每个类似
// 短参考（short-term）和长参考（long-term）
0x0300      LTR_PIC[0..N]    长参考帧控制
```

### 第四类：解码参数

```
0x0400      PIC_WIDTH        图像宽度（像素）
0x0404      PIC_HEIGHT       图像高度（像素）
0x0408      PIC_WIDTH_MB     = (width+15)/16, 通常自动计算
0x040C      PIC_HEIGHT_MB    = (height+15)/16
0x0410      PROFILE_LEVEL    [7:0]=profile, [15:8]=level
                              66=baseline, 77=main, 100=high
0x0414      CHROMA_FMT       [1:0]=0 → 420, 1→422, 2→444
0x0418      BIT_DEPTH        [3:0]=8/10 bit
0x041C      DEC_MODE          [0]=1 正常解码, [1]=1 跳过解码
                               [2]=1 只解析头部
```

### 第五类：输出帧缓冲区

```
0x0500      DISP_Y_ADDR      当前解码帧 Y 平面写入地址
0x0504      DISP_CB_ADDR     Cb 平面地址
0x0508      DISP_CR_ADDR     Cr 平面地址  (或 CB+CR 交错)
0x050C      DISP_STRIDE_Y    Y 行跨度
0x0510      DISP_STRIDE_C    色度行跨度
0x0514      DISP_FMT         输出格式 [1:0]:
                                0=YUV420 planar
                                1=YUV420 semi-planar (NV12)
                                2=YUV422
                                3=RGB (如果有)
```

### 第六类：解码控制/状态

```
0x0600      DEC_CMD          解码命令
                              [0]=1: 开始解码
                              [1]=1: 中止解码
0x0604      DEC_STATUS       解码状态（只读）
                              [0]=IDLE, [1]=BUSY, 2=ERROR
0x0608      DEC_DONE         解码完成标志
0x060C      ERROR_CODE       错误码
                              1=码流错误
                              2=不支持的 profile
                              3=参考帧丢失
                              4=缓冲区溢出
                              5=总线错误
0x0610      DEC_TIME         解码耗时（周期数）
```

### 第七类：后处理（如有）

```
0x0700      PP_EN            后处理使能
0x0704      PP_SCALE_W       缩放目标宽度
0x0708      PP_SCALE_H       缩放目标高度
0x070C      PP_ROTATE        旋转 90/180/270
```

## 4.3 典型配置序列示例（伪代码）

```c
// 步骤1: 上电复位
write32(SW_RST, 0x1);
while (read32(SW_RST) & 0x1);  // 等待复位完成
write32(CLK_EN, 0x1);
write32(IRQ_MASK, 0x3);       // 使能 DONE + ERROR 中断

// 步骤2: 配置码流
write32(BS_BASE_L, (uint32_t)(bitstream_addr & 0xFFFFFFFF));
write32(BS_BASE_H, (uint32_t)(bitstream_addr >> 32));  // 如果64位
write32(BS_SIZE, bitstream_size);
write32(BS_ENDIAN, 0);        // Little-endian

// 步骤3: 配置输出缓冲区
write32(DISP_Y_ADDR, y_buf_addr);
write32(DISP_CB_ADDR, cb_buf_addr);
write32(DISP_CR_ADDR, cr_buf_addr);
write32(DISP_STRIDE_Y, disp_width);     // 行跨度 = 宽度
write32(DISP_STRIDE_C, disp_width/2);
write32(DISP_FMT, 0);         // YUV420 planar

// 步骤4: 设置解码参数（如果寄存器需要）
write32(PIC_WIDTH, pic_width);
write32(PIC_HEIGHT, pic_height);

// 步骤5: 启动解码
write32(DEC_CMD, 0x1);

// 步骤6: 等待完成
// 方式A: 轮询
while ((read32(DEC_STATUS) & 0x1) == 0x1);  // 等待 BUSY 变 0
// 方式B: 中断
wait_irq(DEC_IRQ);

// 步骤7: 检查结果
uint32_t status = read32(DEC_STATUS);
if (status & 0x2) {
    uint32_t err = read32(ERROR_CODE);
    printf("Decode error: code=%d\n", err);
} else {
    printf("Decode OK, cycles=%d\n", read32(DEC_TIME));
    // 输出帧现在在 DISP_*_ADDR 指向的缓冲区中
}
```

## 4.4 具体解码器参考

### Chips&Media WAVE5xx 系列
- 寄存器通过 VPU API 间接访问
- 关键结构体：`DecHandle`, `DecOpenParam`, `DecInitialInfo`
- 参考驱动：`vpuapi/decoder.c`

### Allegro D320/D340
- 寄存器映射在 `/dev/mem` 或 PCI BAR
- `al5d320.h` 定义寄存器地址
- 参考：主线 Linux 内核驱动 `drivers/staging/media/allegro-dvt/`

### VeriSilicon Hantro G1
- `hantro_h1_regs.h`
- 寄存器前缀 `G1_REG_*`
- 参考：`drivers/staging/media/hantro/`

### 自研 IP
- 对照数据手册
- 以上通用模型仍然适用
- 先跑通最简单的 Baseline Profile P 帧解码

## 4.5 寄存器调试技巧

```bash
# Linux 上从用户空间 dump 寄存器映射（需 root）
devmem 0x<base_addr> w    # 写32位
devmem 0x<base_addr>      # 读32位

# 或者通过 /sys/kernel/debug
cat /sys/kernel/debug/regmap/dummy-axi/registers
```

**验证第一步应该做的事：**
1. 读 VERSION 寄存器 — 确认芯片是活的
2. 写 SW_RST → 读 SW_RST — 确认写操作生效
3. 写一个测试值到 DISP_Y_ADDR → 读回来 — 确认地址映射正确
