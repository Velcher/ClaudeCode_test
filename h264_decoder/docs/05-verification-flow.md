# 5. 验证流程

## 5.1 验证环境拓扑

```
┌─────────────────────────────────────────────────────────┐
│ PC (控制端 - 你的电脑)                                   │
│                                                         │
│  ├── test.h264      (码流文件)                           │
│  ├── reference.yuv  (JM/FFmpeg 解码的参考输出)           │
│  └── verify.py      (自动化对比脚本)                     │
│         │                                               │
│         │ 网络 (TCP/UDP/共享内存/PCIe)                    │
│         ▼                                               │
│ ┌───────────────────────────────────┐                   │
│ │ 目标板 / FPGA 原型板               │                   │
│ │  ├── H264 解码器 IP              │                   │
│ │  ├── 寄存器配置接口 (AXI-lite)    │                    │
│ │  ├── 码流 DMA 接口 (AXI-stream)   │                    │
│ │  └── 输出帧缓冲 (DDR)            │                    │
│ └───────────────────────────────────┘                   │
└─────────────────────────────────────────────────────────┘
```

## 5.2 分阶段验证计划

### Phase 0: 冒烟测试（Smoke Test）

**目标：确认芯片能正确复位、读写寄存器**

```
□ read VERSION 寄存器 → 非 0xFFFFFFFF 且非 0x00000000
□ write → read 一个可写寄存器 → 一致
□ SW_RST → 等复位完成 → 状态回到 IDLE
```

**冒烟 Pass 标准：** 寄存器读写正确。

### Phase 1: 单帧最小码流（1 小时搞定）

**目标：解码最优简单的一帧**

使用最简单的 H264 码流：
- **Baseline Profile**（CAVLC 熵解码，简单）
- **仅 I 帧**（没有帧间预测）
- **QCIF 分辨率**（176×144，内存小）
- **单帧**

```bash
# 生成最简单的测试码流
x264 --profile baseline --no-cabac \
     --bframes 0 --keyint 1 \      # 全部 I 帧
     --input-res 176x144 --fps 30 \
     -o test_1frame.h264 \
     --frames 1 \
     akiyo_qcif.yuv
```

操作步骤：
```
1. PC 通过网络把 test_1frame.h264 写入板端 DDR
2. 通过寄存器接口配置：
   - BS_ADDR 指向码流在 DDR 的位置
   - BS_SIZE = 码流文件大小
   - DISP_ADDR 指向输出缓冲区
3. 写 DEC_CMD = 1 启动
4. 等待完成 (轮询或中断)
5. 读状态寄存器：确认 DEC_DONE=1, ERROR_CODE=0
6. 从 DDR 读回 YUV 数据
7. 用 FFmpeg 生成参考 YUV
8. 比较 YUV：逐字节或 PSNR
```

**Phase 1 Pass 标准：** 芯片输出 YUV 与参考 YUV 像素级完全一致（或 PSNR > 50dB）。

### Phase 2: 多帧 I-only（半天）

**目标：连续解码多帧，验证缓冲区管理**

```
□ 10 帧 I-only（不同分辨率：QCIF → CIF → SD → HD）
□ 检查帧边界是否有残留数据（前帧数据泄漏到当前帧）
□ 如果解码器支持显示重排序，检查帧序正确性
```

### Phase 3: I+P 帧（1 天）

**目标：运动补偿、参考帧管理**

```
□ 单 P 帧解码（I + P，2 帧）
□ 多 P 帧（I + P + P + P ...）
□ 参考帧列表管理（L0 list）
□ 不同参考索引的 P 块
□ 分数像素插值（1/4 pel MC）
```

### Phase 4: I+P+B 帧（1 天）

**目标：B 帧、双向预测、显示重排序**

```
□ Main Profile, I+P+B
□ POC (Picture Order Count) 正确的显示重排序
□ 双向加权预测
□ 参考帧标记（MMCO: Memory Management Control Operation）
```

### Phase 5: CABAC + 高级特性（2-3 天）

```
□ CABAC 熵解码：bin 计数、上下文更新
□ 8×8 变换（High Profile）
□ 自适应宏块级量化
□ 去块滤波边界处理
□ 场编码 / MBAFF（如果有）
□ 多 slice
□ DP (Data Partitioning)
```

### Phase 6: 压力测试 & 错误鲁棒性（2 天）

```
□ 最大分辨率 / 最大帧率
□ 最大参考帧数填满 DPB
□ 码流错误注入（中间截断、伪造 NAL 头）
□ 内存带宽压测（其他模块同时占用 DDR）
□ 长时间连续解码（跑一晚上）
```

## 5.3 自动化验证脚本

```python
#!/usr/bin/env python3
"""
h264_verify.py — H264 解码器自动化验证脚本

功能：
1. 通过 socket/JTAG/寄存器接口控制目标解码器
2. 发送码流，读取解码 YUV 输出
3. 与参考 YUV 逐帧比对
4. 计算 PSNR，生成报告
"""

import subprocess
import struct
import sys
import os
import math
import argparse

# ============================================================
# 配置区 — 根据你的环境修改
# ============================================================

class DecoderInterface:
    """抽象接口层 — 你需要实现自己的通信方式"""

    def reset(self):
        """复位解码器"""
        raise NotImplementedError

    def write_reg(self, addr, value):
        """写寄存器"""
        raise NotImplementedError

    def read_reg(self, addr):
        """读寄存器"""
        raise NotImplementedError

    def load_bitstream(self, data):
        """把码流加载到解码器缓冲区"""
        raise NotImplementedError

    def read_output_frame(self, width, height):
        """读回解码后的 YUV420 帧"""
        raise NotImplementedError

    def start_decode(self):
        """启动解码"""
        raise NotImplementedError

    def wait_done(self, timeout_ms=1000):
        """等待解码完成，返回 True/False"""
        raise NotImplementedError

# 示例：如果通过网络控制
class NetworkDecoder(DecoderInterface):
    def __init__(self, host, port):
        import socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))

    def reset(self):
        self.sock.send(b"reset\n")
        resp = self.sock.recv(1024)

    def write_reg(self, addr, value):
        self.sock.send(f"w {addr:x} {value:x}\n".encode())
        resp = self.sock.recv(1024)

    def read_reg(self, addr):
        self.sock.send(f"r {addr:x}\n".encode())
        return int(self.sock.recv(1024).strip(), 16)

    # ... 实现其余方法


# ============================================================
# 码流工具
# ============================================================

def generate_reference_yuv(h264_path, output_path, width, height):
    """用 FFmpeg 生成参考 YUV420 planar"""
    subprocess.run([
        "ffmpeg", "-y",
        "-i", h264_path,
        "-pix_fmt", "yuv420p",
        "-f", "rawvideo",
        "-vf", f"scale={width}:{height}",  # 如果需要缩放
        output_path
    ], check=True, capture_output=True)

def split_yuv_frames(yuv_path, width, height, frame_idx):
    """从 YUV 文件提取指定帧"""
    frame_size = width * height * 3 // 2
    offset = frame_idx * frame_size
    with open(yuv_path, "rb") as f:
        f.seek(offset)
        return f.read(frame_size)


# ============================================================
# PSNR 计算
# ============================================================

def psnr_frame(frame_a, frame_b, width, height):
    """计算两帧 YUV 的 PSNR (Y, U, V 分别计算)"""

    def mse_plane(data_a, data_b, w, h):
        pixels = w * h
        ssd = 0
        for i in range(pixels):
            diff = data_a[i] - data_b[i]
            ssd += diff * diff
        return ssd / pixels

    size_y  = width * height
    size_uv = width * height // 4

    y_a  = frame_a[:size_y]
    y_b  = frame_b[:size_y]
    u_a  = frame_a[size_y:size_y+size_uv]
    u_b  = frame_b[size_y:size_y+size_uv]
    v_a  = frame_a[size_y+size_uv:]
    v_b  = frame_b[size_y+size_uv:]

    mse_y = mse_plane(y_a, y_b, width, height)
    mse_u = mse_plane(u_a, u_b, width//2, height//2)
    mse_v = mse_plane(v_a, v_b, width//2, height//2)

    def to_db(mse):
        if mse == 0:
            return 99.99
        return 10 * math.log10(255*255 / mse)

    return to_db(mse_y), to_db(mse_u), to_db(mse_v)

def psnr_yuv_file(file_a, file_b, width, height):
    """计算两个 YUV 文件的 PSNR（逐帧对比）"""
    frame_size = width * height * 3 // 2
    fa = open(file_a, "rb").read()
    fb = open(file_b, "rb").read()

    num_frames = min(len(fa), len(fb)) // frame_size
    if len(fa) != len(fb):
        print(f"⚠ 文件长度不一致: {len(fa)} vs {len(fb)}, "
              f"比较前 {num_frames} 帧")

    results = []
    avg_psnr = [0, 0, 0]
    for i in range(num_frames):
        off = i * frame_size
        py, pu, pv = psnr_frame(fa[off:off+frame_size],
                                fb[off:off+frame_size],
                                width, height)
        results.append((py, pu, pv))
        avg_psnr[0] += py
        avg_psnr[1] += pu
        avg_psnr[2] += pv

    avg_psnr = [x / num_frames for x in avg_psnr]

    print(f"\n{'='*50}")
    print(f"PSNR 报告 ({num_frames} 帧, {width}×{height})")
    print(f"{'='*50}")
    print(f"  Y 均值:  {avg_psnr[0]:.2f} dB")
    print(f"  U 均值:  {avg_psnr[1]:.2f} dB")
    print(f"  V 均值:  {avg_psnr[2]:.2f} dB")
    print(f"  YUV 均值: {(avg_psnr[0]+avg_psnr[1]+avg_psnr[2])/3:.2f} dB")

    # 标记不合格帧
    threshold = 40.0  # Y PSNR 阈值
    for i, (py, pu, pv) in enumerate(results):
        if py < threshold:
            print(f"  ⚠ 帧 {i}: Y={py:.1f} < {threshold} dB — 可能解码错误!")

    return avg_psnr


# ============================================================
# 逐像素 Diff（精确定位错误）
# ============================================================

def pixel_diff(reference_yuv, chip_yuv, width, height, frame_idx=0):
    """逐像素比较，输出第一个不一致像素的位置"""
    frame_size = width * height * 3 // 2
    off = frame_idx * frame_size

    ref = reference_yuv[off:off+frame_size]
    chip = chip_yuv[off:off+frame_size]

    errors = []
    for i, (r, c) in enumerate(zip(ref, chip)):
        if r != c:
            errors.append(i)
            if len(errors) >= 50:  # 最多报告50个
                break

    if errors:
        total_size = width * height  # 只关注 Y 平面
        for idx in errors[:10]:
            if idx < total_size:
                py = idx // width
                px = idx % width
                print(f"  Y[{py}][{px}] ref={ref[idx]:3d} chip={chip[idx]:3d} "
                      f"diff={abs(ref[idx]-chip[idx])}")
            elif idx < total_size + total_size//4:
                off_c = idx - total_size
                py = off_c // (width//2)
                px = off_c % (width//2)
                print(f"  U[{py}][{px}] ref={ref[idx]:3d} chip={chip[idx]:3d}")
            else:
                print(f"  V[{idx}] ref={ref[idx]} chip={chip[idx]}")
        print(f"  ... 共 {len(errors)} 个像素不一致")

    return len(errors) == 0


# ============================================================
# 单次测试
# ============================================================

def run_single_test(dec, h264_path, width, height, frame_count,
                    ref_yuv_path=None):
    """运行一次解码测试"""
    # 1. 加载码流
    with open(h264_path, "rb") as f:
        bs_data = f.read()
    dec.load_bitstream(bs_data)

    # 2. 生成参考
    if ref_yuv_path is None:
        ref_yuv_path = h264_path + ".ref.yuv"
    if not os.path.exists(ref_yuv_path):
        generate_reference_yuv(h264_path, ref_yuv_path, width, height)

    # 3. 等待解码完成
    if not dec.wait_done(timeout_ms=5000):
        print("❌ 解码超时!")
        return False

    # 4. 读取芯片输出
    chip_output = dec.read_output_frame(width, height)

    # 5. 对比
    ref_data = open(ref_yuv_path, "rb").read()
    return pixel_diff(ref_data, chip_output, width, height)


# ============================================================
# 主流程
# ============================================================

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="H264 Decoder Verification")
    parser.add_argument("--host", default="192.168.1.100",
                        help="解码器板 IP 地址")
    parser.add_argument("--port", type=int, default=8888,
                        help="控制端口")
    parser.add_argument("--h264", required=True,
                        help="测试码流路径")
    parser.add_argument("--width", type=int, required=True,
                        help="图像宽度")
    parser.add_argument("--height", type=int, required=True,
                        help="图像高度")
    parser.add_argument("--ref", help="参考 YUV 路径（可选）")
    parser.add_argument("--loop", type=int, default=1,
                        help="循环测试次数")
    args = parser.parse_args()

    dec = NetworkDecoder(args.host, args.port)

    for i in range(args.loop):
        print(f"\n--- 测试 #{i+1} ---")
        ok = run_single_test(dec, args.h264,
                            args.width, args.height,
                            frame_count=1,
                            ref_yuv_path=args.ref)
        if ok:
            print(f"  ✅ 测试 #{i+1} PASS")
        else:
            print(f"  ❌ 测试 #{i+1} FAIL")
            sys.exit(1)

    print("\n🎉 所有测试通过!")
```

## 5.4 关键检查点 Checklist

```
□ 1. 解码完成中断/标志正常触发
□ 2. 输出 YUV 尺寸正确（W×H，没有丢行/多行）
□ 3. 第一帧 = I 帧 = IDR → 解码成功
□ 4. YUV 三个平面顺序正确（Y then U then V，或 NV12）
□ 5. 色度采样位置正确（center vs co-sited）
□ 6. P 帧参考索引正确
□ 7. B 帧显示顺序正确（POC 重排）
□ 8. 去块滤波边缘处理
□ 9. 码流错误恢复能力（丢包/截断后正常解码后续帧）
□ 10. 内存不泄漏（连续解码后帧缓冲不变）
```
