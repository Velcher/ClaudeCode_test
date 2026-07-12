@echo off
:: code-review-graph-build — 先检查解析质量，再构建 call graph
:: 用法: code-review-graph-build <项目目录>

if "%1"=="" (
    echo 用法: code-review-graph-build ^<项目目录^>
    echo 示例: code-review-graph-build E:\my_project
    exit /b 1
)

set REPO=%1

echo ================================================================
echo  Step 1/2: 检查 tree-sitter-verilog 解析质量
echo ================================================================
python "%~dp0check_parse.py" "%REPO%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ⚠️ 部分文件解析有问题，call graph 可能不完整
    echo 详情见: %REPO%\.code-review-graph\parse_report.txt
    echo.
    pause
)

echo.
echo ================================================================
echo  Step 2/2: 构建 code-review-graph 图谱
echo ================================================================
code-review-graph build --repo "%REPO%"

echo.
echo 报告: %REPO%\.code-review-graph\parse_report.txt
echo 图谱: %REPO%\.code-review-graph\graph.db
