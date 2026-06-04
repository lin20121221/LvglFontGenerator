@echo off
setlocal enabledelayedexpansion

echo ========================================
echo 检查和复制所有必需的第三方 DLL
echo ========================================
echo.

set "BUILD_DIR=Z:\LvglFontUtility\LvglFontGenerator\build"
set "MSYS2_BIN=C:\msys64\mingw64\bin"

REM 检查 MSYS2 是否存在
if not exist "%MSYS2_BIN%" (
    echo [ERROR] MSYS2 目录不存在: %MSYS2_BIN%
    echo 请安装 MSYS2 或修改脚本中的路径
    pause
    exit /b 1
)

REM 定义所有需要的 DLL
echo 定义需要的 DLL 列表...
set "HARFBUZZ_DLLS=libharfbuzz-0.dll libgraphite2.dll"
set "GLIB_DLLS=libglib-2.0-0.dll libintl-8.dll libpcre2-8-0.dll libiconv-2.dll"
set "MINGW_RUNTIME=libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll"
set "FREETYPE_DLLS=libfreetype-6.dll libbz2-1.dll libpng16-16.dll zlib1.dll libbrotlicommon.dll libbrotlidec.dll"

set "ALL_DLLS=%HARFBUZZ_DLLS% %GLIB_DLLS% %MINGW_RUNTIME% %FREETYPE_DLLS%"

echo.
echo [1/2] 复制 DLL 到 Release 目录...
set "COPIED=0"
set "MISSING=0"

for %%D in (%ALL_DLLS%) do (
    if exist "%MSYS2_BIN%\%%D" (
        copy /y "%MSYS2_BIN%\%%D" "%BUILD_DIR%\Release\" >nul 2>&1
        if errorlevel 1 (
            echo [FAIL] %%D
            set /a MISSING+=1
        ) else (
            echo [OK] %%D
            set /a COPIED+=1
        )
    ) else (
        echo [MISS] %%D - 文件不存在
        set /a MISSING+=1
    )
)

echo.
echo Release 目录: 复制了 %COPIED% 个 DLL, 缺失 %MISSING% 个

echo.
echo [2/2] 复制 DLL 到 Debug 目录...
set "COPIED=0"
set "MISSING=0"

for %%D in (%ALL_DLLS%) do (
    if exist "%MSYS2_BIN%\%%D" (
        copy /y "%MSYS2_BIN%\%%D" "%BUILD_DIR%\Debug\" >nul 2>&1
        if errorlevel 1 (
            echo [FAIL] %%D
            set /a MISSING+=1
        ) else (
            echo [OK] %%D
            set /a COPIED+=1
        )
    ) else (
        echo [MISS] %%D - 文件不存在
        set /a MISSING+=1
    )
)

echo.
echo Debug 目录: 复制了 %COPIED% 个 DLL, 缺失 %MISSING% 个

echo.
echo ========================================
echo DLL 部署完成
echo ========================================
echo.

REM 列出 Release 目录中的所有 DLL
echo Release 目录中的 DLL:
dir /b "%BUILD_DIR%\Release\*.dll" 2>nul | find /c /v "" && echo 个 DLL 文件

echo.
echo Debug 目录中的 DLL:
dir /b "%BUILD_DIR%\Debug\*.dll" 2>nul | find /c /v "" && echo 个 DLL 文件

echo.
echo 提示: 如果程序仍然无法启动，请运行:
echo   cd %BUILD_DIR%\Release
echo   LvglFontGenerator.exe
echo 查看具体缺少哪个 DLL
echo.

pause
