#!/usr/bin/env python3
"""
YUV 对比工具 — 对比芯片输出和参考 YUV

用法:
  python yuv_compare.py chip.yuv ref.yuv 352 288
  python yuv_compare.py chip.yuv ref.yuv 1920 1080 --tolerance 2
"""

import math
import sys
import argparse
from collections import Counter


def to_db(mse):
    return 99.99 if mse == 0 else 10 * math.log10(255 * 255 / mse)


def compare_yuv(chip_path, ref_path, width, height, tolerance=0, verbose=False):
    frame_size_y = width * height
    frame_size_uv = (width // 2) * (height // 2)
    frame_size = frame_size_y + 2 * frame_size_uv

    chip_data = open(chip_path, "rb").read()
    ref_data = open(ref_path, "rb").read()

    chip_frames = len(chip_data) // frame_size
    ref_frames = len(ref_data) // frame_size

    if chip_frames != ref_frames:
        print(f"⚠️  帧数不一致: chip={chip_frames}, ref={ref_frames}")

    num_frames = min(chip_frames, ref_frames)
    total_errors = 0
    worst_psnr_y = 999
    worst_psnr_frame = 0

    print(f"\n{'='*60}")
    print(f"  YUV 对比: {width}×{height}, {num_frames} 帧")
    print(f"  容忍像素差: {tolerance}")
    print(f"{'='*60}\n")

    psnr_ys = []
    psnr_us = []
    psnr_vs = []

    for fid in range(num_frames):
        off = fid * frame_size
        frame_chip = chip_data[off : off + frame_size]
        frame_ref  = ref_data[off  : off + frame_size]

        # Y 平面
        y_off = 0
        ssd_y = 0
        err_y = 0
        first_err = None
        for i in range(frame_size_y):
            diff = frame_chip[y_off + i] - frame_ref[y_off + i]
            if abs(diff) > tolerance:
                ssd_y += diff * diff
                err_y += 1
                if first_err is None:
                    first_err = (i // width, i % width, frame_chip[i], frame_ref[i])
        mse_y = ssd_y / frame_size_y if err_y > 0 else 0

        # U 平面
        u_off = frame_size_y
        ssd_u = 0
        err_u = 0
        for i in range(frame_size_uv):
            diff = frame_chip[u_off + i] - frame_ref[u_off + i]
            if abs(diff) > tolerance:
                ssd_u += diff * diff
                err_u += 1
        mse_u = ssd_u / frame_size_uv if err_u > 0 else 0

        # V 平面
        v_off = frame_size_y + frame_size_uv
        ssd_v = 0
        err_v = 0
        for i in range(frame_size_uv):
            diff = frame_chip[v_off + i] - frame_ref[v_off + i]
            if abs(diff) > tolerance:
                ssd_v += diff * diff
                err_v += 1
        mse_v = ssd_v / frame_size_uv if err_v > 0 else 0

        psy = to_db(mse_y)
        psu = to_db(mse_u)
        psv = to_db(mse_v)

        psnr_ys.append(psy)
        psnr_us.append(psu)
        psnr_vs.append(psv)

        frame_errors = err_y + err_u + err_v
        total_errors += frame_errors

        if psy < worst_psnr_y:
            worst_psnr_y = psy
            worst_psnr_frame = fid

        status = "✅" if frame_errors == 0 else ("⚠️ " if psy > 40 else "❌")
        print(f"  [{status}] 帧 {fid:4d}  "
              f"Y={psy:6.2f}  U={psu:6.2f}  V={psv:6.2f}  "
              f"错误像素={frame_errors}")

        if verbose and first_err:
            py, px, cv, rv = first_err
            print(f"        首个错误: Y[{py}][{px}] chip={cv} ref={rv}")

    avg_psnr_y = sum(psnr_ys) / len(psnr_ys)
    avg_psnr_u = sum(psnr_us) / len(psnr_us)
    avg_psnr_v = sum(psnr_vs) / len(psnr_vs)

    print(f"\n{'='*60}")
    print(f"  结果汇总:")
    print(f"    平均 PSNR: Y={avg_psnr_y:.2f} U={avg_psnr_u:.2f} V={avg_psnr_v:.2f} dB")
    print(f"    最差帧:    #{worst_psnr_frame} (Y={worst_psnr_y:.2f} dB)")
    print(f"    总像素错误: {total_errors}")
    print(f"    总像素数:    {num_frames * frame_size}")
    print(f"    错误率:      {total_errors / (num_frames * frame_size) * 100:.4f}%")
    print(f"{'='*60}")

    # 判定
    if total_errors == 0:
        print("\n🎉 完美 — 逐像素完全一致!")
        return True
    elif avg_psnr_y > 50:
        print("\n✅ PASS — 像素级一致 (PSNR > 50 dB)")
        return True
    elif avg_psnr_y > 40:
        print("\n⚠️  接近 — 可能有微小舍入差异")
        return False
    else:
        print("\n❌ FAIL — 解码输出明显有问题")
        return False


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="H264 解码器 YUV 输出对比工具")
    parser.add_argument("chip_yuv", help="芯片解码输出的 YUV 文件")
    parser.add_argument("ref_yuv", help="参考解码器输出的 YUV 文件")
    parser.add_argument("width", type=int, help="图像宽度")
    parser.add_argument("height", type=int, help="图像高度")
    parser.add_argument("--tolerance", "-t", type=int, default=0,
                        help="允许的像素差值 (默认 0=逐位精确)")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="显示首个错误像素位置")
    parser.add_argument("--frame", "-f", type=int, default=None,
                        help="只比较指定帧")
    args = parser.parse_args()

    # 如果只比较单帧，提取出来
    if args.frame is not None:
        raise NotImplementedError("单帧模式待实现")

    ok = compare_yuv(
        args.chip_yuv, args.ref_yuv,
        args.width, args.height,
        tolerance=args.tolerance,
        verbose=args.verbose,
    )

    sys.exit(0 if ok else 1)
