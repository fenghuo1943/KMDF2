@echo off
setlocal enabledelayedexpansion

echo ================================
echo   VHF Driver Install & Diagnose
echo ================================
cd /d %~dp0

set DEVICE_ID=ROOT\KMDF2
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

echo.
echo [INFO] Files OK
echo   INF: %INF_FILE%
echo   SYS: %SYS_FILE%

:: ====== 卸载旧设备 ======
echo.
echo [STEP 1] Removing old device...
pnputil /remove-device %DEVICE_ID%\* 2>nul
devcon remove %DEVICE_ID%\* 2>nul
timeout /t 2 /nobreak >nul

:: ====== 删除旧驱动 ======
echo.
echo [STEP 2] Removing old driver package...
for /f "tokens=2 delims=:" %%a in ('pnputil /enum-drivers ^| findstr /i "KMDF2.inf"') do (
    set OEM_INF=%%a
    echo   Deleting !OEM_INF!...
    pnputil /delete-driver !OEM_INF! /force 2>nul
)

:: ====== 安装新驱动 ======
echo.
echo [STEP 3] Installing driver package...
pnputil /add-driver "%INF_FILE%" /install

if %errorlevel% neq 0 (
    echo [ERROR] Driver installation failed
    pause
    exit /b 1
)

:: ====== 创建设备 ======
echo.
echo [STEP 4] Creating root-enumerated device...
devcon install "%INF_FILE%" "%DEVICE_ID%"

if %errorlevel% neq 0 (
    echo [ERROR] Device creation failed
    goto :diagnose
)

:: ====== 等待设备初始化 ======
echo.
echo [STEP 5] Waiting for device initialization...
timeout /t 3 /nobreak >nul

:: ====== 诊断信息 ======
:diagnose
echo.
echo ================================
echo   Device Status
echo ================================
devcon status "%DEVICE_ID%\*"

echo.
echo ================================
echo   Hardware IDs
echo ================================
devcon hwids "%DEVICE_ID%\*"

echo.
echo ================================
echo   Driver Details
echo ================================
devcon driverfiles "%DEVICE_ID%\*"

echo.
echo ================================
echo   Registry Check (LowerFilters)
echo ================================
for /f "tokens=2 delims=\" %%a in ('devcon hwids "%DEVICE_ID%\*" ^| findstr /i "ROOT\\KMDF2"') do (
    set INSTANCE=%%a
    echo Checking HKLM\SYSTEM\CurrentControlSet\Enum\ROOT\KMDF2\!INSTANCE!\LowerFilters
    reg query "HKLM\SYSTEM\CurrentControlSet\Enum\ROOT\KMDF2\!INSTANCE!" /v LowerFilters 2>nul
    if !errorlevel! equ 0 (
        echo   [OK] LowerFilters exists
    ) else (
        echo   [ERROR] LowerFilters NOT FOUND - This is the problem!
    )
)

echo.
echo ================================
echo   Next Steps
echo ================================
echo 1. Open DebugView (as Admin)
echo 2. Enable Capture Kernel (Ctrl+K)
echo 3. Re-run this script
echo 4. Check DebugView output for VHF logs
echo.
echo If you see "Code 10" error:
echo   - Check LowerFilters registry value
echo   - Ensure mshidkmdf.sys exists in System32\drivers
echo   - Verify report descriptor is correct
echo.

pause
