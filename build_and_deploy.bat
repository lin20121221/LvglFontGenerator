@echo off
setlocal enabledelayedexpansion

echo ========================================
echo LvglFontGenerator 自动编译和部署
echo ========================================
echo.

REM 配置路径
set "PROJECT_DIR=Z:\LvglFontUtility\LvglFontGenerator"
set "BUILD_DIR=%PROJECT_DIR%\build"
set "QT_DIR=C:\Qt\6.11.0\msvc2022_64"
set "CMAKE_GENERATOR=Visual Studio 17 2022"
set "MSYS2_BIN=C:\msys64\mingw64\bin"

REM 检查 Qt 是否存在
if not exist "%QT_DIR%" (
    echo [ERROR] Qt 目录不存在: %QT_DIR%
    echo 请修改脚本中的 QT_DIR 变量
    pause
    exit /b 1
)

REM 检查 CMake 是否可用
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake 未找到，请确保 CMake 已安装并在 PATH 中
    pause
    exit /b 1
)

REM 检查 MSYS2 是否存在（HarfBuzz DLL）
if not exist "%MSYS2_BIN%" (
    echo [WARNING] MSYS2 目录不存在: %MSYS2_BIN%
    echo HarfBuzz DLL 可能无法复制
)

echo [1/6] 清理旧的构建目录...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
)
mkdir "%BUILD_DIR%"
echo [OK] 构建目录已清理

echo.
echo [2/6] 配置 CMake...
cd /d "%BUILD_DIR%"
cmake -G "%CMAKE_GENERATOR%" -A x64 "%PROJECT_DIR%"
if errorlevel 1 (
    echo [ERROR] CMake 配置失败
    pause
    exit /b 1
)
echo [OK] CMake 配置成功

echo.
echo [3/6] 编译 Release 版本...
cmake --build . --config Release
if errorlevel 1 (
    echo [ERROR] Release 编译失败
    pause
    exit /b 1
)
echo [OK] Release 编译成功

echo.
echo [4/6] 编译 Debug 版本...
cmake --build . --config Debug
if errorlevel 1 (
    echo [ERROR] Debug 编译失败
    pause
    exit /b 1
)
echo [OK] Debug 编译成功

echo.
echo [5/6] 部署 Qt 依赖 (Release)...
cd /d "%BUILD_DIR%\Release"
"%QT_DIR%\bin\windeployqt.exe" LvglFontGenerator.exe
if errorlevel 1 (
    echo [WARNING] windeployqt 失败，但继续...
) else (
    echo [OK] Qt 依赖部署成功 (Release)
)

echo.
echo [6/6] 复制 HarfBuzz 和 MinGW 运行时 DLL...

REM HarfBuzz 及其依赖的 DLL 列表
set "HARFBUZZ_DLLS=libharfbuzz-0.dll libgraphite2.dll libglib-2.0-0.dll libintl-8.dll libpcre2-8-0.dll"
set "MINGW_RUNTIME_DLLS=libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll"
set "FREETYPE_DLLS=libfreetype-6.dll libbz2-1.dll libpng16-16.dll zlib1.dll libbrotlicommon.dll libbrotlidec.dll"
set "ICONV_DLLS=libiconv-2.dll"

REM 复制到 Release
echo 复制 DLL 到 Release...
set "MISSING_DLLS="
for %%D in (%HARFBUZZ_DLLS% %MINGW_RUNTIME_DLLS% %FREETYPE_DLLS% %ICONV_DLLS%) do (
    if exist "%MSYS2_BIN%\%%D" (
        copy /y "%MSYS2_BIN%\%%D" "%BUILD_DIR%\Release\" >nul 2>&1
        if errorlevel 1 (
            echo [WARNING] 无法复制 %%D
        )
    ) else (
        set "MISSING_DLLS=!MISSING_DLLS! %%D"
    )
)

if defined MISSING_DLLS (
    echo [WARNING] 以下 DLL 未找到:!MISSING_DLLS!
) else (
    echo [OK] 所有 DLL 已复制到 Release
)

REM 复制到 Debug
echo 复制 DLL 到 Debug...
set "MISSING_DLLS="
for %%D in (%HARFBUZZ_DLLS% %MINGW_RUNTIME_DLLS% %FREETYPE_DLLS% %ICONV_DLLS%) do (
    if exist "%MSYS2_BIN%\%%D" (
        copy /y "%MSYS2_BIN%\%%D" "%BUILD_DIR%\Debug\" >nul 2>&1
        if errorlevel 1 (
            echo [WARNING] 无法复制 %%D
        )
    ) else (
        set "MISSING_DLLS=!MISSING_DLLS! %%D"
    )
)

if defined MISSING_DLLS (
    echo [WARNING] 以下 DLL 未找到:!MISSING_DLLS!
) else (
    echo [OK] 所有 DLL 已复制到 Debug
)

echo.
echo ========================================
echo 编译和部署完成！
echo ========================================
echo.
echo Release 版本: %BUILD_DIR%\Release\LvglFontGenerator.exe
echo Debug 版本:   %BUILD_DIR%\Debug\LvglFontGenerator.exe
echo.

REM 检查可执行文件
if exist "%BUILD_DIR%\Release\LvglFontGenerator.exe" (
    echo [OK] Release 可执行文件已生成
) else (
    echo [ERROR] Release 可执行文件未找到
)

if exist "%BUILD_DIR%\Debug\LvglFontGenerator.exe" (
    echo [OK] Debug 可执行文件已生成
) else (
    echo [ERROR] Debug 可执行文件未找到
)

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
