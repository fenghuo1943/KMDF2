#include "driver.h"
#include <wdmsec.h>

#pragma comment(lib, "vhfkm.lib")
#pragma comment(lib, "Wdmsec.lib")

VHFHANDLE g_VhfHandle = NULL;
WDFDEVICE g_ControlDevice = NULL;

// ================= HID Descriptor =================

UCHAR g_HidReportDescriptor[] = {

    // ================= Mouse (Report ID 1) =================
    0x05, 0x01,
    0x09, 0x02,
    0xA1, 0x01,
    0x85, 0x01,        // Report ID = 1

    0x09, 0x01,
    0xA1, 0x00,

    0x05, 0x09,
    0x19, 0x01,
    0x29, 0x03,
    0x15, 0x00,
    0x25, 0x01,
    0x95, 0x03,
    0x75, 0x01,
    0x81, 0x02,

    0x95, 0x01,
    0x75, 0x05,
    0x81, 0x03,

    0x05, 0x01,
    0x09, 0x30,
    0x09, 0x31,
    0x09, 0x38,        // Wheel
    0x15, 0x81,        // Logical Min (-127)
    0x25, 0x7F,        // Logical Max (127)
    0x75, 0x08,
    0x95, 0x01,
    0x81, 0x06,
    0x15, 0x81,
    0x25, 0x7F,
    0x75, 0x08,
    0x95, 0x02,
    0x81, 0x06,

    0xC0,
    0xC0,

    // ================= Keyboard (Report ID 2) =================
    0x05, 0x01,
    0x09, 0x06,
    0xA1, 0x01,
    0x85, 0x02,        // Report ID = 2

    0x05, 0x07,
    0x19, 0xE0,
    0x29, 0xE7,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x08,
    0x81, 0x02,        // modifier

    0x95, 0x01,
    0x75, 0x08,
    0x81, 0x03,        // reserved

    0x95, 0x06,
    0x75, 0x08,
    0x15, 0x00,
    0x25, 0x65,
    0x05, 0x07,
    0x19, 0x00,
    0x29, 0x65,
    0x81, 0x00,        // key array

    0xC0
};

// ================= DriverEntry =================

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    KdPrint(("[VHF Driver] DriverEntry\n"));

    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    );

    if (!NT_SUCCESS(status)) {
        KdPrint(("[VHF Driver] WdfDriverCreate failed: %x\n", status));
        return status;
    }

    // 创建用户态控制接口
    status = CreateControlDevice(WdfGetDriver());
    if (!NT_SUCCESS(status)) {
        KdPrint(("[VHF Driver] CreateControlDevice failed: %x\n", status));
    }

    return status;
}

// ================= 创建设备（VHF） =================

NTSTATUS EvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS status;
    WDFDEVICE device;
    VHF_CONFIG vhfConfig;

    UNREFERENCED_PARAMETER(Driver);

    KdPrint(("[VHF Driver] EvtDeviceAdd\n"));

    //status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = EvtDeviceContextCleanup;

    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("[VHF Driver] WdfDeviceCreate failed: %x\n", status));
        return status;
    }

    VHF_CONFIG_INIT(
        &vhfConfig,
        WdfDeviceWdmGetDeviceObject(device),
        sizeof(g_HidReportDescriptor),
        g_HidReportDescriptor
    );

    vhfConfig.VendorID = 0x1234;
    vhfConfig.ProductID = 0x5678;
    vhfConfig.VersionNumber = 1;

    status = VhfCreate(&vhfConfig, &g_VhfHandle);
    if (!NT_SUCCESS(status)) {
        KdPrint(("[VHF Driver] VhfCreate failed: %x\n", status));
        return status;
    }

    status = VhfStart(g_VhfHandle);
    if (!NT_SUCCESS(status)) {
        KdPrint(("[VHF Driver] VhfStart failed: %x\n", status));
        return status;
    }

    KdPrint(("[VHF Driver] VHF started\n"));

    return STATUS_SUCCESS;
}

// ================= 发送 HID 报告 =================

VOID SendMouseReport(CHAR dx, CHAR dy, CHAR wheel, UCHAR buttons)
{
    if (!g_VhfHandle)
        return;

    MOUSE_REPORT report;
    RtlZeroMemory(&report, sizeof(report));

    report.ReportID = 1;
    report.Buttons = buttons;
    report.X = dx;
    report.Y = dy;
    report.Wheel = wheel;

    HID_XFER_PACKET packet;
    RtlZeroMemory(&packet, sizeof(packet));

    packet.reportBuffer = (PUCHAR)&report;
    packet.reportBufferLen = sizeof(report);
    packet.reportId = 1;
    KdPrint(("[VHF Driver] move: %d\n", report.X));
    VhfReadReportSubmit(g_VhfHandle, &packet);
}
VOID SendKeyboardReport(UCHAR modifier, UCHAR* keys, UCHAR keyCount)
{
    if (!g_VhfHandle)
        return;

    KEYBOARD_REPORT report;
    RtlZeroMemory(&report, sizeof(report));

    report.ReportID = 2;
    report.Modifier = modifier;

    for (int i = 0; i < keyCount && i < 6; i++) {
        report.Key[i] = keys[i];
    }

    HID_XFER_PACKET packet;
    RtlZeroMemory(&packet, sizeof(packet));

    packet.reportBuffer = (PUCHAR)&report;
    packet.reportBufferLen = sizeof(report);
    packet.reportId = 2;

    VhfReadReportSubmit(g_VhfHandle, &packet);
}

// ================= IOCTL 处理 =================

VOID EvtIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    NTSTATUS status = STATUS_SUCCESS;

    switch (IoControlCode)
    {
    case IOCTL_MOUSE_MOVE:
    {
        PMOUSE_MOVE_DATA data = NULL;

        status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(MOUSE_MOVE_DATA),
            (PVOID*)&data,
            NULL
        );

        if (NT_SUCCESS(status)) {
            SendMouseReport(data->dx, data->dy, 0, 0);
        }
        break;
    }

    case IOCTL_MOUSE_CLICK:
    {
        PMOUSE_CLICK_DATA data = NULL;

        status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(MOUSE_CLICK_DATA),
            (PVOID*)&data,
            NULL
        );

        if (NT_SUCCESS(status)) {
            if (data->down) {
                SendMouseReport(0, 0, 0, data->button);
            }
            else {
                SendMouseReport(0, 0, 0, 0);
            }
        }
        break;
    }

    case IOCTL_MOUSE_WHEEL:
    {
        PMOUSE_WHEEL_DATA data = NULL;

        status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(MOUSE_WHEEL_DATA),
            (PVOID*)&data,
            NULL
        );

        if (NT_SUCCESS(status)) {
            SendMouseReport(0, 0, data->wheel, 0);
            SendMouseReport(0, 0, 0, 0); // 清零
        }
        break;
    }

    case IOCTL_KEYBOARD_MULTI:
    {
        PKEYBOARD_MULTI_DATA data = NULL;

        status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(KEYBOARD_MULTI_DATA),
            (PVOID*)&data,
            NULL
        );

        if (NT_SUCCESS(status)) {

            SendKeyboardReport(data->modifier, data->keys, data->keyCount);

            // 松开（非常重要）
            UCHAR empty[6] = { 0 };
            SendKeyboardReport(0, empty, 0);
        }
        break;
    }

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestComplete(Request, status);
}

// ================= 控制设备 =================

NTSTATUS CreateControlDevice(WDFDRIVER Driver)
{
    PWDFDEVICE_INIT pInit = NULL;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    NTSTATUS status;
    UNICODE_STRING devName, symLink;
    UNICODE_STRING sddl;

    RtlInitUnicodeString(&sddl,
        L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGWGX;;;WD)");
    //pInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);
    pInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);
    if (!pInit)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlInitUnicodeString(&devName, L"\\Device\\FenghuoVHF");
    status = WdfDeviceInitAssignName(pInit, &devName);
    if (!NT_SUCCESS(status))
        return status;

    status = WdfDeviceCreate(&pInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    if (!NT_SUCCESS(status))
        return status;

    RtlInitUnicodeString(&symLink, L"\\DosDevices\\FenghuoVHF");
    status = WdfDeviceCreateSymbolicLink(device, &symLink);
    if (!NT_SUCCESS(status))
        return status;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchSequential
    );

    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;

    status = WdfIoQueueCreate(
        device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        NULL
    );

    if (!NT_SUCCESS(status))
        return status;

    WdfControlFinishInitializing(device);

    g_ControlDevice = device;

    return STATUS_SUCCESS;
}
VOID EvtDeviceContextCleanup(WDFOBJECT DeviceObject)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KdPrint(("[VHF Driver] Cleanup called\n"));

    if (g_VhfHandle)
    {
        VhfDelete(g_VhfHandle, TRUE);  // TRUE = 同步删除（必须）
        g_VhfHandle = NULL;

        KdPrint(("[VHF Driver] VhfDelete done\n"));
    }
}