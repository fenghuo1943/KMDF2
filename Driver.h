#pragma once
#include <ntddk.h>
#include <wdf.h>
#include <vhf.h>

// IOCTL 鐎规矮绠?
#define IOCTL_VIRTUAL_MOUSE_MOVE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIRTUAL_MOUSE_CLICK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// 姒х姵鐖ｉ弫鐗堝祦閸栧懐绮ㄩ弸?
typedef struct _MOUSE_INPUT_DATA {
    CHAR dx;              // X鏉炲娴夌€靛湱些閸?
    CHAR dy;              // Y鏉炲娴夌€靛湱些閸?
    UCHAR buttons;        // 閹稿鎸抽悩鑸碘偓?(bit0=瀹革箓鏁? bit1=閸欐娊鏁? bit2=娑擃參鏁?
} MOUSE_INPUT_DATA, *PMOUSE_INPUT_DATA;

// 鐠佹儳顦稉濠佺瑓閺?
typedef struct _DEVICE_CONTEXT {
    VHFHANDLE VhfHandle;      // VHF閸欍儲鐒?
    BOOLEAN IsStarted;         // VHF閺勵垰鎯佸鎻掓儙閸?
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

// 閸戣姤鏆熸竟鐗堟
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtDeviceReleaseHardware;
EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtDeviceContextCleanup;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

// 鏉堝懎濮崙鑺ユ殶
NTSTATUS CreateAndStartVhf(PDEVICE_CONTEXT Context, WDFDEVICE Device);
VOID StopAndDeleteVhf(PDEVICE_CONTEXT Context);
VOID SendMouseReport(PDEVICE_CONTEXT Context, PMOUSE_INPUT_DATA MouseData);
