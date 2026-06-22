# 1. 获取 H264 码流文件

## 1.1 标准一致性码流（推荐首选）

ITU/ISO 官方发布的 H.264 一致性测试码流，覆盖各种 feature。

**来源：**
- **JM 参考软件包**（最新）: https://vcgit.hhi.fraunhofer.de/jvet/jm
- **历史归档**: https://github.com/Srella/JM (mirror)
- **直接下载码流**:
  ```
  wget http://trace.eas.asu.edu/h264/jm/JM.zip          # JM 参考软件
  wget ftp://hvc.usherbrooke.ca/videotest/AKIYO.zip     # 常用测试序列
  wget http://media.xiph.org/video/derf/                 # 原始 YUV 测试序列
  ```

**JM 下载解压后，码流在：**
```
JM/bin/
├── test.264           # 基础测试码流
├── test.263           # H.263
└── foreman*.264       # 多种分辨率
```

**常用标准测试序列：**

| 序列名 | 分辨率 | 帧数 | 特点 |
|--------|--------|------|------|
| foreman_qcif | 176×144 | 300 | 经典，运动+场景切换 |
| akiyo_qcif | 176×144 | 300 | 静态背景，人脸 |
| mobile_cif | 352×288 | 300 | 纹理复杂，运动 |
| bus_cif | 352×288 | 150 | 全局运动 |
| football_cif | 352×288 | 260 | 快速运动 |
| coastguard_cif | 352×288 | 300 | 全局平移 |
| stefan_sif | 352×240 | 90 | 快速运动（网球） |
| foreman_720p | 1280×720 | — | 高清测试 |
| ducks_take_off_1080p | 1920×1080 | — | 复杂纹理，高清 |

## 1.2 自己生成码流

### 用 x264 编码（推荐，参数可控）
```bash
# 先准备好原始 YUV 文件，然后
x264 \
  --preset medium \
  --profile baseline \         # baseline/main/high
  --level 3.0 \
  --fps 30 \
  --input-res 352x288 \
  --output test.h264 \
  input.yuv

# 只做 I 和 P 帧（简单验证先用这个）
x264 --profile baseline --no-cabac --bframes 0 \
     --input-res 176x144 --fps 30 -o test.h264 input.yuv
```

### 用 FFmpeg 编码
```bash
ffmpeg -f rawvideo -pix_fmt yuv420p -s 176x144 -r 30 \
       -i input.yuv -c:v libx264 -profile:v baseline \
       -level 3.0 -preset medium -bf 0 test.h264
```

### 用 JM 编码器（精确控制每项参数）
```bash
# JM encoder, 配置文件方式
./lencod -f encoder_baseline.cfg
```

## 1.3 码流文件格式说明

H264 码流有两种常见封装：

```
# 裸流 (Annex B) — 解码器通常接收这种
00 00 00 01 [NAL unit...] 00 00 00 01 [NAL unit...]

# AVCC 格式（MP4 中用）— 4字节长度前缀
[4-byte length] [NAL unit...] [4-byte length] [NAL unit...]
```

**验证时确认：你的解码器接受哪种格式？** 大多数硬件解码器接受 Annex B 裸流。

## 1.4 检查码流 NAL 类型

```bash
# 快速检查 NAL 类型
xxd test.h264 | head -50

# 或用 Python 脚本分析
python3 << 'EOF'
with open("test.h264", "rb") as f:
    data = f.read()
i = 0
while i < len(data) - 4:
    if data[i:i+4] == b'\x00\x00\x00\x01':
        nal = data[i+4]
        nal_type = nal & 0x1f
        ref = (nal >> 5) & 0x03
        print(f"offset={i}, type={nal_type}, ref={ref}, "
              f"part_of_idr={(nal>>5)&1 if nal_type==5 else '-'}")
        i += 4
    else:
        i += 1
EOF
```
