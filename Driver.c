#include "Driver.h"
//#include <hidport.h>
#pragma comment(lib, "VhfKm.lib")

const UCHAR g_ReportDescriptor[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x02,       // Usage (Mouse)
    0xA1, 0x01,       // Collection (Application)

    0x85, 0x01,

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
    0x15, 0x81,
    0x25, 0x7F,
    0x75, 0x08,
    0x95, 0x02,
    0x81, 0x06,

    0xC0,
    0xC0
};



NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, VirtualMouseEvtDeviceAdd);

    return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
}
NTSTATUS VirtualMouseEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);

    //WdfFdoInitSetFilter(DeviceInit);

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, DEVICE_CONTEXT);

    WDFDEVICE device;
    NTSTATUS status = WdfDeviceCreate(&DeviceInit, &attr, &device);
    if (!NT_SUCCESS(status)) return status;

    PDEVICE_CONTEXT ctx = DeviceGetContext(device);

    UNICODE_STRING sym;
    RtlInitUnicodeString(&sym, L"\\DosDevices\\VirtualMouse");
    WdfDeviceCreateSymbolicLink(device, &sym);

    WDF_IO_QUEUE_CONFIG q;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&q, WdfIoQueueDispatchParallel);
    q.EvtIoDeviceControl = EvtIoDeviceControl;
    WdfIoQueueCreate(device, &q, WDF_NO_OBJECT_ATTRIBUTES, NULL);

    VHF_CONFIG vhfConfig;
    VHF_CONFIG_INIT(&vhfConfig,
        WdfDeviceWdmGetPhysicalDevice(device),
        sizeof(g_ReportDescriptor),
        (PUCHAR)g_ReportDescriptor
    );
    vhfConfig.VendorID = 0x1234;
    vhfConfig.ProductID = 0x5678;
    vhfConfig.VersionNumber = 0x0001;

    KdPrint(("Before VhfCreate\n"));
    status = VhfCreate(&vhfConfig, &ctx->VhfHandle);
    KdPrint(("VhfCreate status = 0x%x\n", status));

    if (!NT_SUCCESS(status)) return status;

    status = VhfStart(ctx->VhfHandle);
    KdPrint(("VhfStart status = 0x%x\n", status));
    if (!NT_SUCCESS(status)) return status;

    KdPrint(("VhfStart VHF started\n"));

    return status;
}
VOID EvtIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
)
{
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    NTSTATUS status = STATUS_SUCCESS;

    if (IoControlCode == IOCTL_VIRTUAL_INPUT)
    {
        PINPUT_PACKET pkt;
        size_t len;

        status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(INPUT_PACKET),
            &pkt,
            &len
        );

        if (NT_SUCCESS(status))
        {
            PDEVICE_CONTEXT ctx = DeviceGetContext(
                WdfIoQueueGetDevice(Queue)
            );

            SendMouseReport(ctx, pkt->mouse.dx, pkt->mouse.dy, pkt->mouse.buttons);

            KdPrint(("MOVE: X=%d Y=%d\n", pkt->mouse.dx, pkt->mouse.dy));
        }
    }
    else
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    WdfRequestComplete(Request, status);
}
VOID SendMouseReport(PDEVICE_CONTEXT ctx, CHAR dx, CHAR dy, UCHAR buttons)
{
    UCHAR report[4];

    report[0] = 0x01;
    report[0] = buttons;
    report[1] = dx;
    report[2] = dy;

    HID_XFER_PACKET packet;

    packet.reportBuffer = report;
    packet.reportBufferLen = sizeof(report);
    packet.reportId = 0x01;

    VhfReadReportSubmit(ctx->VhfHandle, &packet);
}