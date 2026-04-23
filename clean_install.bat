@echo off
setlocal enabledelayedexpansion

echo ================================
echo   VHF Driver Clean Install
echo ================================
cd /d %~dp0

set DEVICE_HW_ID=ROOT\KMDF2
set DRIVER_DIR=.\x64\Debug\KMDF2
set INF_FILE=%DRIVER_DIR%\KMDF2.inf

:: ====== 步骤1: 完全清理 ======
echo.
echo [STEP 1] Cleaning up old installation...

:: 禁用并删除所有KMDF2设备
for /f "tokens=2 delims=\" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Enum\ROOT" /s /f "KMDF2" /k 2^>nul ^| findstr "HKEY"') do (
    set DEV_PATH=%%a
    echo   Disabling device: !DEV_PATH!
    devcon disable "!DEV_PATH!" 2>nul
    devcon remove "!DEV_PATH!" 2>nul
)

timeout /t 2 /nobreak >nul

:: 删除所有KMDF2驱动包
for /f "tokens=2 delims=:" %%a in ('pnputil /enum-drivers ^| findstr /i "KMDF2"') do (
    set OEM_INF=%%a
    echo   Deleting driver: !OEM_INF!
    pnputil /delete-driver !OEM_INF! /force 2>nul
)

:: 清理注册表
reg delete "HKLM\SYSTEM\CurrentControlSet\Enum\ROOT\KMDF2" /f 2>nul
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\KMDF2" /f 2>nul

timeout /t 2 /nobreak >nul

:: ====== 步骤2: 安装驱动包 ======
echo.
echo [STEP 2] Installing driver package...
pnputil /add-driver "%INF_FILE%" /install

if %errorlevel% neq 0 (
    echo [ERROR] Failed to install driver package
    pause
    exit /b 1
)

:: ====== 步骤3: 使用正确的命令创建设备 ======
echo.
echo [STEP 3] Creating device node...

:: 方法1: 使用 devcon create + install
devcon create "%DEVICE_HW_ID%" "{745a17a0-74d3-11d0-b6fe-00a0c90f57da}" "VHF Virtual Mouse"

if %errorlevel% equ 0 (
    echo   Device node created
) else (
    echo   [WARNING] devcon create failed, trying alternative method...
)

timeout /t 1 /nobreak >nul

:: 方法2: 扫描硬件变化
echo   Scanning for hardware changes...
devcon rescan

timeout /t 3 /nobreak >nul

:: ====== 步骤4: 验证 ======
echo.
echo [STEP 4] Verifying installation...

:: 检查设备是否存在
devcon status "%DEVICE_HW_ID%*" > temp_status.txt 2>&1
findstr /i "running" temp_status.txt >nul
if %errorlevel% equ 0 (
    echo   [OK] Device is running
    type temp_status.txt
) else (
    echo   [ERROR] Device not running
    type temp_status.txt
    echo.
    echo   Checking device status...
    devcon status "%DEVICE_HW_ID%*"
)

del temp_status.txt 2>nul

:: 检查注册表
echo.
echo [STEP 5] Checking registry...
for /f "tokens=2 delims=\" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Enum\ROOT" /s /f "KMDF2" /k 2^>nul ^| findstr "HKEY"') do (
    set INST=%%a
    echo   Instance: !INST!
    
    reg query "HKLM\SYSTEM\CurrentControlSet\Enum\ROOT\KMDF2\!INST!" /v LowerFilters 2>nul | findstr "mshidkmdf" >nul
    if !errorlevel! equ 0 (
        echo     [OK] LowerFilters contains mshidkmdf
    ) else (
        echo     [ERROR] LowerFilters missing or incorrect
        reg query "HKLM\SYSTEM\CurrentControlSet\Enum\ROOT\KMDF2\!INST!" /v LowerFilters 2>nul
    )
)

echo.
echo ================================
echo   Installation Complete
echo ================================
echo.
echo Next steps:
echo 1. Check DebugView for VHF logs
echo 2. If Code 10 error persists, check:
echo    - LowerFilters registry value
echo    - Event Viewer for detailed errors
echo.

pause
