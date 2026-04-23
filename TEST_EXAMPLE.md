# VHF 虚拟鼠标驱动 - 测试示例

## 驱动信息

- **驱动文件**: `x64\Debug\KMDF2.sys`
- **设备名称**: `\\DosDevices\\VirtualMouse`
- **设备ID**: `Root\KMDF2`
- **供应商ID**: `0x1234`
- **产品ID**: `0x5678`

## IOCTL 定义

```c
// 鼠标移动
#define IOCTL_VIRTUAL_MOUSE_MOVE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// 鼠标点击
#define IOCTL_VIRTUAL_MOUSE_CLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
```

## 数据结构

```c
typedef struct _MOUSE_INPUT_DATA {
    CHAR dx;              // X轴相对移动 (-127 to 127)
    CHAR dy;              // Y轴相对移动 (-127 to 127)
    UCHAR buttons;        // 按钮状态 (bit0=左键, bit1=右键, bit2=中键)
} MOUSE_INPUT_DATA, *PMOUSE_INPUT_DATA;
```

## C++ 测试示例

```cpp
#include <windows.h>
#include <iostream>

#define IOCTL_VIRTUAL_MOUSE_MOVE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIRTUAL_MOUSE_CLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _MOUSE_INPUT_DATA {
    CHAR dx;
    CHAR dy;
    UCHAR buttons;
} MOUSE_INPUT_DATA;

int main()
{
    // 打开设备
    HANDLE hDevice = CreateFileW(
        L"\\\\.\\VirtualMouse",
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        std::cout << "无法打开设备，错误代码: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "设备已打开，开始发送鼠标事件..." << std::endl;

    // 示例1: 鼠标向右移动 10 像素
    MOUSE_INPUT_DATA moveData = { 10, 0, 0 };
    DWORD bytesReturned;
    
    if (DeviceIoControl(hDevice, IOCTL_VIRTUAL_MOUSE_MOVE, 
                        &moveData, sizeof(moveData),
                        NULL, 0, &bytesReturned, NULL))
    {
        std::cout << "鼠标移动成功" << std::endl;
    }

    // 示例2: 鼠标左键按下
    MOUSE_INPUT_DATA downData = { 0, 0, 0x01 };  // bit0 = 左键
    DeviceIoControl(hDevice, IOCTL_VIRTUAL_MOUSE_CLICK,
                    &downData, sizeof(downData),
                    NULL, 0, &bytesReturned, NULL);
    
    Sleep(100);  // 保持按下100ms

    // 示例3: 鼠标左键释放
    MOUSE_INPUT_DATA upData = { 0, 0, 0x00 };
    DeviceIoControl(hDevice, IOCTL_VIRTUAL_MOUSE_CLICK,
                    &upData, sizeof(upData),
                    NULL, 0, &bytesReturned, NULL);

    CloseHandle(hDevice);
    std::cout << "测试完成" << std::endl;
    return 0;
}
```

## Python 测试示例

```python
import ctypes
from ctypes import wintypes
import time

# 定义常量
FILE_DEVICE_UNKNOWN = 0x00000022
METHOD_BUFFERED = 0
FILE_ANY_ACCESS = 0

def CTL_CODE(device_type, function, method, access):
    return (device_type << 16) | (access << 14) | (function << 2) | method

IOCTL_VIRTUAL_MOUSE_MOVE = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_VIRTUAL_MOUSE_CLICK = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

# 加载Windows API
kernel32 = ctypes.windll.kernel32

# 打开设备
hDevice = kernel32.CreateFileW(
    "\\\\.\\VirtualMouse",
    0x40000000,  # GENERIC_WRITE
    0,
    None,
    3,  # OPEN_EXISTING
    0,
    None
)

if hDevice == -1:
    print(f"无法打开设备，错误代码: {kernel32.GetLastError()}")
    exit(1)

print("设备已打开")

# 定义鼠标数据结构
class MOUSE_INPUT_DATA(ctypes.Structure):
    _fields_ = [
        ("dx", ctypes.c_char),
        ("dy", ctypes.c_char),
        ("buttons", ctypes.c_ubyte)
    ]

# 示例1: 鼠标移动
move_data = MOUSE_INPUT_DATA(dx=ctypes.c_char(20), dy=ctypes.c_char(0), buttons=0)
bytes_returned = wintypes.DWORD()

result = kernel32.DeviceIoControl(
    hDevice,
    IOCTL_VIRTUAL_MOUSE_MOVE,
    ctypes.byref(move_data),
    ctypes.sizeof(move_data),
    None,
    0,
    ctypes.byref(bytes_returned),
    None
)

if result:
    print("鼠标移动成功")

# 示例2: 鼠标点击
click_data = MOUSE_INPUT_DATA(dx=ctypes.c_char(0), dy=ctypes.c_char(0), buttons=1)
kernel32.DeviceIoControl(
    hDevice,
    IOCTL_VIRTUAL_MOUSE_CLICK,
    ctypes.byref(click_data),
    ctypes.sizeof(click_data),
    None,
    0,
    ctypes.byref(bytes_returned),
    None
)

time.sleep(0.1)

# 释放按键
release_data = MOUSE_INPUT_DATA(dx=ctypes.c_char(0), dy=ctypes.c_char(0), buttons=0)
kernel32.DeviceIoControl(
    hDevice,
    IOCTL_VIRTUAL_MOUSE_CLICK,
    ctypes.byref(release_data),
    ctypes.sizeof(release_data),
    None,
    0,
    ctypes.byref(bytes_returned),
    None
)

# 关闭设备
kernel32.CloseHandle(hDevice)
print("测试完成")
```

## 按钮位映射

| 位 | 按钮 | 值 |
|---|------|-----|
| 0 | 左键 | 0x01 |
| 1 | 右键 | 0x02 |
| 2 | 中键 | 0x04 |

例如：
- 左键+右键同时按下: `buttons = 0x03`
- 三个键都按下: `buttons = 0x07`

## 安装驱动

使用管理员权限运行：

```batch
install.bat
```

## 卸载驱动

```batch
pnputil /remove-device ROOT\KMDF2\0000
sc delete KMDF2
```

## 调试输出

驱动会输出详细的调试信息，可以使用 DebugView 工具查看：
https://docs.microsoft.com/en-us/sysinternals/downloads/debugview

调试信息包括：
- VHF 创建和启动状态
- 鼠标移动和点击事件
- 错误信息
