# 2. 生成参考 YUV 420

## 2.1 什么是 YUV 420

H264 解码器的标准输出是 **YUV 4:2:0 平面格式 (planar)**：

```
Y 平面: W×H   字节（每个像素一个亮度分量）
U 平面: W/2×H/2 字节（色度分量）
V 平面: W/2×H/2 字节（色度分量）

总大小 = W×H×1.5 字节

文件布局（planar）:
|<---  Y (W×H)  --->|<-- U (W/2×H/2) -->|<-- V (W/2×H/2) -->|
```

⚠️ 注意：YUV420 有 planar（Y然后U然后V）和 semi-planar（Y然后UV交错）两种。
大多数解码器输出 planar 格式，但**务必确认你的芯片输出哪种！**

## 2.2 生成参考 YUV 输出

### 方法一：JM 参考解码器（最标准）

```bash
# 下载编译 JM
git clone https://github.com/Srella/JM.git
cd JM
make

# 用 ldecod 解码生成参考 YUV
./bin/ldecod -i input.264 -o reference.yuv
```
JM 的输出是逐帧精确的，符合标准。

### 方法二：FFmpeg（最方便）

```bash
# H264 → YUV420p planar
ffmpeg -i input.h264 -pix_fmt yuv420p -f rawvideo reference.yuv

# 验证分辨率
ffprobe -i input.h264 -show_streams | grep -E "width|height|pix_fmt"

# 逐帧解码到独立 YUV 文件（每帧一个文件，方便调试）
ffmpeg -i input.h264 -pix_fmt yuv420p frame_%04d.yuv
```

### 方法三：从原始 YUV 生成码流再解码验证（闭环）

```bash
# 1. 生成已知内容的 YUV（如纯色、渐变）
python3 generate_test_yuv.py   # → test.yuv

# 2. 编码成 H264
x264 --profile baseline --input-res 352x288 -o test.h264 test.yuv

# 3. 解码回 YUV（参考输出）
ffmpeg -i test.h264 -pix_fmt yuv420p -f rawvideo decoded_ref.yuv

# 4. 解码器输出 = chip_output.yuv
# 5. 对比: diff reference.yuv chip_output.yuv（或者用 PSNR）
```

## 2.3 重点：编码-解码循环的损失

⚠️ **注意**：H264 是有损编码！即使原始 YUV → 编码 → 解码，输出 != 输入原始。
这就是为什么需要用**同一个码流、标准解码器**生成的 YUV 作为参考。

```
原始 YUV ──编码──→ .h264 ──参考解码(JM/FFmpeg)──→ 参考 YUV ✓
                        ──芯片解码──────────────→ 芯片 YUV   ← 和参考 YUV 比
```

## 2.4 简单的自检码流

生成一段已知内容的测试 YUV，覆盖边界值：

```python
# generate_known_yuv.py
import struct

W, H, frames = 64, 64, 5
with open("known.yuv", "wb") as f:
    for fid in range(frames):
        for y in range(H):
            for x in range(W):
                # Y: 按帧递增的渐变，便于识别帧序
                val = (fid * 40 + x + y) & 0xFF
                f.write(bytes([val]))
        # UV: 灰色 (128)
        for _ in range(W//2 * H//2):
            f.write(bytes([128]))
        for _ in range(W//2 * H//2):
            f.write(bytes([128]))
```

然后 x264 编码 → 芯片解码 → 对比。

## 2.5 快速 YUV 查看工具

```bash
# ffplay 直接看
ffplay -f rawvideo -pixel_format yuv420p -video_size 352x288 output.yuv

# 转 PNG 逐帧查看
ffmpeg -f rawvideo -pix_fmt yuv420p -s 352x288 -i output.yuv -vframes 10 frame_%03d.png
```
