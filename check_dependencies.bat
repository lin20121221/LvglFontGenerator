@echo off
setlocal enabledelayedexpansion

echo ========================================
echo LvglFontGenerator DLL 依赖检查工具
echo ========================================
echo.

set "EXE_PATH=Z:\LvglFontUtility\LvglFontGenerator\build\Release\LvglFontGenerator.exe"

if not exist "%EXE_PATH%" (
    echo [ERROR] 可执行文件不存在: %EXE_PATH%
    pause
    exit /b 1
)

echo 检查可执行文件: %EXE_PATH%
echo.

REM 尝试运行程序
echo [测试 1] 尝试启动程序...
start "" "%EXE_PATH%"
timeout /t 3 >nul

REM 检查进程是否在运行
tasklist | find /i "LvglFontGenerator.exe" >nul
if errorlevel 1 (
    echo [FAIL] 程序未能启动
    echo.
    echo 可能的原因:
    echo 1. 缺少必需的 DLL
    echo 2. DLL 版本不兼容
    echo 3. 运行时错误
    echo.
    echo 建议操作:
    echo 1. 运行 copy_dlls.bat 复制所有 DLL
    echo 2. 检查 Windows 事件查看器中的错误日志
    echo 3. 使用 Dependency Walker 检查依赖
) else (
    echo [OK] 程序成功启动！
    echo.
    echo 进程信息:
    tasklist | find /i "LvglFontGenerator.exe"
)

echo.
echo ========================================
echo.

REM 列出当前目录中的 DLL
set "DLL_DIR=%~dp0build\Release"
echo Release 目录中的 DLL 文件:
echo %DLL_DIR%
echo.

if exist "%DLL_DIR%" (
    dir /b "%DLL_DIR%\*.dll" 2>nul
    echo.
    dir /b "%DLL_DIR%\*.dll" 2>nul | find /c /v ""
    echo 个 DLL 文件
) else (
    echo [ERROR] Release 目录不存在
)

echo.
echo ========================================
echo 必需的 DLL 清单
echo ========================================
echo.
echo Qt 依赖 (由 windeployqt 部署):
echo   - Qt6Core.dll
echo   - Qt6Gui.dll
echo   - Qt6Widgets.dll
echo   - Qt6Network.dll (可选)
echo   - Qt6Svg.dll (可选)
echo.
echo HarfBuzz 依赖:
echo   - libharfbuzz-0.dll
echo   - libgraphite2.dll
echo.
echo GLib 依赖:
echo   - libglib-2.0-0.dll
echo   - libintl-8.dll
echo   - libpcre2-8-0.dll
echo   - libiconv-2.dll
echo.
echo MinGW 运行时:
echo   - libgcc_s_seh-1.dll
echo   - libstdc++-6.dll
echo   - libwinpthread-1.dll
echo.
echo FreeType 依赖:
echo   - libfreetype-6.dll
echo   - libbz2-1.dll
echo   - libpng16-16.dll
echo   - zlib1.dll
echo   - libbrotlicommon.dll
echo   - libbrotlidec.dll
echo.

pause
