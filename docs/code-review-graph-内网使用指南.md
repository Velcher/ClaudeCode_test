# code-review-graph 离线环境使用指南

> 本指南面向在内网（无互联网）环境中需要对 Verilog / SystemVerilog 项目进行 Call Graph 分析的用户。

---

## 一、工具简介

`code-review-graph` 是一个基于 `tree-sitter` 的代码调用关系图谱生成工具。通过解析 Verilog/SystemVerilog 文件，它能够：

- 生成模块与模块之间的实例化调用关系图（Call Graph）
- 生成可拖拽、可缩放的交互式 HTML 可视化图
- 生成 Markdown 格式的 Wiki 文档
- 解析质量检测：**自动分析 RTL 代码解析质量与调用链完整性**

本压缩包包含经过定制修复的 `code-review-graph` 源码、所有离线 Python 依赖以及验证示例项目。

---

## 二、压缩包内容说明

| 文件/目录 | 用途 |
|---|---|
| `offline_pkgs/` | **离线 Python 依赖包**：包含 `code-review-graph` 及其所有依赖的 `.whl` 文件 |
| `code-review-graph-main/` | **修复后的工具源码**：已修复了对 SystemVerilog 语法（`interface_instantiation`）的识别、解析质量检查及可视化显示问题 |
| `check_parse.py` | **独立解析质量检查脚本** |
| `sync_crg.py` | **源码同步脚本** |
| `asynfifo/` | **小规模验证项目**：用于初学者快速了解工具的输出格式 |
| `ibex/` | **中规模验证项目（RISC-V Ibex 内核）**：包含约 300 个 SystemVerilog 文件 |
| `verilog-pcie-master/` | **中规模验证项目（PCIe 控制器）**：纯 Verilog，用于验证 tree-sitter 解析强度 |

---

## 三、编译环境配置与安装

**前提条件**：

-   **操作系统**：Windows （建议 Windows 10 / 11 或对应版本 Windows Server）
-   **Python 版本**：**必须为 Python 3.11.x** （为了兼容预下载的二进制依赖包）
-   **Git**：代码解析过程需要依赖 `git` 跟踪文件变更，请确保已安装 Git

### 安装步骤

1.  **配置 Python 与 Git**
    -   安装 Python 3.11.x，并勾选 “Add Python to PATH”
    -   安装 Git，配置基本的用户名和邮箱
    -   确认环境正常：
        ```cmd
        python --version
        git --version
        ```

2.  **离线安装 code-review-graph**
    -   在命令行中执行：
        ```cmd
        python -m pip install --no-index --find-links=<离线包路径>\offline_pkgs code-review-graph
        ```
    -   例如，假设压缩包解压至 `D:\`:
        ```cmd
        python -m pip install --no-index --find-links=D:\offline_pkgs code-review-graph
        ```

3.  **覆盖修复版源码**（重要）
    -   将本压缩包中 `code-review-graph-main\code-review-graph-main\code_review_graph` 下的所有文件，覆盖到 Python 的 site-packages 目录中。你也可以使用 `sync_crg.py` 脚本自动完成该步骤：
        ```cmd
        python sync_crg.py
        ```

4.  **验证安装**
    ```cmd
    code-review-graph --version
    ```
    应正常输出版本号 `2.3.6`。

---

## 四、快速体验（使用 asynfifo 项目）

建议首先使用本压缩包内置的 `asynfifo/` 项目验证工具链完整性。

1.  **初始化 Git 仓库**
    ```cmd
    cd asynfifo
    git init
    git add .
    git commit -m "init"
    ```

2.  **生成 Call Graph**
    ```cmd
    code-review-graph build
    code-review-graph visualize
    ```

3.  **查看结果**
    -   打开当前目录下生成的 `.code-review-graph\graph.html`
    -   你将看到一个简单的 `asyn_fifo` 模块及其 Testbench 的调用关系图。

---

## 五、在自有项目中使用

1.  **初始化项目**
    进入你自己的 RTL 代码目录，执行（若已是 Git 仓库则可跳过）：
    ```cmd
    git init
    git add .
    git commit -m "project init"
    ```

2.  **生成 Call Graph**
    ```cmd
    code-review-graph build
    code-review-graph visualize
    ```

3.  **查看解析质量报告**
    在构建过程中，工具会自动分析每一个 `.v` 文件的解析质量，生成 `.code-review-graph\parse_report.txt`。

    -   解析质量报告会统计每个文件的 `tree-sitter` ERROR 数量。
    -   **特别注意**：报告中标为 `bad` （错误率 > 5%）的文件，其内部实例化关系可能丢失。

---

## 六、验证项目说明

### 1. Ibex （RISC-V CPU 内核）
-   **路径**：`ibex/`
-   **规模**：约 300 个 `.sv` 文件
-   **目的**：验证中型 SystemVerilog 项目的 Call Graph 生成效果。
-   **使用方法**：
    ```cmd
    cd ibex
    git init
    git add .
    git commit -m "init"
    code-review-graph build
    code-review-graph visualize
    ```
    打开 `.code-review-graph\graph.html` 查看。

### 2. verilog-pcie （PCIe 控制器）
-   **路径**：`verilog-pcie-master/`
-   **规模**：约 75 个纯 Verilog 文件（选取自原项目的 rtl 和 common 目录）
-   **目的**：验证纯 Verilog （不含 SystemVerilog 高级语法）项目的解析准确率（理论应接近 100%）。
-   **使用方法**：
    ```cmd
    cd verilog-pcie-master
    git init
    git add .
    git commit -m "init"
    code-review-graph build
    code-review-graph visualize
    ```
    打开 `.code-review-graph\graph.html` 查看。

---

## 七、打开 graph.html 注意事项

### 自包含设计

本修复版生成的 `graph.html` **已将 D3.js 内联**（约 280KB），完全自包含，不需要任何网络连接。双击即可直接在浏览器中打开使用，内网/离线环境均可正常工作。

### 节点堆积在左上角怎么办？

本修复版已在模板中内置了 `position:fixed` 修复，绝大多数情况下图会自动居中显示。如果仍有问题，按 F12 打 Console，粘贴以下备用脚本：

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
```

---

## 八、常见问题与踩坑总结

### 1. 解析失败 / 孤立节点
-   **表现**：Call Graph 图中出现孤立节点，但实际代码中存在例化关系。
-   **排查**：查看构建后的 `.code-review-graph\parse_report.txt`。
-   **原因 A**：代码使用了过于复杂的 SystemVerilog 语法（例如复杂的 `interface` 声明，或在 `#(parameter)` 括号内写注释），导致 `tree-sitter` 解析为较新的语法树节点，未能被 code-review-graph 识别 。
-   **应对**：
    -   检查报告中的 ERROR 节点片段。
    -   如果仅少量文件报告错误，并导致孤立节点，是已知的 `tree-sitter-verilog` 限制。由于我们安装的是修复版，常见的不带参例化问题（`module_name inst_name (...)`）已修复。
    -   对于 ERROR 超过 5% 的 SystemVerilog 文件，建议忽略该文件或将其暂时移除，单独审查，避免干扰全图的调用关系判断。

### 2. 构建时无输出
-   **原因**：项目目录不是 Git 仓库。
-   **解决**：依次执行 `git init` → `git add .` → `git commit` 初始化。

### 3. 代码中存在大量 Python / Shell 脚本
-   **表现**：构建报告里发现了很多 `py`、`sh` 等不属于 Verilog 的文件。
-   **解决**：
    -   建议仅将 RTL 设计文件所在子目录（如 `./rtl`）纳入 Git 管理
    -   方式：进入 `rtl` 目录执行 `git init`（避免管理到上层验证脚本及工具链文件）。

### 4. 打开 graph.html 一直显示 Loading / 图不动
-   **原因**：旧版模板生成的 HTML 从 `d3js.org` CDN 加载 D3.js，内网被拦截。
-   **解决**（✅ 已修复）：本修复版已将 D3.js 内联到 HTML 中（280KB，完全自包含）。重新执行 `code-review-graph visualize` 生成即可。验证：`grep "Copyright.*Mike Bostock" .code-review-graph\graph.html` 有输出说明已内联。

