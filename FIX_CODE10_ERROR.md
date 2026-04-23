# VHF驱动代码10错误 - 最终解决方案

## 问题症状

**设备管理器显示**: "该设备无法启动。(代码 10)"  
**状态码**: STATUS_DEVICE_DATA_ERROR

**DebugView日志**:
```
[VHF Driver] DriverEntry called
[VHF Driver] EvtDeviceAdd called
[VHF Driver] PnP callbacks registered
[VHF Driver] Creating device object...
[VHF Driver] Device created successfully
[VHF Driver] EvtDeviceContextCleanup called    ← 关键：没有调用 PrepareHardware
```

**install_debug.bat输出**:
```
Device node created. Install is complete when drivers are installed...
Updating drivers for ROOT\KMDF2 from ...KMDF2.inf.
Drivers installed successfully.
[ERROR] Device creation failed
No matching devices found.
```

## 根本原因分析

从日志可以看出：
1. ✅ 驱动加载成功 (DriverEntry called)
2. ✅ 设备对象创建成功 (Device created successfully)
3. ❌ **驱动未绑定到设备** (没有调用 EvtDevicePrepareHardware)
4. ❌ 设备立即被清理 (EvtDeviceContextCleanup called)

这表明：**设备节点创建了，但PnP管理器没有找到匹配的驱动程序来绑定它。**

## 已修复的问题

### 1. INF文件段落命名错误 ✅

**错误**:
```inf
[KMDF2_Device.NT.HW]
AddReg = KMDF2_Device.NT.AddReg.HW    ; ← 错误的段落名

[KMDF2_Device.NT.AddReg.HW]           ; ← 这个段落不会被找到
HKR,,"LowerFilters",0x00010000,"mshidkmdf"
```

**修复**:
```inf
[KMDF2_Device.NT.HW]
AddReg = KMDF2_Device.NT.HW.AddReg    ; ← 正确的段落名

[KMDF2_Device.NT.HW.AddReg]           ; ← 匹配的名称
HKR,,"LowerFilters",0x00010000,"mshidkmdf"
```

### 2. LowerFilters注册表类型 ✅

使用 `0x00010000` (FLG_ADDREG_TYPE_MULTI_SZ) 而不是 `0x00010008`

## 完整解决步骤

### 步骤1: 完全清理旧安装

以**管理员身份**运行 PowerShell：

```powershell
# 停止并删除服务
Stop-Service KMDF2 -Force -ErrorAction SilentlyContinue
sc delete KMDF2

# 禁用并移除所有KMDF2设备
Get-PnpDevice | Where-Object {$_.InstanceId -like "*KMDF2*"} | ForEach-Object {
    Disable-PnpDevice -InstanceId $_.InstanceId -Confirm:$false -ErrorAction SilentlyContinue
    pnputil /remove-device $_.InstanceId
}

# 删除所有KMDF2驱动包
pnputil /enum-drivers | Select-String "KMDF2" | ForEach-Object {
    $line = $_.Line
    if ($line -match "oem(\d+)\.inf") {
        $oemNum = $matches[1]
        Write-Host "Deleting oem$oemNum.inf..."
        pnputil /delete-driver "oem$oemNum.inf" /force
    }
}

# 清理注册表
Remove-Item -Path "HKLM:\SYSTEM\CurrentControlSet\Enum\ROOT\KMDF2" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path "HKLM:\SYSTEM\CurrentControlSet\Services\KMDF2" -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "Cleanup complete. Please restart your computer." -ForegroundColor Green
```

### 步骤2: 重启计算机

**这一步是必须的**，以确保所有驱动状态被完全清除。

### 步骤3: 使用clean_install.bat安装

重启后，以**管理员身份**运行：

```batch
clean_install.bat
```

这个脚本会：
1. 再次清理残留
2. 安装驱动包
3. 创建设备节点（使用正确的Class GUID）
4. 扫描硬件变化
5. 验证安装状态
6. 检查LowerFilters注册表值

### 步骤4: 验证安装

#### 方法1: 检查设备状态
```batch
devcon status ROOT\KMDF2*
```

应该看到：
```
ROOT\KMDF2\0000
    Name: VHF Virtual Mouse Device
    Driver is running.
```

#### 方法2: 检查DebugView日志
应该看到完整的初始化序列：
```
[VHF Driver] DriverEntry called
[VHF Driver] EvtDeviceAdd called
[VHF Driver] PnP callbacks registered
[VHF Driver] Creating device object...
[VHF Driver] Device created successfully
[VHF Driver] EvtDevicePrepareHardware called         ← 关键！
[VHF Driver] Creating VHF...
[VHF Driver] Report descriptor size: 37 bytes
[VHF Driver] Calling VhfCreate...
[VHF Driver] VhfCreate succeeded, handle: 0x...
[VHF Driver] Calling VhfStart...
[VHF Driver] VHF started successfully
```

#### 方法3: 检查注册表
```batch
reg query "HKLM\SYSTEM\CurrentControlSet\Enum\ROOT\KMDF2\0000" /v LowerFilters
```

应该看到：
```
LowerFilters    REG_MULTI_SZ    mshidkmdf
```

## 如果仍然失败

### 检查清单

- [ ] 是否以管理员身份运行安装脚本？
- [ ] 是否在清理后重启了计算机？
- [ ] DebugView中是否看到 "EvtDevicePrepareHardware called"？
- [ ] 注册表中LowerFilters是否存在且包含 "mshidkmdf"？
- [ ] mshidkmdf.sys是否存在于 C:\Windows\System32\drivers\？

### 手动检查INF安装

```batch
:: 查看已安装的驱动包
pnputil /enum-drivers | findstr "KMDF2"

:: 应该看到类似：
:: 发布名称: oemXX.inf
:: 原始名称: KMDF2.inf
:: 提供者名称: MyCompany
:: 类名称: HIDClass
```

### 手动创建设备节点

如果 `devcon create` 失败，可以尝试：

```batch
:: 方法1: 使用正确的Class GUID
devcon create ROOT\KMDF2 "{745a17a0-74d3-11d0-b6fe-00a0c90f57da}" "VHF Virtual Mouse"

:: 方法2: 扫描硬件变化
devcon rescan

:: 方法3: 手动更新驱动
devcon update x64\Debug\KMDF2\KMDF2.inf ROOT\KMDF2
```

### 查看Windows事件日志

```powershell
# 查看系统日志中的PnP错误
Get-WinEvent -FilterHashtable @{
    LogName='System'
    ProviderName='Microsoft-Windows-Kernel-PnP'
    Level=2
} | Select-Object -First 10 | Format-List *

# 查看Service Control Manager日志
Get-WinEvent -FilterHashtable @{
    LogName='System'
    ProviderName='Service Control Manager'
} | Where-Object {$_.Message -like "*KMDF2*"} | Format-List *
```

## 常见错误代码含义

| 错误代码 | 含义 | 解决方案 |
|---------|------|---------|
| 0xc0000010 | STATUS_INVALID_DEVICE_REQUEST | 缺少mshidkmdf LowerFilters |
| 0xc000009a | STATUS_INSUFFICIENT_RESOURCES | 资源不足，重启系统 |
| 0x80070005 | ACCESS_DENIED | 需要管理员权限 |
| 0x80070002 | FILE_NOT_FOUND | 驱动文件不存在或路径错误 |

## 技术细节

### 为什么会出现这个问题？

1. **INF段落命名不匹配**: Windows PnP管理器在设备安装时查找 `[DeviceID.NT.HW]` 段中引用的 `AddReg` 段落。如果名称不匹配，注册表设置不会被应用。

2. **LowerFilters缺失**: VHF驱动依赖 `mshidkmdf` (Microsoft HID KMDF Framework) 作为下层过滤器来处理HID报告。如果没有这个过滤器，VHF无法正常工作。

3. **设备枚举时序**: 如果驱动包安装和设备创建之间的时序不对，可能导致驱动未正确绑定。

### VHF驱动的正确架构

```
用户模式应用
    ↓ IOCTL
KMDF驱动 (KMDF2.sys)
    ↓ VHF API
VHF框架
    ↓
mshidkmdf.sys (LowerFilters)  ← 必需！
    ↓
HIDClass驱动
    ↓
HID子系统
```

## 联系支持

如果以上所有步骤都无法解决问题，请提供：

1. **完整的DebugView日志** (从驱动加载到设备清理)
2. **clean_install.bat的完整输出**
3. **注册表导出**:
   ```batch
   reg export "HKLM\SYSTEM\CurrentControlSet\Enum\ROOT\KMDF2" kmdf2_enum.reg
   reg export "HKLM\SYSTEM\CurrentControlSet\Services\KMDF2" kmdf2_service.reg
   ```
4. **设备管理器截图** (显示设备属性和错误详情)
5. **Windows事件日志** (系统和应用程序日志中与KMDF2相关的条目)

## 参考文档

- [Microsoft VHF Documentation](https://learn.microsoft.com/zh-cn/windows-hardware/drivers/hid/virtual-hid-framework--vhf-)
- [KMDF Driver Development](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/)
- [INF File Sections](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/inf-file-sections)
