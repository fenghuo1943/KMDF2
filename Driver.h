#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <vhf.h>
#include <wdmsec.h>
//#include <hidport.h>

// Driver
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtDeviceContextCleanup;

// IOCTL 回调
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

// 全局 VHF handle
extern VHFHANDLE g_VhfHandle;


// ================= IOCTL 定义 =================

#define IOCTL_MOUSE_MOVE  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUSE_CLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUSE_WHEEL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_MULTI CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

// 鼠标移动
typedef struct _MOUSE_MOVE_DATA {
    CHAR dx;
    CHAR dy;
} MOUSE_MOVE_DATA, * PMOUSE_MOVE_DATA;

// 鼠标点击
typedef struct _MOUSE_CLICK_DATA {
    UCHAR button;
    UCHAR down;
} MOUSE_CLICK_DATA, * PMOUSE_CLICK_DATA;
// 鼠标滚轮
typedef struct _MOUSE_WHEEL_DATA {
    CHAR wheel;   // 正=上滚，负=下滚
} MOUSE_WHEEL_DATA, * PMOUSE_WHEEL_DATA;
// 键盘输入
typedef struct _KEYBOARD_MULTI_DATA {
    UCHAR modifier;
    UCHAR keyCount;
    UCHAR keys[6];
} KEYBOARD_MULTI_DATA, * PKEYBOARD_MULTI_DATA;

// ================= HID 报告结构 =================

typedef struct _MOUSE_REPORT {
    UCHAR ReportID;   // 1
    UCHAR Buttons;
    CHAR  X;
    CHAR  Y;
    CHAR  Wheel;      // 新增
} MOUSE_REPORT;
typedef struct _KEYBOARD_REPORT {
    UCHAR ReportID;   // =2
    UCHAR Modifier;
    UCHAR Reserved;
    UCHAR Key[6];
} KEYBOARD_REPORT;

// 创建控制设备
NTSTATUS CreateControlDevice(WDFDRIVER Driver);

// 发送鼠标数据
VOID SendMouseReport(CHAR dx, CHAR dy, CHAR wheel, UCHAR buttons);
VOID SendKeyboardReport(UCHAR modifier, UCHAR* keys, UCHAR keyCount);