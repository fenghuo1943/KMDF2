@echo off
setlocal

echo ================================
echo   KMDF Driver Install Script
echo ================================
cd /d %~dp0
:: ====== 配置区 ======
set DEVICE_ID=root\KMDF2
set DRIVER_DIR=.\x64\Debug\KMDF2
set INF_FILE=%DRIVER_DIR%\KMDF2.inf
set SYS_FILE=%DRIVER_DIR%\KMDF2.sys
:: ====== 检查文件 ======
if not exist "%INF_FILE%" (
    echo [ERROR] INF not found: %INF_FILE%
    pause
    exit /b 1
)

if not exist "%SYS_FILE%" (
    echo [ERROR] SYS not found: %SYS_FILE%
    pause
    exit /b 1
)

:: ====== 检查 devcon ======
where devcon >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] devcon not found in PATH
    echo Please add WDK devcon path or use full path
    pause
    exit /b 1
)

:: ====== 卸载旧设备 ======
echo.
echo [STEP] Removing old device...
devcon remove %DEVICE_ID%

:: ====== 删除旧驱动（可选）=====
echo.
echo [STEP] Removing old driver package...
pnputil /delete-driver %INF_FILE% /uninstall /force >nul 2>nul

:: ====== 安装驱动 ======
echo.
echo [STEP] Installing driver...
devcon install %INF_FILE% %DEVICE_ID%

if %errorlevel% neq 0 (
    echo [ERROR] Driver install failed
    pause
    exit /b 1
)

:: ====== 重启设备 ======
echo.
echo [STEP] Restarting device...
devcon restart %DEVICE_ID%

:: ====== 查看状态 ======
echo.
echo [STEP] Checking device status...
devcon status %DEVICE_ID%

echo.
echo ================================
echo   DONE
echo ================================

pause