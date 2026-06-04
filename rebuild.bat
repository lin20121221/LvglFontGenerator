@echo off
setlocal enabledelayedexpansion

echo ========================================
echo LvglFontGenerator 快速重新编译
echo ========================================
echo.

set "BUILD_DIR=Z:\LvglFontUtility\LvglFontGenerator\build"

REM 检查构建目录是否存在
if not exist "%BUILD_DIR%" (
    echo [ERROR] 构建目录不存在: %BUILD_DIR%
    echo 请先运行 build_and_deploy.bat 进行完整编译
    pause
    exit /b 1
)

echo [1/2] 重新编译 Release 版本...
cd /d "%BUILD_DIR%"
cmake --build . --config Release
if errorlevel 1 (
    echo [ERROR] Release 编译失败
    pause
    exit /b 1
)
echo [OK] Release 编译成功

echo.
echo [2/2] 重新编译 Debug 版本...
cmake --build . --config Debug
if errorlevel 1 (
    echo [ERROR] Debug 编译失败
    pause
    exit /b 1
)
echo [OK] Debug 编译成功

echo.
echo ========================================
echo 重新编译完成！
echo ========================================
echo.
echo Release 版本: %BUILD_DIR%\Release\LvglFontGenerator.exe
echo Debug 版本:   %BUILD_DIR%\Debug\LvglFontGenerator.exe
echo.

echo 是否现在运行 Release 版本? (Y/N)
set /p RUN_APP=
if /i "%RUN_APP%"=="Y" (
    start "" "%BUILD_DIR%\Release\LvglFontGenerator.exe"
    echo.
    echo 程序已启动！
)

echo.
pause
