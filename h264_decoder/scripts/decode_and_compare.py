#!/usr/bin/env python3
"""
H264 解码 → YUV → 二进制文本，然后和原始 YUV 对比

用法:
  python decode_and_compare.py tests/streams/test_176x144_1frame.h264 176 144

流程:
  输入 .h264  →  FFmpeg 解码  →  decoded.yuv  →  decoded.dat  →  diff 原始 .txt
"""

import sys
import os
import subprocess
import difflib
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)


def find_ffmpeg():
    """找 ffmpeg.exe 在不在"""
    # 先试 PATH
    for name in ["ffmpeg", "ffmpeg.exe"]:
        try:
            r = subprocess.run([name, "-version"],
                               capture_output=True, timeout=5)
            if r.returncode == 0:
                return name
        except (FileNotFoundError, subprocess.TimeoutExpired):
            continue

    # 常见安装位置
    guesses = [
        r"C:\ffmpeg\bin\ffmpeg.exe",
        r"C:\Program Files\ffmpeg\bin\ffmpeg.exe",
        os.path.expandvars(r"%LOCALAPPDATA%\Microsoft\WinGet\Packages\Gyan.FFmpeg_*\ffmpeg.exe"),
    ]
    for g in guesses:
        if os.path.exists(g):
            return g

    return None


def decode_h264_to_yuv(h264_path, yuv_out, W, H, ffmpeg_cmd):
    """FFmpeg: H264 → YUV420 planar"""
    cmd = [
        ffmpeg_cmd, "-y",
        "-i", h264_path,
        "-pix_fmt", "yuv420p",
        "-f", "rawvideo",
        "-vf", f"scale={W}:{H}:flags=neighbor",
        yuv_out,
    ]
    print(f"  [CMD] {' '.join(cmd)}")
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    if r.returncode != 0:
        print(f"  [STDERR] {r.stderr[-500:]}")
        return False
    return True


def yuv_to_dat(yuv_path, dat_out, W, H):
    """YUV 二进制 → 十六进制文本 .dat（同 yuv_to_hex.py 逻辑）"""
    with open(yuv_path, "rb") as f:
        data = f.read()

    y_size = W * H
    uv_size = W * H // 4

    Y = data[:y_size]
    U = data[y_size : y_size + uv_size]
    V = data[y_size + uv_size : y_size + 2 * uv_size]

    with open(dat_out, "w", encoding="utf-8") as fout:
        fout.write(f"; YUV420 Planar Binary Dump (DECODED)\n")
        fout.write(f"; Source : {os.path.abspath(yuv_path)}\n")
        fout.write(f"; Size   : {len(data)} bytes\n")
        fout.write(f"; Width  : {W}\n")
        fout.write(f"; Height : {H}\n")
        fout.write(f"; Frames : {len(data) // (W*H*3//2)}\n")
        fout.write(f";\n")
        fout.write(f"; Layout:\n")
        fout.write(f";   Y  plane : offset 0x{0:08X}  size {y_size}  ({W}x{H})\n")
        fout.write(f";   U  plane : offset 0x{y_size:08X}  size {uv_size}  ({W//2}x{H//2})\n")
        fout.write(f";   V  plane : offset 0x{y_size+uv_size:08X}  size {uv_size}  ({W//2}x{H//2})\n")
        fout.write(f"; ============================================================\n")

        def dump_plane(label, plane_data, pw, ph, start_offset):
            fout.write(f"\n; --- {label} Plane ({pw}x{ph}) ---\n\n")
            for row in range(ph):
                row_offset = start_offset + row * pw
                for col in range(0, pw, 16):
                    offset = row_offset + col
                    chunk = plane_data[row * pw + col : row * pw + col + 16]
                    hex_str = " ".join(f"{b:02X}" for b in chunk)
                    ascii_str = "".join(chr(b) if 32 <= b <= 126 else "." for b in chunk)
                    fout.write(f"{offset:08X}: {hex_str:<48s} |{ascii_str}|\n")

        dump_plane("Y",  Y,  W,     H,     0)
        dump_plane("U",  U,  W//2,  H//2,  y_size)
        dump_plane("V",  V,  W//2,  H//2,  y_size + uv_size)


def compare_dat(orig_dat, decoded_dat):
    """逐行对比两个 .dat，输出差异"""
    with open(orig_dat, "r", encoding="utf-8") as f:
        lines_a = f.readlines()
    with open(decoded_dat, "r", encoding="utf-8") as f:
        lines_b = f.readlines()

    # 只对比数据行（以 8 位十六进制地址开头的行）
    data_a = [l for l in lines_a if len(l) > 8 and l[8] == ":"]
    data_b = [l for l in lines_b if len(l) > 8 and l[8] == ":"]

    diffs = []
    max_lines = max(len(data_a), len(data_b))

    for i in range(max_lines):
        a = data_a[i].strip() if i < len(data_a) else "<缺失行>"
        b = data_b[i].strip() if i < len(data_b) else "<缺失行>"
        if a != b:
            diffs.append((i, a, b))

    return diffs, len(data_a), len(data_b)


def pause():
    try:
        input("按 Enter 键退出...")
    except (EOFError, OSError):
        pass


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(__doc__)
        pause()
        sys.exit(1)

    h264_path = os.path.abspath(sys.argv[1])
    W = int(sys.argv[2])
    H = int(sys.argv[3])

    if not os.path.exists(h264_path):
        print(f"[ERROR] 文件不存在: {h264_path}")
        pause()
        sys.exit(1)

    # 找 ffmpeg
    ffmpeg = find_ffmpeg()
    if not ffmpeg:
        print("[ERROR] 找不到 ffmpeg!")
        print("  请安装 FFmpeg: winget install ffmpeg")
        print("  或放在 C:\\ffmpeg\\bin\\ffmpeg.exe")
        pause()
        sys.exit(1)
    print(f"[OK] ffmpeg: {ffmpeg}")

    # 路径
    base = os.path.splitext(h264_path)[0]
    decoded_yuv = base + "_decoded.yuv"
    decoded_dat = base + "_decoded.txt"

    # 原始文件
    orig_yuv = base + ".yuv"
    orig_dat = base + ".txt"

    # 步骤1: 解码
    print(f"\n[1/3] 解码 H264 → YUV...")
    print(f"  输入:  {h264_path}")
    print(f"  输出:  {decoded_yuv}")
    ok = decode_h264_to_yuv(h264_path, decoded_yuv, W, H, ffmpeg)
    if not ok:
        print("[ERROR] FFmpeg 解码失败")
        pause()
        sys.exit(1)
    dec_size = os.path.getsize(decoded_yuv)
    exp_size = W * H * 3 // 2
    print(f"  [OK] 解码完成, 大小: {dec_size} bytes (预期 {exp_size})")
    if dec_size != exp_size:
        print(f"  [WARN] 大小不匹配! 解码输出可能有额外帧")

    # 步骤2: 转二进制文本
    print(f"\n[2/3] YUV → 十六进制文本...")
    print(f"  输出:  {decoded_dat}")
    yuv_to_dat(decoded_yuv, decoded_dat, W, H)
    print(f"  [OK] 已生成 {decoded_dat}")

    # 步骤3: 对比
    print(f"\n[3/3] 对比原始 YUV 和 解码 YUV...")
    print(f"  原始: {orig_dat}")
    print(f"  解码: {decoded_dat}")

    if not os.path.exists(orig_dat):
        # 原始 .dat 不存在，先生成
        print(f"  [INFO] 原始 .dat 不存在, 先生成...")
        if os.path.exists(orig_yuv):
            yuv_to_dat(orig_yuv, orig_dat, W, H)
            print(f"  [OK] 已生成 {orig_dat}")
        else:
            print(f"  [ERROR] 原始 YUV 也不存在: {orig_yuv}")
            pause()
            sys.exit(1)

    diffs, cnt_a, cnt_b = compare_dat(orig_dat, decoded_dat)

    print(f"\n{'='*60}")
    print(f"  对比结果")
    print(f"{'='*60}")
    print(f"  原始数据行: {cnt_a}")
    print(f"  解码数据行: {cnt_b}")
    print(f"  差异行数:   {len(diffs)}")

    if diffs:
        print(f"\n  --- 差异详情 (前 30 条) ---")
        for idx, (i, a, b) in enumerate(diffs[:30]):
            print(f"\n  差异 #{idx+1} (第 {i} 个数据行):")
            print(f"    原始: {a[:100]}")
            print(f"    解码: {b[:100]}")
        if len(diffs) > 30:
            print(f"\n  ... 共 {len(diffs)} 条差异, 只显示前 30 条")
        print(f"\n  [WARN] 解码输出与原始不一致 — 正常, H.264 是有损编码")

        # 统计第一个差异在哪个平面
        first_line = data_a[diffs[0][0]] if diffs else ""
        print(f"\n  第一个差异:")
        print(f"    行内容: {data_a[diffs[0][0]].strip()}")
        print(f"    行内容: {data_b[diffs[0][0]].strip()}")
        # 从地址判断在哪个平面
        if diffs:
            addr_hex = data_a[diffs[0][0]][:8]
            addr = int(addr_hex, 16)
            y_size = W * H
            uv_size = W * H // 4
            if addr < y_size:
                plane = "Y"
            elif addr < y_size + uv_size:
                plane = "U"
            else:
                plane = "V"
            print(f"    所在平面: {plane} (offset 0x{addr:08X})")
    else:
        print(f"\n  [OK] 完全一致! (无损编码或刚好匹配)")

    print(f"\n{'='*60}")
    pause()
