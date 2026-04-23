#include "Driver.h"

const UCHAR g_ReportDescriptor[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x02,       // Usage (Mouse)
    0xA1, 0x01,       // Collection (Application)
    
    0x85, 0x01,       //   Report ID (1)
    
    0x09, 0x01,       //   Usage (Pointer)
    0xA1, 0x00,       //   Collection (Physical)
    

    0x05, 0x09,       //     Usage Page (Button)
    0x19, 0x01,       //     Usage Minimum (Button 1)
    0x29, 0x03,       //     Usage Maximum (Button 3)
    0x15, 0x00,       //     Logical Minimum (0)
    0x25, 0x01,       //     Logical Maximum (1)
    0x95, 0x03,       //     Report Count (3)
    0x75, 0x01,       //     Report Size (1)
    0x81, 0x02,       //     Input (Data, Variable, Absolute)
    

    0x95, 0x01,       //     Report Count (1)
    0x75, 0x05,       //     Report Size (5)
    0x81, 0x03,       //     Input (Constant, Variable, Absolute)
    

    0x05, 0x01,       //     Usage Page (Generic Desktop)
    0x09, 0x30,       //     Usage (X)
    0x09, 0x31,       //     Usage (Y)
    0x15, 0x81,       //     Logical Minimum (-127)
    0x25, 0x7F,       //     Logical Maximum (127)
    0x75, 0x08,       //     Report Size (8)
    0x95, 0x02,       //     Report Count (2)
    0x81, 0x06,       //     Input (Data, Variable, Relative)
    
    0xC0,             //   End Collection
    0xC0              // End Collection
};


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    KdPrint(("[VHF Driver] DriverEntry called\n"));


    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);
    

    status = WdfDriverCreate(DriverObject, 
                             RegistryPath, 
                             WDF_NO_OBJECT_ATTRIBUTES, 
                             &config, 
                             WDF_NO_HANDLE);
    
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] WdfDriverCreate failed: 0x%x\n", status));
    }
    
    return status;
}


NTSTATUS EvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);
    
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES attr;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    WDF_IO_QUEUE_CONFIG queueConfig;
    NTSTATUS status;

    KdPrint(("[VHF Driver] EvtDeviceAdd called\n"));


    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);


    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
    pnpCallbacks.EvtDeviceReleaseHardware = EvtDeviceReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, DEVICE_CONTEXT);
    attr.EvtCleanupCallback = EvtDeviceContextCleanup;


    status = WdfDeviceCreate(&DeviceInit, &attr, &device);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] WdfDeviceCreate failed: 0x%x\n", status));
        return status;
    }


    UNICODE_STRING symbolicLink;
    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\VirtualMouse");
    status = WdfDeviceCreateSymbolicLink(device, &symbolicLink);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] WdfDeviceCreateSymbolicLink failed: 0x%x\n", status));
        return status;
    }


    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;
    
    status = WdfIoQueueCreate(device, 
                              &queueConfig, 
                              WDF_NO_OBJECT_ATTRIBUTES, 
                              WDF_NO_HANDLE);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] WdfIoQueueCreate failed: 0x%x\n", status));
        return status;
    }

    KdPrint(("[VHF Driver] Device created successfully\n"));
    return STATUS_SUCCESS;
}


NTSTATUS EvtDevicePrepareHardware(WDFDEVICE Device, 
                                   WDFCMRESLIST ResourcesRaw, 
                                   WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PDEVICE_CONTEXT context = DeviceGetContext(Device);
    NTSTATUS status;

    KdPrint(("[VHF Driver] EvtDevicePrepareHardware called\n"));


    context->VhfHandle = NULL;
    context->IsStarted = FALSE;


    status = CreateAndStartVhf(context, Device);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] CreateAndStartVhf failed: 0x%x\n", status));
        return status;
    }

    KdPrint(("[VHF Driver] VHF started successfully\n"));
    return STATUS_SUCCESS;
}


NTSTATUS EvtDeviceReleaseHardware(WDFDEVICE Device, WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PDEVICE_CONTEXT context = DeviceGetContext(Device);

    KdPrint(("[VHF Driver] EvtDeviceReleaseHardware called\n"));


    StopAndDeleteVhf(context);

    return STATUS_SUCCESS;
}


VOID EvtDeviceContextCleanup(WDFOBJECT DeviceObject)
{
    WDFDEVICE device = (WDFDEVICE)DeviceObject;
    PDEVICE_CONTEXT context = DeviceGetContext(device);

    KdPrint(("[VHF Driver] EvtDeviceContextCleanup called\n"));


    StopAndDeleteVhf(context);
}


NTSTATUS CreateAndStartVhf(PDEVICE_CONTEXT Context, WDFDEVICE Device)
{
    VHF_CONFIG vhfConfig;
    NTSTATUS status;

    KdPrint(("[VHF Driver] Creating VHF...\n"));


    VHF_CONFIG_INIT(&vhfConfig,
                    WdfDeviceWdmGetDeviceObject(Device),
                    sizeof(g_ReportDescriptor),
                    (PUCHAR)g_ReportDescriptor);


    vhfConfig.VendorID = 0x1234;
    vhfConfig.ProductID = 0x5678;
    vhfConfig.VersionNumber = 0x0001;


    status = VhfCreate(&vhfConfig, &Context->VhfHandle);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] VhfCreate failed: 0x%x\n", status));
        Context->VhfHandle = NULL;
        return status;
    }

    KdPrint(("[VHF Driver] VhfCreate succeeded\n"));


    status = VhfStart(Context->VhfHandle);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] VhfStart failed: 0x%x\n", status));
        VhfDelete(Context->VhfHandle, TRUE);
        Context->VhfHandle = NULL;
        return status;
    }

    Context->IsStarted = TRUE;
    KdPrint(("[VHF Driver] VhfStart succeeded\n"));

    return STATUS_SUCCESS;
}


VOID StopAndDeleteVhf(PDEVICE_CONTEXT Context)
{
    if (Context->VhfHandle != NULL)
    {
        KdPrint(("[VHF Driver] Deleting VHF...\n"));
        

        VhfDelete(Context->VhfHandle, TRUE);
        Context->VhfHandle = NULL;
        Context->IsStarted = FALSE;
        
        KdPrint(("[VHF Driver] VHF deleted\n"));
    }
}


VOID EvtIoDeviceControl(WDFQUEUE Queue,
                        WDFREQUEST Request,
                        size_t OutputBufferLength,
                        size_t InputBufferLength,
                        ULONG IoControlCode)
{
    UNREFERENCED_PARAMETER(OutputBufferLength);
    
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_CONTEXT context;
    WDFDEVICE device;


    device = WdfIoQueueGetDevice(Queue);
    context = DeviceGetContext(device);

    switch (IoControlCode)
    {
        case IOCTL_VIRTUAL_MOUSE_MOVE:
        {
            PMOUSE_INPUT_DATA mouseData;
            size_t bytesReturned;

            
            if (InputBufferLength < sizeof(MOUSE_INPUT_DATA))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                KdPrint(("[VHF Driver] Buffer too small for MOUSE_INPUT_DATA\n"));
                break;
            }

            
            status = WdfRequestRetrieveInputBuffer(Request, 
                                                   sizeof(MOUSE_INPUT_DATA), 
                                                   &mouseData, 
                                                   &bytesReturned);
            if (!NT_SUCCESS(status))
            {
                KdPrint(("[VHF Driver] WdfRequestRetrieveInputBuffer failed: 0x%x\n", status));
                break;
            }


            SendMouseReport(context, mouseData);
            
            KdPrint(("[VHF Driver] Mouse move: dx=%d, dy=%d, buttons=0x%02X\n",
                     mouseData->dx, mouseData->dy, mouseData->buttons));
            break;
        }

        case IOCTL_VIRTUAL_MOUSE_CLICK:
        {
            PMOUSE_INPUT_DATA mouseData;
            size_t bytesReturned;

            if (InputBufferLength < sizeof(MOUSE_INPUT_DATA))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = WdfRequestRetrieveInputBuffer(Request,
                                                   sizeof(MOUSE_INPUT_DATA),
                                                   &mouseData,
                                                   &bytesReturned);
            if (!NT_SUCCESS(status))
            {
                KdPrint(("[VHF Driver] WdfRequestRetrieveInputBuffer failed: 0x%x\n", status));
                break;
            }


            SendMouseReport(context, mouseData);
            
            KdPrint(("[VHF Driver] Mouse click: buttons=0x%02X\n", mouseData->buttons));
            break;
        }

        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            KdPrint(("[VHF Driver] Invalid IOCTL: 0x%X\n", IoControlCode));
            break;
        }
    }


    WdfRequestComplete(Request, status);
}


VOID SendMouseReport(PDEVICE_CONTEXT Context, PMOUSE_INPUT_DATA MouseData)
{
    UCHAR report[4];
    HID_XFER_PACKET packet;
    NTSTATUS status;


    if (Context->VhfHandle == NULL || !Context->IsStarted)
    {
        KdPrint(("[VHF Driver] VHF not initialized or started\n"));
        return;
    }


    report[0] = 0x01;
    report[1] = MouseData->buttons;
    report[2] = MouseData->dx;
    report[3] = MouseData->dy;


    RtlZeroMemory(&packet, sizeof(packet));
    packet.reportBuffer = report;
    packet.reportBufferLen = sizeof(report);
    packet.reportId = 0x01;           // Report ID


    status = VhfReadReportSubmit(Context->VhfHandle, &packet);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] VhfReadReportSubmit failed: 0x%x\n", status));
    }
}
