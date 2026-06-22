#!/usr/bin/env python3
"""
YUV 文件 → 十六进制文本文件 (.dat)
用法:
  python yuv_to_hex.py                                  # 转换默认 test_176x144_1frame.yuv
  python yuv_to_hex.py <input.yuv> <W> <H>              # 转换指定文件
  python yuv_to_hex.py -o output.dat input.yuv 176 144  # 指定输出文件
"""

import sys
import os


def pause():
    """等待按键，兼容双击运行和命令行管道"""
    try:
        input("按 Enter 键退出...")
    except (EOFError, OSError):
        pass  # 命令行模式下忽略

DEFAULT_YUV = os.path.join(os.path.dirname(__file__),
                           "..", "tests", "streams", "test_176x144_1frame.yuv")


def yuv_to_hex(yuv_path, out_path, W, H):
    """把 YUV 文件的每个字节转成两位十六进制，写入文本文件"""

    with open(yuv_path, "rb") as fin:
        data = fin.read()

    y_size  = W * H
    uv_size = W * H // 4

    Y = data[:y_size]
    U = data[y_size : y_size + uv_size]
    V = data[y_size + uv_size : y_size + 2 * uv_size]

    with open(out_path, "w", encoding="utf-8") as fout:
        fout.write(f"; YUV420 Planar Binary Dump\n")
        fout.write(f"; Source : {os.path.abspath(yuv_path)}\n")
        fout.write(f"; Size   : {len(data)} bytes\n")
        fout.write(f"; Width  : {W}\n")
        fout.write(f"; Height : {H}\n")
        fout.write(f"; Format : YUV420 planar, 8-bit per pixel\n")
        fout.write(f"; Frames : {len(data) // (W*H*3//2)}\n")
        fout.write(f";\n")
        fout.write(f"; Layout:\n")
        fout.write(f";   Y  plane : offset 0x{0:08X}  size {y_size}  ({W}x{H})\n")
        fout.write(f";   U  plane : offset 0x{y_size:08X}  size {uv_size}  ({W//2}x{H//2})\n")
        fout.write(f";   V  plane : offset 0x{y_size+uv_size:08X}  size {uv_size}  ({W//2}x{H//2})\n")
        fout.write(f";\n")
        fout.write(f"; 每行格式: offset: <16字节的十六进制>  |<ASCII可见字符>\n")
        fout.write(f"; ============================================================\n")

        def dump_plane(label, plane_data, plane_w, plane_h, start_offset):
            fout.write(f"\n; --- {label} Plane ({plane_w}x{plane_h}) ---\n\n")
            rows = plane_h
            cols = plane_w
            for row in range(rows):
                row_offset = start_offset + row * cols
                # 每行分多个 16 字节组
                for col in range(0, cols, 16):
                    offset = row_offset + col
                    chunk = plane_data[row * cols + col : row * cols + col + 16]
                    hex_str = " ".join(f"{b:02X}" for b in chunk)
                    ascii_str = "".join(chr(b) if 32 <= b <= 126 else "." for b in chunk)
                    fout.write(f"{offset:08X}: {hex_str:<48s} |{ascii_str}|\n")

        dump_plane("Y", Y, W, H, 0)
        dump_plane("U", U, W // 2, H // 2, y_size)
        dump_plane("V", V, W // 2, H // 2, y_size + uv_size)

    print(f"[OK] 已生成: {out_path}")
    print(f"     文件大小: {os.path.getsize(out_path)} bytes")
    print(f"     包含 {W}x{H} YUV420 的完整十六进制 dump")


def yuv_to_binary_text(yuv_path, out_path):
    """把 YUV 文件的每个字节转成 8 位二进制文本 (01010101...)"""

    with open(yuv_path, "rb") as fin:
        data = fin.read()

    with open(out_path, "w", encoding="utf-8") as fout:
        fout.write(f"; YUV420 Binary Bits Dump\n")
        fout.write(f"; Source: {os.path.abspath(yuv_path)}\n")
        fout.write(f"; ============================================================\n\n")

        for i in range(0, len(data), 16):
            offset = i
            chunk = data[i:i+16]

            # 二进制表示: 每个字节 8 位
            bin_str = ""
            for b in chunk:
                bin_str += f"{b:08b} "

            hex_str = " ".join(f"{b:02X}" for b in chunk)
            dec_str = " ".join(f"{b:3d}" for b in chunk)

            fout.write(f"@{offset:08X}\n")
            fout.write(f"  HEX: {hex_str}\n")
            fout.write(f"  DEC: {dec_str}\n")
            fout.write(f"  BIN: {bin_str}\n\n")

    print(f"[OK] 已生成: {out_path}")


if __name__ == "__main__":
    if "-h" in sys.argv or "--help" in sys.argv:
        print(__doc__)
        print()
        pause()
        sys.exit(0)

    # 解析参数
    out_path = None
    args = sys.argv[1:]

    if "-o" in args:
        idx = args.index("-o")
        out_path = args[idx + 1]
        args.pop(idx)       # 移除 -o
        args.pop(idx)       # 移除输出路径
    elif "--bin" in args:
        args.remove("--bin")
        binary_mode = True
    else:
        binary_mode = False

    # 默认输出：和 yuv 同名，后缀改为 .txt
    if not out_path:
        yuv_path = args[0] if args else DEFAULT_YUV
        base = os.path.splitext(os.path.abspath(yuv_path))[0]
        out_path = base + ".txt"

    yuv_path = args[0] if args else DEFAULT_YUV
    yuv_path = os.path.abspath(yuv_path)

    if not os.path.exists(yuv_path):
        print(f"[ERROR] 文件不存在: {yuv_path}")
        print()
        pause()
        sys.exit(1)

    W = int(args[1]) if len(args) > 1 else 176
    H = int(args[2]) if len(args) > 2 else 144

    yuv_to_hex(yuv_path, out_path, W, H)
    print()
    pause()
