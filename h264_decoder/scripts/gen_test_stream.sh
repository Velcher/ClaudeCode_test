#!/usr/bin/env bash
# 生成 H264 验证测试码流和参考 YUV
# 需要: x264 / ffmpeg + JM 参考解码器 (或仅 FFmpeg)

set -euo pipefail

OUTDIR="${1:-./test_streams}"
WIDTH="${2:-176}"
HEIGHT="${3:-144}"
FRAMES="${4:-5}"
PROFILE="${5:-baseline}"   # baseline | main | high

mkdir -p "$OUTDIR"

echo "============================================="
echo "  生成 H264 测试码流"
echo "  分辨率: ${WIDTH}×${HEIGHT}"
echo "  帧数:   ${FRAMES}"
echo "  Profile: ${PROFILE}"
echo "  输出:   ${OUTDIR}/"
echo "============================================="
echo ""

# ── 步骤1: 生成已知原始 YUV ──────────────────
echo "[1/4] 生成测试原始 YUV..."
RAW_YUV="${OUTDIR}/raw_${WIDTH}x${HEIGHT}_${FRAMES}f.yuv"

python3 -c "
import sys
W, H = ${WIDTH}, ${HEIGHT}
fm_cnt = ${FRAMES}

with open('${RAW_YUV}', 'wb') as f:
    for fid in range(fm_cnt):
        for y in range(H):
            for x in range(W):
                # Y: 可识别图案 — 帧号相关 + 空间渐变
                val = (fid * 40 + x*2 + y) & 0xFF
                f.write(bytes([val]))
        # U, V: 固定灰色 (128)
        for _ in range(W//2 * H//2):
            f.write(bytes([128]))
            f.write(bytes([128]))
"
echo "  → ${RAW_YUV}"

# ── 步骤2: 编码成 H264 ─────────────────────
echo "[2/4] 编码 H264..."

H264_OUT="${OUTDIR}/test_${PROFILE}_${WIDTH}x${HEIGHT}_${FRAMES}f.h264"

if [ "$PROFILE" = "baseline" ]; then
    # Baseline: CAVLC, 无 B 帧, 全部 I 帧最简单
    x264 --profile baseline --no-cabac \
         --bframes 0 --keyint 1 \
         --input-res ${WIDTH}x${HEIGHT} --fps 30 \
         -o "${H264_OUT}" \
         --frames "${FRAMES}" \
         "${RAW_YUV}" 2>&1 | tail -3
elif [ "$PROFILE" = "main" ]; then
    # Main: CABAC, B 帧, I 间隔 = 30
    x264 --profile main --cabac \
         --bframes 2 --keyint 30 \
         --input-res ${WIDTH}x${HEIGHT} --fps 30 \
         -o "${H264_OUT}" \
         --frames "${FRAMES}" \
         "${RAW_YUV}" 2>&1 | tail -3
elif [ "$PROFILE" = "high" ]; then
    x264 --profile high --cabac \
         --bframes 3 --keyint 30 \
         --input-res ${WIDTH}x${HEIGHT} --fps 30 \
         -o "${H264_OUT}" \
         --frames "${FRAMES}" \
         "${RAW_YUV}" 2>&1 | tail -3
else
    echo "Unknown profile: ${PROFILE}"
    exit 1
fi

echo "  → ${H264_OUT}"
FILE_SIZE=$(stat -c%s "${H264_OUT}" 2>/dev/null || stat -f%z "${H264_OUT}")
echo "  文件大小: ${FILE_SIZE} 字节"

# ── 步骤3: 生成参考 YUV (用 FFmpeg) ────────
echo "[3/4] 生成参考解码 YUV..."

REF_YUV="${OUTDIR}/ref_${PROFILE}_${WIDTH}x${HEIGHT}_${FRAMES}f.yuv"

ffmpeg -y -i "${H264_OUT}" \
       -pix_fmt yuv420p \
       -f rawvideo \
       "${REF_YUV}" 2>/dev/null

REF_SIZE=$(stat -c%s "${REF_YUV}" 2>/dev/null || stat -f%z "${REF_YUV}")
echo "  → ${REF_YUV}"
echo "  文件大小: ${REF_SIZE} 字节"
echo "  预期大小: $(( WIDTH * HEIGHT * 3 / 2 * FRAMES )) 字节"

# ── 步骤4: 二次确认 ── 把参考 YUV 再编码一次 ─
echo "[4/4] round-trip 校验 (参考 YUV → re-encode → re-decode → 对比)..."

RE_H264="${OUTDIR}/roundtrip.h264"
RE_YUV="${OUTDIR}/roundtrip.yuv"

x264 --profile baseline --no-cabac --bframes 0 --keyint 1 \
     --input-res ${WIDTH}x${HEIGHT} --fps 30 \
     -o "${RE_H264}" --frames "${FRAMES}" "${REF_YUV}" 2>&1 | tail -1

ffmpeg -y -i "${RE_H264}" -pix_fmt yuv420p \
       -f rawvideo "${RE_YUV}" 2>/dev/null

# 简单 diff
DIFF_COUNT=$(python3 -c "
a = open('${REF_YUV}','rb').read()
b = open('${RE_YUV}','rb').read()
mn = min(len(a), len(b))
diffs = sum(1 for i in range(mn) if a[i]!=b[i])
print(diffs)
")

if [ "$DIFF_COUNT" -eq 0 ]; then
    echo "  ✅ Round-trip: 逐像素一致"
else
    echo "  ⚠️  Round-trip: ${DIFF_COUNT} 个像素差异（有损编码正常现象）"
fi

echo ""
echo "============================================="
echo "  生成完成!"
echo "  码流:    ${H264_OUT}"
echo "  参考YUV: ${REF_YUV}"
echo ""
echo "  芯片验证命令:"
echo "    # 将码流送到解码器，读出输出 YUV"
echo "    python3 ../scripts/yuv_compare.py chip_output.yuv ${REF_YUV} ${WIDTH} ${HEIGHT}"
echo "============================================="
