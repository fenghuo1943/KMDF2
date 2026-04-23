# 代码10错误 (STATUS_DEVICE_DATA_ERROR) 修复指南

## 问题描述
设备安装时出现"该设备无法启动。(代码 10)"错误，状态码为 STATUS_DEVICE_DATA_ERROR。

## 已修复的问题

### 1. INF文件 LowerFilters 类型错误 ✅
**问题**: `HKR,,"LowerFilters",0x00010008,"mshidkmdf"`  
**修复**: `HKR,,"LowerFilters",0x00010000,"mshidkmdf"`

- `0x00010000` = FLG_ADDREG_TYPE_MULTI_SZ (正确的多字符串类型)
- `0x00010008` 是错误的类型值

### 2. 报告描述符变量名不匹配 ✅
**问题**: 变量定义为 `HeadSetReportDescriptor`，但代码中使用 `g_ReportDescriptor`  
**修复**: 统一使用 `g_ReportDescriptor`

### 3. 报告描述符内容错误 ✅
**问题**: 使用了不完整的耳机按钮描述符（只有1字节）  
**修复**: 恢复为标准的鼠标报告描述符（4字节：Report ID + Buttons + X + Y）

## 重新安装驱动步骤

### 步骤1: 卸载旧驱动
```batch
:: 以管理员身份运行
pnputil /remove-device ROOT\KMDF2\0000
sc delete KMDF2
```

### 步骤2: 清理设备管理器
1. 打开设备管理器 (devmgmt.msc)
2. 查看 -> 显示隐藏的设备
3. 找到灰色的 "VHF Virtual Mouse Device"
4. 右键 -> 卸载设备 -> 勾选"删除此设备的驱动程序软件"

### 步骤3: 安装新驱动
```batch
:: 以管理员身份运行
install.bat
```

### 步骤4: 验证安装
```batch
:: 检查设备状态
pnputil /enum-devices /instanceid ROOT\KMDF2\*

:: 应该看到类似输出：
:: 实例 ID: ROOT\KMDF2\0000
:: 类名称: HIDClass
:: 状态: Started
```

## 调试方法

### 方法1: 使用 DebugView 查看日志
1. 下载 DebugView: https://docs.microsoft.com/en-us/sysinternals/downloads/debugview
2. 以管理员身份运行
3. 启用 "Capture Kernel" (Ctrl+K)
4. 重新安装驱动
5. 查看输出，应该看到：
   ```
   [VHF Driver] DriverEntry called
   [VHF Driver] EvtDeviceAdd called
   [VHF Driver] EvtDevicePrepareHardware called
   [VHF Driver] Creating VHF...
   [VHF Driver] Report descriptor size: 37 bytes
   [VHF Driver] Calling VhfCreate...
   [VHF Driver] VhfCreate succeeded, handle: 0x...
   [VHF Driver] Calling VhfStart...
   [VHF Driver] VHF started successfully
   ```

### 方法2: 检查事件查看器
1. 打开事件查看器 (eventvwr.msc)
2. Windows 日志 -> 系统
3. 查找来源为 "Service Control Manager" 的错误

### 方法3: 检查注册表
```batch
:: 查看 LowerFilters 是否正确设置
reg query "HKLM\SYSTEM\CurrentControlSet\Enum\ROOT\KMDF2\0000" /v LowerFilters
```

应该看到：
```
LowerFilters    REG_MULTI_SZ    mshidkmdf
```

## 如果问题仍然存在

### 检查清单
- [ ] INF文件中 LowerFilters 使用 `0x00010000`
- [ ] 报告描述符完整且正确（37字节）
- [ ] 驱动文件已重新编译
- [ ] 旧驱动已完全卸载
- [ ] 以管理员身份安装

### 常见错误代码
- `0xc0000010` (STATUS_INVALID_DEVICE_REQUEST): 缺少 mshidkmdf LowerFilters
- `0xc000009a` (STATUS_INSUFFICIENT_RESOURCES): 资源不足，重启系统
- `0x80070005` (ACCESS_DENIED): 需要管理员权限

### 获取详细错误信息
```batch
:: 获取设备详细状态
powershell "Get-PnpDevice -InstanceId 'ROOT\KMDF2\*' | Format-List *"

:: 查看驱动加载日志
powershell "Get-WinEvent -FilterHashtable @{LogName='System'; ProviderName='Microsoft-Windows-Kernel-PnP'} | Select-Object -First 20"
```

## 测试驱动功能

安装成功后，可以使用以下PowerShell脚本测试：

```powershell
# 打开设备句柄
$devicePath = "\\.\VirtualMouse"

try {
    # 这里需要使用C++或C#程序发送IOCTL
    Write-Host "设备已就绪，请使用测试程序发送鼠标事件"
} catch {
    Write-Host "错误: $_"
}
```

或使用提供的 C++/Python 测试示例（见 TEST_EXAMPLE.md）

## 联系支持

如果以上步骤都无法解决问题，请提供：
1. DebugView 的完整日志输出
2. 设备管理器中的设备状态截图
3. `pnputil /enum-devices /instanceid ROOT\KMDF2\*` 的输出
4. 事件查看器中的相关错误信息
