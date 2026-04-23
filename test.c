#include <windows.h>
#include <iostream>

#define IOCTL_MOUSE_MOVE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    char dx;
    char dy;
} MOUSE_MOVE_DATA;

int main()
{
    // 1️⃣ 打开驱动
    HANDLE h = CreateFile(
        L"\\\\.\\VhfMouseCtl",
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (h == INVALID_HANDLE_VALUE) {
        std::cout << "打开设备失败\n";
        std::cout << "错误码: " << GetLastError() << std::endl;
        return 0;
    }

    std::cout << "打开成功\n";

    // 2️⃣ 构造数据
    MOUSE_MOVE_DATA data;
    data.dx = 50;
    data.dy = 0;

    // 3️⃣ 发送 IOCTL
    DWORD ret = 0;
    BOOL ok = DeviceIoControl(
        h,
        IOCTL_MOUSE_MOVE,
        &data,
        sizeof(data),
        NULL,
        0,
        &ret,
        NULL
    );

    if (!ok) {
        std::cout << "发送失败: " << GetLastError() << std::endl;
    } else {
        std::cout << "发送成功\n";
    }

    CloseHandle(h);
    return 0;
}