# code-review-graph 离线部署指南

## 一、外网机器：准备离线包（只需执行一次）

### 1. 下载 code-review-graph 及全部依赖

```bash
# 创建存放目录
mkdir offline_pkgs

# 下载所有 .whl 文件（自动递归下载 50+ 个依赖）
python3 -m pip download -d offline_pkgs code-review-graph

# 验证下载完整（应约 50-60 个 .whl 文件）
ls offline_pkgs/*.whl | wc -l
```

### 2. 把 offline_pkgs 整个文件夹拷贝到 U 盘

---

## 二、内网机器：安装（每台机器执行一次）

### 前置条件

- **Python 版本必须和外网机器一致**（二进制 .whl 不跨版本兼容）
- Windows 推荐 Python 3.11.x

### 安装命令

```cmd
:: 从 U 盘离线安装（不联网）
python -m pip install --no-index --find-links=<U盘盘符>\offline_pkgs code-review-graph
```

安装后验证：

```cmd
python -c "from code_review_graph.cli import main; print('OK')"
```

---

## 三、对 RTL 项目生成 Call Graph

### 前置条件

- 项目必须是 git 仓库（如果不是，先 `git init` + `git add .` + `git commit -m "init"`）

### 命令

```bash
# 1. 构建图谱（解析所有 .v/.sv 文件）
code-review-graph build --repo <项目目录>

# 2. 生成可交互的 HTML 图
code-review-graph visualize --repo <项目目录>

# 3. 用浏览器打开
start <项目目录>\.code-review-graph\graph.html
```

### 可选：用 HTTP 服务查看（比 file:// 打开更稳定）

```bash
# code-review-graph 自带 HTTP 服务
code-review-graph visualize --repo <项目目录> --serve
# 浏览器打开 http://localhost:8765/graph.html
```

---

## 四、资源消耗参考

| 项目规模 | .sv/.v 文件数 | build 时间 | graph.html 大小 |
|----------|--------------|-----------|----------------|
| 小型（如 async_fifo） | 2 | < 5 秒 | ~30 KB |
| 中型（如 Ibex RISC-V） | ~300 | ~30 秒 | ~1.5 MB |
| 大型（如香山/XiangShan） | 数千 | 数分钟 | 更大 |

---

## 五、常见问题

**Q: build 没有任何输出？**
检查项目是否是 git 仓库，code-review-graph 需要 git 才能工作。

**Q: 打开 graph.html 只看到左上角一小块？**
SVG 作为替换元素在 `file://` 协议下会退回到 300×150 默认尺寸。F12 打开 Console，粘贴以下代码回车：

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

**Q: 想要静态 SVG 图？**
需要安装 matplotlib：
```cmd
python -m pip install --no-index --find-links=<U盘>\offline_pkgs matplotlib
```
然后 `--format svg`。

**Q: 只关注 Verilog 文件？**
`build` 会自动识别文件扩展名（`.v` `.sv` `.vh` `.svh`），只解析 Verilog/SystemVerilog，忽略其他文件。