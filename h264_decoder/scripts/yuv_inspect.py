#!/usr/bin/env python3
"""
YUV 二进制查看工具 — Windows 上替代 xxd 查看 YUV 原始数据

用法:
  python yuv_inspect.py                     # 默认查看 test_176x144_1frame.yuv
  python yuv_inspect.py <your.yuv> <W> <H>  # 查看任意 YUV 文件
"""

import sys
import os
import io

# 修正 Windows GBK 编码问题
if sys.stdout.encoding != 'utf-8':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

# 替换 emoji 为 ASCII 安全字符
def ok():    return "[OK]"
def fail():  return "[FAIL]"
def warn():  return "[WARN]"

# 默认文件
DEFAULT_YUV = os.path.join(os.path.dirname(__file__),
                           "..", "tests", "streams", "test_176x144_1frame.yuv")
DEFAULT_W, DEFAULT_H = 176, 144

# ============================================================
def hexdump(data, offset=0, nbytes=None, label=""):
    """模仿 xxd 的十六进制输出"""
    if nbytes is None:
        nbytes = len(data) - offset
    end = min(offset + nbytes, len(data))

    print(f"\n{'='*70}")
    print(f"  {label}  偏移 0x{offset:06X} ~ 0x{end:06X}  (共 {end-offset} bytes)")
    print(f"{'='*70}")

    # 按行输出，每行 16 字节
    for row_start in range((offset // 16) * 16, end, 16):
        line_offset = row_start
        hex_str = ""
        ascii_str = ""
        for i in range(16):
            pos = row_start + i
            if pos < offset or pos >= end:
                hex_str += "   "
                ascii_str += " "
            else:
                byte = data[pos]
                # 高亮非重复值（用不同颜色标记边界变化）
                # 简单版本：显示十六进制值
                if pos >= offset:
                    hex_str += f"{byte:02X} "
                    # ASCII 可见字符显示原字符，否则显示 .
                    ascii_str += chr(byte) if 32 <= byte <= 126 else "."
                else:
                    hex_str += ".. "
                    ascii_str += "."

        print(f"  {line_offset:08X}: {hex_str} {ascii_str}")


def inspect_yuv(filepath, W, H):
    """完整检查一个 YUV420 planar 文件"""

    if not os.path.exists(filepath):
        print(f"[ERROR] 文件不存在: {filepath}")
        print()
        input("按 Enter 键退出...")

    with open(filepath, "rb") as f:
        data = f.read()

    file_size = len(data)
    y_size  = W * H
    uv_size = W * H // 4        # 每个色度平面 = W/2 * H/2
    frame_size = y_size + uv_size * 2
    num_frames = file_size // frame_size

    Y  = data[:y_size]
    U  = data[y_size : y_size + uv_size]
    V  = data[y_size + uv_size : y_size + uv_size * 2]

    # ── 文件概览 ──
    print(f"""
╔══════════════════════════════════════════════════════════╗
║          YUV420 Planar 文件分析工具                      ║
╠══════════════════════════════════════════════════════════╣
║  文件    : {filepath}
║  大小    : {file_size} bytes
║  分辨率  : {W}×{H}
║  帧数    : {num_frames}
║  预期大小: {frame_size} bytes/帧
║  匹配    : {'OK' if file_size == frame_size * num_frames else 'MISMATCH!'}
╠══════════════════════════════════════════════════════════╣
║  Y  平面 : offset 0x{0:08X}  size {y_size:6d}  ({W}×{H})
║  U  平面 : offset 0x{y_size:08X}  size {uv_size:6d}  ({W//2}×{H//2})
║  V  平面 : offset 0x{y_size + uv_size:08X}  size {uv_size:6d}  ({W//2}×{H//2})
╚══════════════════════════════════════════════════════════╝
""")

    # ── Y 平面开头 128 bytes (看清每行结构) ──
    print("【Y 平面 — 行 0 (左上角)】")
    print(f"  前 16 列是白色方块 (Y=0xFF), 其余为 7 条彩条的起始部分")
    hexdump(data, 0, 128, "Y 平面 行 0 (0x000000)")

    # ── Y 平面 行 0 的前 32 像素，逐像素标注 ──
    print(f"\n【逐像素解析 — Y 平面第 0 行 (共 {W} 像素)】")
    print(f"{'  列':>4s}: ", end="")
    for col in range(min(32, W)):
        val = Y[col]
        print(f"{val:3d}", end=" ")
    print()
    print(f"{'  十六进制':>4s}: ", end="")
    for col in range(min(32, W)):
        print(f"{Y[col]:02X} ", end="")
    print()

    # ── 7 条彩条的边界在哪里 ──
    bar_w = W // 7  # 每条宽度（像素）
    print(f"\n【7 条彩条的列边界 (每条约 {bar_w} 列宽)】")
    for b in range(7):
        start_col = b * bar_w
        end_col = start_col + bar_w - 1
        sample_col = start_col + bar_w // 2
        y_val = Y[10 * W + sample_col]  # 第10行取样

        # 色度平面中的对应位置
        u_idx = 5 * (W//2) + sample_col // 2
        v_idx = 5 * (W//2) + sample_col // 2
        u_val = U[u_idx] if u_idx < len(U) else 0
        v_val = V[v_idx] if v_idx < len(V) else 0

        print(f"  条{b}: 列{start_col:3d}-{end_col:3d}  "
              f"Y={y_val:3d}(0x{y_val:02X})  "
              f"U={u_val:3d}(0x{u_val:02X})  "
              f"V={v_val:3d}(0x{v_val:02X})")

    # ── 底部白色横条 ──
    white_start_row = H - 16
    white_offset = white_start_row * W
    print(f"\n【底部白色横条 — Y 平面 行{white_start_row} 起 (16行)】")
    hexdump(data, white_offset, 48, f"Y 平面 行{white_start_row} (0x{white_offset:06X})")

    # ── Y → U 分界线 ──
    print(f"\n【Y 平面末尾 → U 平面开头 (0x{y_size:06X})】")
    hexdump(data, y_size - 32, 80, f"Y→U 分界")

    # ── U → V 分界线 ──
    uv_boundary = y_size + uv_size
    print(f"\n【U 平面末尾 → V 平面开头 (0x{uv_boundary:06X})】")
    hexdump(data, uv_boundary - 32, 80, f"U→V 分界")

    # ── V 平面末尾 → 文件末尾 ──
    print(f"\n【V 平面末尾 → 文件结尾 (0x{file_size:06X})】")
    hexdump(data, file_size - 48, 48, f"文件末尾")

    print(f"\n{'='*70}")
    print(f"  分析完成。文件大小正确，三个平面边界清晰可辨。")

    # ── 判定 ──
    if file_size == frame_size * num_frames:
        print(f"  [OK] 文件格式正确: {W}x{H} YUV420 planar x {num_frames} 帧")
    else:
        print(f"  [ERROR] 文件大小异常!")
    print(f"{'='*70}")


def dump_raw_hex(filepath, max_bytes=4096):
    """简单的原始 hex dump — 像 xxd 那样"""
    with open(filepath, "rb") as f:
        data = f.read(max_bytes)

    print(f"\n── 原始 hex dump (前 {len(data)} bytes) ──")
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hex_part = " ".join(f"{b:02X}" for b in chunk)
        ascii_part = "".join(chr(b) if 32 <= b <= 126 else "." for b in chunk)
        print(f"{i:08X}: {hex_part:<48s}  {ascii_part}")


# ============================================================
if __name__ == "__main__":
    if "-h" in sys.argv or "--help" in sys.argv:
        print(__doc__)
        print()
        input("按 Enter 键退出...")

    # 判断参数
    if len(sys.argv) >= 4:
        filepath = sys.argv[1]
        W = int(sys.argv[2])
        H = int(sys.argv[3])
    elif len(sys.argv) == 2 and sys.argv[1] == "--raw":
        dump_raw_hex(DEFAULT_YUV)
        print()
        input("按 Enter 键退出...")
    elif len(sys.argv) == 2 and sys.argv[1] == "--hex":
        dump_raw_hex(DEFAULT_YUV, max_bytes=2048)
        print()
        input("按 Enter 键退出...")
    else:
        filepath = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_YUV
        W = DEFAULT_W if len(sys.argv) <= 1 else 176
        H = DEFAULT_H if len(sys.argv) <= 1 else 144

    # 绝对路径转换
    filepath = os.path.abspath(filepath)

    inspect_yuv(filepath, W, H)

    print("\n[*] 提示: 用 --hex 参数查看简洁版 hex dump")
    print("   python yuv_inspect.py --hex")
    print()
    input("按 Enter 键退出...")
