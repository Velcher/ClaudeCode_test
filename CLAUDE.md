# Claude Code 实验项目

## 技术栈
- verilog
- 按功能模块组织

## Git 权限
- 本地命令可直接执行（add、status、diff、log 等）
- **需要确认的操作**：git commit、影响 GitHub 公开页面的操作（push、新分支首次推送、gh pr create 等）、新建分支前通知用户（含分支名和用途）
- 同样适用于 gh 相关命令
- **不下载 GitHub 仓库**：不执行 `git clone`、`curl`/`wget` 下载 GitHub 仓库或 zip，因为代理速度很慢。需要克隆/下载时告知用户手动操作
- 保持提交历史干净，尽可能少 commit
- **执行大型修改、不确定结果的修改、或删除操作前，先 git commit 保存当前状态**，方便回退。提交信息以"快照："开头，如 `快照：修改 code-review-graph 可视化模板前`

## 沟通约定
- 用户使用语音输入，可能存在错别字或谐音（如"渐新"= 建新、"cloud点MD"= CLAUDE.md、"shan了"= 删了），不确定的指令先确认再执行
- 语音无法区分大小写，需根据上下文自动修正（如汇编指令统一用小写 `ld1d` 而非 `LD1D`）
- `FMoPA` 只能有三种写法：`fmopa`、`Fmopa`、`FMOPA`，不可混用大小写

## code-review-graph 工作流

生成 call graph 前，使用 wrapper 脚本自动检查解析质量 + 构建：

```bash
# 自动检查 + 构建（两步合一的 wrapper）
E:\ClaudeCode_test\code-review-graph-build.bat <项目目录>

# 单独检查（不构建，只看 tree-sitter 解析质量）
python E:\ClaudeCode_test\check_parse.py <RTL目录>
```

报告生成在 `<项目>\.code-review-graph\parse_report.txt`，与 `graph.db` 在同一目录。

**解析质量判断标准**：
- ✅ `<0.5%` ERROR → call graph 可信
- ⚠️ `0.5%-5%` ERROR → 部分模块实例化可能遗漏（通常因 SystemVerilog 高级语法导致）
- ❌ `>5%` ERROR → 图不可靠，禁止用来做审查判断
