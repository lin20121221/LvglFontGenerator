@echo off
setlocal enabledelayedexpansion

echo ========================================
echo LvglFontGenerator 自动编译和部署 (MinGW)
echo ========================================
echo.

REM 配置路径
set "PROJECT_DIR=Z:\LvglFontUtility\LvglFontGenerator"
set "BUILD_DIR=%PROJECT_DIR%\build"
set "QT_DIR=C:\Qt\6.11.0\mingw_64"
set "CMAKE_GENERATOR=MinGW Makefiles"
set "MINGW_BIN=C:\Qt\Tools\mingw1310_64\bin"
set "MSYS2_BIN=C:\msys64\mingw64\bin"

REM 检查 Qt 是否存在
if not exist "%QT_DIR%" (
    echo [ERROR] Qt 目录不存在: %QT_DIR%
    echo 请修改脚本中的 QT_DIR 变量
    pause
    exit /b 1
)

REM 检查 MinGW 是否存在
if not exist "%MINGW_BIN%" (
    echo [ERROR] MinGW 目录未找到: %MINGW_BIN%
    echo 请修改脚本中的 MINGW_BIN 变量
    pause
    exit /b 1
)
if not exist "%MINGW_BIN%\g++.exe" (
    echo [ERROR] MinGW 编译器 g++.exe 未找到
    pause
    exit /b 1
)

REM 将 MinGW 和 Qt 添加到 PATH
set "PATH=%MINGW_BIN%;%QT_DIR%\bin;%PATH%"

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

echo [1/5] 清理旧的构建目录...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
)
mkdir "%BUILD_DIR%"
echo [OK] 构建目录已清理

echo.
echo [2/5] 配置 CMake...
cd /d "%BUILD_DIR%"
cmake -G "%CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=Release "%PROJECT_DIR%"
if errorlevel 1 (
    echo [ERROR] CMake 配置失败
    pause
    exit /b 1
)
echo [OK] CMake 配置成功

echo.
echo [3/5] 编译项目...
cmake --build . --config Release -- -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo [ERROR] 编译失败
    pause
    exit /b 1
)
echo [OK] 编译成功

echo.
echo [4/5] 部署 Qt 依赖...
"%QT_DIR%\bin\windeployqt.exe" "%BUILD_DIR%\LvglFontGenerator.exe"
if errorlevel 1 (
    echo [WARNING] windeployqt 失败，但继续...
) else (
    echo [OK] Qt 依赖部署成功
)

echo.
echo [5/5] 复制 HarfBuzz 和依赖 DLL...

REM HarfBuzz 及其依赖的 DLL 列表
set "HARFBUZZ_DLLS=libharfbuzz-0.dll libgraphite2.dll libglib-2.0-0.dll libintl-8.dll libpcre2-8-0.dll"
set "FREETYPE_DLLS=libfreetype-6.dll libbz2-1.dll libpng16-16.dll zlib1.dll libbrotlicommon.dll libbrotlidec.dll"
set "ICONV_DLLS=libiconv-2.dll"

REM 复制到构建目录
echo 复制 DLL 到构建目录...
set "MISSING_DLLS="
for %%D in (%HARFBUZZ_DLLS% %FREETYPE_DLLS% %ICONV_DLLS%) do (
    if exist "%MSYS2_BIN%\%%D" (
        copy /y "%MSYS2_BIN%\%%D" "%BUILD_DIR%\" >nul 2>&1
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
    echo [OK] 所有 DLL 已复制
)

echo.
echo ========================================
echo 编译和部署完成！
echo ========================================
echo.
echo 可执行文件: %BUILD_DIR%\LvglFontGenerator.exe
echo.

REM 检查可执行文件
if exist "%BUILD_DIR%\LvglFontGenerator.exe" (
    echo [OK] 可执行文件已生成
) else (
    echo [ERROR] 可执行文件未找到
)

echo.
echo 是否现在运行程序? (Y/N)
set /p RUN_APP=
if /i "%RUN_APP%"=="Y" (
    start "" "%BUILD_DIR%\LvglFontGenerator.exe"
    echo.
    echo 程序已启动！
)

echo.
pause
