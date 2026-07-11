# check_parse.py — 检查 tree-sitter-verilog 解析质量
# 用法: python check_parse.py <RTL目录> [--out <输出文件>]
import sys
from pathlib import Path
from datetime import datetime
import tree_sitter_language_pack as tslp

parser = tslp.get_parser("verilog")

def check_file(path):
    source = Path(path).read_bytes()
    tree = parser.parse(source)

    errors = []
    error_nodes = 0

    def count(node):
        nonlocal error_nodes
        if node.type == "ERROR":
            error_nodes += 1
            if len(errors) <= 5:
                line = source[:node.start_byte].count(b"\n") + 1
                snippet = node.text.decode("utf-8", "replace")[:60].replace("\n", "\\n")
                errors.append((line, snippet))
        for child in node.children:
            count(child)

    count(tree.root_node)
    total = sum(1 for _ in _walk(tree.root_node))

    return {
        "name": path.name,
        "errors": error_nodes,
        "total_nodes": total,
        "error_rate": round(100 * error_nodes / max(1, total), 2),
        "error_samples": errors,
    }


def _walk(node):
    yield node
    for child in node.children:
        yield from _walk(child)


def generate_report(target_dir, output_path=None):
    """扫描目录下所有 .sv/.v 文件，生成解析质量报告。"""
    target = Path(target_dir)
    files = sorted(list(target.rglob("*.sv")) + list(target.rglob("*.v")))

    if not files:
        return None, "未找到 .sv 或 .v 文件"

    results = []
    for f in files:
        try:
            results.append(check_file(f))
        except Exception as e:
            results.append({
                "name": f.name,
                "errors": -1,
                "total_nodes": 0,
                "error_rate": -1,
                "error_samples": [("?", str(e))],
            })

    # 分级统计
    ok = [r for r in results if r["error_rate"] >= 0 and r["error_rate"] < 0.5]
    warn = [r for r in results if 0.5 <= r["error_rate"] < 5.0]
    bad = [r for r in results if r["error_rate"] >= 5.0]
    crashed = [r for r in results if r["error_rate"] < 0]

    lines = []
    lines.append("=" * 70)
    lines.append(f"  Tree-Sitter-Verilog 解析质量报告")
    lines.append(f"  生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(f"  扫描目录: {target_dir}")
    lines.append(f"  文件总数: {len(files)}")
    lines.append("=" * 70)
    lines.append("")

    # 逐文件明细
    lines.append("-" * 70)
    lines.append(f"  逐文件明细")
    lines.append("-" * 70)
    for r in results:
        if r["error_rate"] < 0:
            lines.append(f"  💥 {r['name']}: 解析崩溃 — {r['error_samples'][0][1]}")
        elif r["error_rate"] < 0.5:
            lines.append(f"  ✅ {r['name']}: {r['errors']} ERR / {r['total_nodes']} nodes ({r['error_rate']:.1f}%)")
        elif r["error_rate"] < 5.0:
            lines.append(f"  ⚠️  {r['name']}: {r['errors']} ERR / {r['total_nodes']} nodes ({r['error_rate']:.1f}%)")
        else:
            lines.append(f"  ❌ {r['name']}: {r['errors']} ERR / {r['total_nodes']} nodes ({r['error_rate']:.1f}%)")
        for lineno, snippet in r.get("error_samples", []):
            lines.append(f"       L{lineno}: '{snippet}'")
    lines.append("")

    # 侧写
    lines.append("-" * 70)
    lines.append(f"  侧写: ✅ OK (<0.5%) · ⚠️ WARN (0.5-5%) · ❌ BAD (>5%)")
    lines.append("-" * 70)

    for label, group in [("✅ 解析正常", ok), ("⚠️ 部分实例化可能遗漏", warn), ("❌ 连线大量缺失", bad), ("💥 崩溃", crashed)]:
        if group:
            pct = round(100 * len(group) / len(files), 1)
            lines.append(f"  {label}: {len(group)} 个文件 ({pct}%)")
            if label.startswith("⚠️") or label.startswith("❌"):
                for r in group:
                    lines.append(f"    - {r['name']} ({r['error_rate']:.1f}%)")

    lines.append("")

    # 总结建议
    lines.append("-" * 70)
    lines.append(f"  总结建议")
    lines.append("-" * 70)

    overall_ok_pct = round(100 * len(ok) / len(files), 1)
    lines.append(f"  整体解析正常率: {len(ok)}/{len(files)} = {overall_ok_pct}%")

    if len(bad) > 0 or crashed:
        lines.append(f"  ⚠️  call graph 可能不可靠 — 建议 review 以上 ❌ 文件后重试")
        lines.append(f"  💡 提示: 红色文件通常使用了 tree-sitter-verilog 不支持的")
        lines.append(f"          SystemVerilog 高级语法 (logic type, interface, 增强参数化等)")
    elif len(warn) > 0:
        lines.append(f"  ✅ call graph 基本可信 — 部分 SystemVerilog 文件有轻微解析问题")
        lines.append(f"  💡 可以通过 code-review-graph visualize 生成图后,")
        lines.append(f"     检查孤立节点是否为实际被实例化的模块")
    else:
        lines.append(f"  ✅ 所有文件解析正常，call graph 可信")

    lines.append("")

    report = "\n".join(lines)

    # 输出到文件
    if output_path is None:
        output_path = target / ".code-review-graph" / "parse_report.txt"
    Path(output_path).parent.mkdir(parents=True, exist_ok=True)
    Path(output_path).write_text(report, encoding="utf-8")
    print(report)

    # 返回同级数给调用方
    return results, report


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("用法: python check_parse.py <RTL目录>")
        print("  python check_parse.py E:\\my_project\\rtl")
        sys.exit(1)

    target_dir = sys.argv[1]
    results, report = generate_report(target_dir)
    if results is None:
        print(report)
        sys.exit(2)

    ok_count = sum(1 for r in results if 0 <= r["error_rate"] < 0.5)
    total = len(results)
    if ok_count == total:
        print(f"\n✅ 全部 {total} 个文件解析正常")
    else:
        print(f"\n报告保存在: {Path(target_dir) / '.code-review-graph' / 'parse_report.txt'}")
        sys.exit(1 if ok_count < total * 0.5 else 0)
