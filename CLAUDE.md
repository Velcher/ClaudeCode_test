# Claude Code 实验项目

## 技术栈
- verilog
- 按功能模块组织

## Git 权限
- 本地命令可直接执行（add、status、diff、log 等）
- **需要确认的操作**：git commit、影响 GitHub 公开页面的操作（push、新分支首次推送、gh pr create 等）、新建分支前通知用户（含分支名和用途）
- 同样适用于 gh 相关命令
- 保持提交历史干净，尽可能少 commit

## 沟通约定
- 用户使用语音输入，可能存在错别字或谐音（如"渐新"= 建新、"cloud点MD"= CLAUDE.md、"shan了"= 删了），不确定的指令先确认再执行
- 语音无法区分大小写，需根据上下文自动修正（如汇编指令统一用小写 `ld1d` 而非 `LD1D`）
- `FMoPA` 只能有三种写法：`fmopa`、`Fmopa`、`FMOPA`，不可混用大小写
