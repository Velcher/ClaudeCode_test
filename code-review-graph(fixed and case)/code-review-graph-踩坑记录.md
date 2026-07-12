# code-review-graph 踩坑记录

> 日期：2026-07-11
> 项目：cv32e40p、Ibex、verilog-pcie、async_fifo

---

## 1. 香山项目无法直接分析

**现象**：525 个 `.scala` 文件，0 个 Verilog 文件，`code-review-graph build` 无有效输出。

**根因**：香山是 Chisel（Scala）项目，tree-sitter 不支持 Scala/Chisel。需要先 `make verilog` 生成 `.v` 文件才能分析。

**解决**：换用纯 Verilog 项目（Ibex、cv32e40p）。

**教训**：分析前先 `find . -name "*.sv" | wc -l` 确认项目文件类型。

---

## 2. 打开 graph.html 节点全部挤在左上角

**现象**：双击 `file://` 打开 `graph.html`，所有节点显示在屏幕左上角极小区域。

**根因**：SVG 是 CSS 替换元素（replaced element）。HTML 中无显式 `width`/`height` 属性时，`file://` 协议下浏览器退回 SVG 默认 intrinsic size（300×150），导致整个力导向图被压缩。`fitGraph()` 在 300×150 的小盒子里缩放，看起来就像"左上角一小块"。

**排查方法**：
```javascript
// Console 中检查 SVG 实际渲染尺寸
svg.node().getBoundingClientRect()
// 如果返回 width/height 远小于窗口尺寸，就是 CSS 替换元素问题
```

**解决**：打开 graph.html，F12 Console 粘贴执行：
```javascript
var svgEl = svg.node();
svgEl.style.cssText = "width:100vw !important;height:100vh !important;position:fixed;top:0;left:0;margin:0;";
simulation.tick(200);
var bbox = gRoot.node().getBBox();
var b = bbox;
var pad = 0.05;
var w = b.width * (1 + 2 * pad);
var h = b.height * (1 + 2 * pad);
var scale = Math.min(window.innerWidth / w, window.innerHeight / h);
scale = Math.max(scale, 0.9);
var tx = window.innerWidth / 2 - (b.x + b.width / 2) * scale;
var ty = window.innerHeight / 2 - (b.y + b.height / 2) * scale;
svg.transition().duration(500).call(zoomBehavior.transform, d3.zoomIdentity.translate(tx, ty).scale(scale));
console.log("Done. Scale:", scale, "BBox:", b.width, b.height);
```

**尝试过的无效方案**（回顾用）：
- `getBBox()` + `setTimeout` 重试 → 无效
- 数据坐标替代 `getBBox` → 方向对但没解决容器尺寸问题
- 自动折叠阈值 2000→300 → 无关
- `svg { position: absolute }` → 节点叠在一起

**教训**：可视化问题先用 `getBoundingClientRect()` 查容器尺寸，再修 transform。

---

## 3. `python3 -m code_review_graph.cli build` 静默无输出

**根因**：`cli.py` 没有 `if __name__ == "__main__": main()` 入口。`-m` 方式只导入模块不执行 `main()`。

**解决**：pip 安装后直接使用 `code-review-graph` 短命令（pip 自动生成 `.exe` 入口）。

---

## 4. cv32e40p 出现孤立节点（部分已修复）

**现象**：图中 6 个模块（`cv32e40p_controller`、`cv32e40p_aligner`、`cv32e40p_decoder`、`cv32e40p_hwloop_regs`、`cv32e40p_int_controller`、`cv32e40p_apu_disp`）没有任何 CALLS 边。

**排查**：`grep -rn "模块名" *.sv` 确认它们在 `id_stage.sv` 和 `ex_stage.sv` 中确实被实例化了。

**根因分两类**：

| 文件 | 问题 | 状态 |
|---|---|---|
| `id_stage.sv`（5 个模块） | tree-sitter-verilog 解析失败，783 个 ERROR（4.2%），吞了全部 `module_instantiation` 节点 | ❌ 无法修复——tree-sitter-verilog 不支持该文件的 SystemVerilog 高级语法 |
| `ex_stage.sv`（`apu_disp`） | 无 `#()` 参数的实例化被 tree-sitter-verilog 解析为 `interface_instantiation`，code-review-graph 原本只认 `module_instantiation` | ✅ 已修复 |

**修复 `interface_instantiation` 的代码改动**（`parser.py` 两处）：
```python
# ① _CALL_TYPES 新增 interface_instantiation
"verilog": ["module_instantiation", "interface_instantiation", ...]

# ② 名字提取同时接受两种 identifier
if first.type in ("simple_identifier", "interface_identifier"):
    return first.text.decode("utf-8", errors="replace")
```

**教训**：tree-sitter-verilog 的 AST 节点类型在不同 SV 语法下有歧义。排查步骤：
1. `grep -rn "模块名" *.sv` 确认真实实例化关系
2. 用 tree-sitter 裸跑该文件看 AST 节点类型
3. 检查 code-review-graph 的 `_CALL_TYPES` 和名字提取逻辑是否覆盖了该节点类型

---

## 5. tree-sitter-verilog 的根本局限

**核心问题**：tree-sitter-verilog 本质是 **Verilog 解析器**，不是 SystemVerilog 解析器。

**不支持的 SV 特性**（会导致 ERROR 节点 → 实例化关系丢失）：
- `logic` 类型替代 `wire`/`reg`
- `always_ff` / `always_comb` 替代 `always @(...)`
- `interface` / `modport`
- 增强的参数化语法

**解决方案**：写了解析质量检查脚本（`check_parse.py` → `parse_check.py`），集成到 `code-review-graph build` 自动运行。

**判断标准**：

| ERROR 率 | 图标 | 含义 |
|---|---|---|
| < 0.5% | ✅ | call graph 可信 |
| 0.5% - 5% | ⚠️ | 部分模块实例化可能遗漏 |
| > 5% | ❌ | 图不可靠，禁止用来做审查判断 |

**对自己项目的前置检查**：
```bash
# build 时自动运行，报告在：
<项目>\.code-review-graph\parse_report.txt
```

**教训**：手写传统 Verilog（`wire`/`reg`、`always @(posedge clk)`、简单参数化）应全部正常；用了大量 SV 特性的项目（如 cv32e40p `id_stage.sv`）会有解析遗漏。

---

## 6. 非 Verilog 文件混入图谱

**现象**：`build` 时解析了 CI 脚本、Python、Shell、YAML 等 27 个非 Verilog 文件。

**解决**：
```bash
git init
git add rtl/        # 只跟踪 RTL 目录
git commit -m "..."
code-review-graph build
```

**注意**：code-review-graph 默认忽略 `vendor/**` 目录。如果需要包含第三方 IP，创建空的 `.code-review-graphignore` 覆盖。

---

## 7. 离线部署

**流程**：
```bash
# 外网机器：下载所有依赖
python3 -m pip download -d offline_pkgs code-review-graph

# 内网机器：离线安装（前提：Python 版本一致）
python -m pip install --no-index --find-links=<U盘>\offline_pkgs code-review-graph
```

**关键点**：
- 外网和内网 Python 版本必须一致（否则二进制 `.whl` 不兼容）
- 共下载 80 个 `.whl`，约 40MB
- 安装后直接可用 `code-review-graph build` 短命令

---

## 完整工作流（总结）

```bash
# 1. 前置条件
git init && git add rtl/ && git commit -m "init"

# 2. 构建 + 自动解析质量检查（一条命令）
code-review-graph build

# 3. 查看解析质量报告
cat .code-review-graph/parse_report.txt
# 确认 ✅ 文件占比 > 95% 再继续

# 4. 生成可视化
code-review-graph visualize

# 5. 打开 graph.html，如节点在左上角则粘贴 Console 脚本（见第 2 节）
```

## 核心文件索引

| 文件 | 位置 | 作用 |
|---|---|---|
| `parse_check.py` | `code-review-graph-main/code_review_graph/` | 解析质控检查，`build` 时自动调用 |
| `check_parse.py` | 本压缩包根目录 | 独立版检查脚本 |
| `sync_crg.py` | 本压缩包根目录 | 同步修复版源码到 site-packages，已自动检测路径 |
| `offline_pkgs/` | 本压缩包根目录 | 离线部署包（80 个 .whl + README） |
| `code-review-graph-build.bat` | 本压缩包根目录 | 先检查解析质量再构建的 wrapper |

---

## 8. `pcie_s10_if.v` 解析失败（6% ERROR）：`#(` 后跟注释导致模块声明丢失

**现象**：`pcie_s10_if` 在图中没有 Class 节点，`pcie_s10_msi` 被它实例化了但图中无 CALLS 边。

**排查**：`parse_report.txt` 显示 `pcie_s10_if.v` 6.0% ERROR（❌），对比同项目 `pcie_s10_if_tx.v` 仅 0.2% ERROR（✅）。

**根因**：tree-sitter-verilog 不支持 `#(` 和第一个 `parameter` 之间插入注释：

```verilog
// ❌ tree-sitter 炸了（6% ERROR）
module pcie_s10_if #
(
    // H-Tile/L-Tile AVST segment count    ← 这个注释导致解析失败
    parameter SEG_COUNT = 1,
    ...

// ✅ 解析正常（0.2% ERROR）
module pcie_s10_if_tx #
(
    parameter SEG_COUNT = 1,               ← 没注释
    ...
```

不是 `module #(params) (ports)` 这种写法本身有问题——同项目的 `pcie_s10_if_tx.v` 和 Ibex 全部正常。只是 tree-sitter-verilog 的语法规则不认 `#(` 后紧跟注释这种写法。

**教训**：
- 手写 Verilog 时，`#(` 和第一个 `parameter` 之间不要插注释
- 遇到 ERROR 率超过 5% 的文件，优先检查模块声明处的特殊写法
- 这个限制不影响传统 Verilog-2001 的无参数模块（`module my_mod (...)` 不会有这个问题）

---

## 9. graph.html 打开一直 "Laying out graph..." / D3.js CDN 加载失败

**现象**：双击 `graph.html`，一直显示 loading spinner 和 "Laying out graph..."，永远不结束。

**排查**：F12 打开 Console，输入 `typeof d3` — 返回 `"undefined"` 说明 D3.js 没加载。Network 面板显示 `d3js.org` 请求被代理/防火墙拦截。

**根因**：原版 `graph.html` 通过 CDN 加载 D3.js：
```html
<script src="https://d3js.org/d3.v7.min.js" ...></script>
```
内网环境或代理环境下，`d3js.org` 无法访问，整个页面无法渲染。

**修复**（✅ 已实施）：修改了 `visualization.py` 的 `_HTML_TEMPLATE` 和 `_AGGREGATED_HTML_TEMPLATE`，将 CDN `<script src="d3js.org">` 替换为 `<script>d3.v7.min.js 内联代码</script>`（280KB）。现在 `code-review-graph visualize` 生成的 `graph.html` 约 330KB，**完全自包含，零网络依赖**。`file://` 双击即用，内网/离线环境均可。

**验证**：
```bash
# 检查生成的 HTML 是否自包含
grep "Copyright.*Mike Bostock" <项目>\.code-review-graph\graph.html
# 如果有输出 → D3.js 已内联
grep "d3js.org" <项目>\.code-review-graph\graph.html
# 如果只有版权注释无 CDN 链接 → 自包含
```

**教训**：内网部署的工具，所有外部依赖必须下载后本地化或内联 - 不能依赖 CDN。

---

## 10. 硬编码绝对路径 & 外部网络依赖审计

**审计时间**：2026-07-11，对各配置文件进行全量扫描。

### 已修复的硬编码路径

| 文件 | 修复前 | 修复后 |
|---|---|---|
| `sync_crg.py` | `src = r'E:\ClaudeCode_test\...'` | `os.path.dirname(__file__)` 相对路径 |
| `sync_crg.py` | `site = r'C:\Users\elcher\...\site-packages'` | `site.getsitepackages()` 自动检测 |
| `code-review-graph-build.bat` | `python E:\ClaudeCode_test\check_parse.py` | `python "%~dp0check_parse.py"` 相对路径 |

### 外部网络依赖总览

| 依赖 | 现状 | 影响 |
|---|---|---|
| **D3.js CDN** | ✅ 已内联（visualization.py 模板） | 不影响 — graph.html 完全自包含 |
| **tree-sitter-language-pack** | 纯本地二进制包 | 不影响 — pip 离线安装即可 |
| **嵌入向量 API**（embeddings.py） | 可选功能，需 `CRG_OPENAI_API_KEY` 等环境变量 | 不影响 — 不调用 `embed_graph` 则不使用 |
| **GitHub API / PyPI** | build/visualize 不使用网络 | 不影响 — 正常流程完全离线 |

### 结论

本修复版的**核心功能**（`build` + `visualize`）完全离线可用。唯一需要外网的操作是 `pip install`（已通过离线包解决），以及可选的 `embed_graph` 语义搜索功能。

**数据文件（graph.db、graph.html）均为本地文件，不涉及任何云传输。**
