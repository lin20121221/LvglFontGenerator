@echo off
echo ========================================
echo LvglFontGenerator HarfBuzz 测试
echo ========================================
echo.

set EXE_PATH=Z:\LvglFontUtility\LvglFontGenerator\build\Release\LvglFontGenerator.exe
set FONT_PATH=Z:\wqy-zenhei\arialbd.ttf

echo 检查可执行文件...
if exist "%EXE_PATH%" (
    echo [OK] 找到可执行文件: %EXE_PATH%
) else (
    echo [ERROR] 未找到可执行文件: %EXE_PATH%
    pause
    exit /b 1
)

echo.
echo 检查测试字体...
if exist "%FONT_PATH%" (
    echo [OK] 找到测试字体: %FONT_PATH%
) else (
    echo [ERROR] 未找到测试字体: %FONT_PATH%
    pause
    exit /b 1
)

echo.
echo 检查 HarfBuzz DLL...
set DLL_PATH=Z:\LvglFontUtility\LvglFontGenerator\build\Release\libharfbuzz-0.dll
if exist "%DLL_PATH%" (
    echo [OK] 找到 HarfBuzz DLL: %DLL_PATH%
) else (
    echo [ERROR] 未找到 HarfBuzz DLL: %DLL_PATH%
    pause
    exit /b 1
)

echo.
echo ========================================
echo 所有检查通过！
echo ========================================
echo.
echo 现在请手动运行 LvglFontGenerator：
echo.
echo 1. 双击运行: %EXE_PATH%
echo.
echo 2. 在界面中：
echo    - 选择字体: %FONT_PATH%
echo    - 字号: 16
echo    - BPP: 4 或 8
echo    - [重要] 勾选 "Enable Kerning" 选项
echo    - 输入测试字符: AVTOWAVY
echo    - 点击生成
echo.
echo 3. 查看控制台输出，应该看到：
echo    "Successfully extracted kerning using HarfBuzz"
echo.
echo 4. 检查生成的 .c 文件，应该包含 kerning 表而不是 "No kerning data"
echo.
echo ========================================
echo.

pause

echo.
echo 是否现在启动 LvglFontGenerator? (Y/N)
set /p LAUNCH=
if /i "%LAUNCH%"=="Y" (
    start "" "%EXE_PATH%"
    echo.
    echo LvglFontGenerator 已启动！
) else (
    echo.
    echo 你可以稍后手动运行: %EXE_PATH%
)

echo.
pause
