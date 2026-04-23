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

// ================= HID 报告结构 =================

typedef struct _MOUSE_REPORT {
    UCHAR Buttons;
    CHAR  X;
    CHAR  Y;
} MOUSE_REPORT;

// 创建控制设备
NTSTATUS CreateControlDevice(WDFDRIVER Driver);

// 发送鼠标数据
VOID SendMouseReport(CHAR dx, CHAR dy, UCHAR buttons);