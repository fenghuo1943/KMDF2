#pragma once
#include <ntddk.h>
#include <wdf.h>
#include <vhf.h>

#define IOCTL_VIRTUAL_INPUT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _INPUT_PACKET {
    ULONG Type; // 1 = Mouse
    struct {
        CHAR dx;
        CHAR dy;
        UCHAR buttons;
    } mouse;
} INPUT_PACKET, * PINPUT_PACKET;

typedef struct _DEVICE_CONTEXT {
    VHFHANDLE VhfHandle;
} DEVICE_CONTEXT, * PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext);

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD VirtualMouseEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT VirtualMouseEvtSelfManagedIoInit;
EVT_WDF_OBJECT_CONTEXT_CLEANUP VirtualMouseEvtDeviceContextCleanup;
VOID SendMouseReport(PDEVICE_CONTEXT ctx, CHAR dx, CHAR dy, UCHAR buttons);
