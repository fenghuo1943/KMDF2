# VHF驱动诊断脚本
Write-Host "================================" -ForegroundColor Cyan
Write-Host "  VHF Driver Diagnosis Tool" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""

$deviceInstanceId = "ROOT\KMDF2\*"

# 1. 检查设备是否存在
Write-Host "[1] 检查设备状态..." -ForegroundColor Yellow
$devices = Get-PnpDevice -InstanceId $deviceInstanceId -ErrorAction SilentlyContinue

if ($devices) {
    foreach ($device in $devices) {
        Write-Host "  设备实例ID: $($device.InstanceId)" -ForegroundColor Green
        Write-Host "  友好名称: $($device.FriendlyName)" -ForegroundColor Green
        Write-Host "  类名: $($device.Class)" -ForegroundColor Green
        Write-Host "  状态: $($device.Status)" -ForegroundColor $(if($device.Status -eq 'OK'){'Green'}else{'Red'})
        Write-Host "  问题代码: $($device.ProblemCode)" -ForegroundColor $(if($device.ProblemCode -eq 0){'Green'}else{'Red'})
        
        if ($device.ProblemCode -ne 0) {
            Write-Host "  问题描述: $($device.ProblemDescription)" -ForegroundColor Red
        }
    }
} else {
    Write-Host "  [ERROR] 未找到设备" -ForegroundColor Red
}

Write-Host ""

# 2. 检查注册表 LowerFilters
Write-Host "[2] 检查注册表 LowerFilters..." -ForegroundColor Yellow
$enumPath = "HKLM:\SYSTEM\CurrentControlSet\Enum\ROOT\KMDF2"

if (Test-Path $enumPath) {
    $instances = Get-ChildItem $enumPath
    
    foreach ($instance in $instances) {
        $fullPath = "$($instance.PSPath)"
        Write-Host "  检查: $($instance.Name)" -ForegroundColor Cyan
        
        $lowerFilters = Get-ItemProperty -Path $fullPath -Name "LowerFilters" -ErrorAction SilentlyContinue
        
        if ($lowerFilters) {
            Write-Host "    LowerFilters: $($lowerFilters.LowerFilters)" -ForegroundColor Green
            
            if ($lowerFilters.LowerFilters -contains "mshidkmdf") {
                Write-Host "    [OK] mshidkmdf 在 LowerFilters 中" -ForegroundColor Green
            } else {
                Write-Host "    [ERROR] mshidkmdf 不在 LowerFilters 中!" -ForegroundColor Red
            }
        } else {
            Write-Host "    [ERROR] LowerFilters 不存在!" -ForegroundColor Red
            Write-Host "    这是导致代码10错误的主要原因" -ForegroundColor Red
        }
    }
} else {
    Write-Host "  [INFO] 设备未在注册表中枚举" -ForegroundColor Yellow
}

Write-Host ""

# 3. 检查驱动服务
Write-Host "[3] 检查驱动服务..." -ForegroundColor Yellow
$service = Get-Service -Name "KMDF2" -ErrorAction SilentlyContinue

if ($service) {
    Write-Host "  服务名称: $($service.Name)" -ForegroundColor Green
    Write-Host "  显示名称: $($service.DisplayName)" -ForegroundColor Green
    Write-Host "  状态: $($service.Status)" -ForegroundColor $(if($service.Status -eq 'Running'){'Green'}else{'Yellow'})
    Write-Host "  启动类型: $($service.StartType)" -ForegroundColor Green
} else {
    Write-Host "  [WARNING] KMDF2 服务不存在" -ForegroundColor Yellow
}

Write-Host ""

# 4. 检查驱动文件
Write-Host "[4] 检查驱动文件..." -ForegroundColor Yellow
$sysPath = "C:\Windows\System32\drivers\KMDF2.sys"

if (Test-Path $sysPath) {
    $file = Get-Item $sysPath
    Write-Host "  [OK] KMDF2.sys 存在" -ForegroundColor Green
    Write-Host "    大小: $($file.Length) 字节" -ForegroundColor Green
    Write-Host "    修改时间: $($file.LastWriteTime)" -ForegroundColor Green
} else {
    Write-Host "  [ERROR] KMDF2.sys 不存在于 System32\drivers" -ForegroundColor Red
}

$mshidkmdfPath = "C:\Windows\System32\drivers\mshidkmdf.sys"
if (Test-Path $mshidkmdfPath) {
    Write-Host "  [OK] mshidkmdf.sys 存在" -ForegroundColor Green
} else {
    Write-Host "  [ERROR] mshidkmdf.sys 不存在!" -ForegroundColor Red
}

Write-Host ""

# 5. 检查INF安装
Write-Host "[5] 检查已安装的驱动包..." -ForegroundColor Yellow
$oemInfs = pnputil /enum-drivers | Select-String "KMDF2.inf"

if ($oemInfs) {
    Write-Host "  [OK] KMDF2.inf 已安装:" -ForegroundColor Green
    $oemInfs | ForEach-Object { Write-Host "    $_" -ForegroundColor Green }
} else {
    Write-Host "  [ERROR] KMDF2.inf 未安装" -ForegroundColor Red
}

Write-Host ""
Write-Host "================================" -ForegroundColor Cyan
Write-Host "  诊断完成" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "如果看到 LowerFilters 缺失：" -ForegroundColor Yellow
Write-Host "  1. 卸载设备: devcon remove ROOT\KMDF2\*" -ForegroundColor White
Write-Host "  2. 删除驱动: pnputil /delete-driver oemXX.inf /force" -ForegroundColor White
Write-Host "  3. 重新安装: 使用 install_debug.bat" -ForegroundColor White
Write-Host ""

pause
