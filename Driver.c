#include "Driver.h"

// 姒х姵鐖ｉ幎銉ユ啞閹诲繗鍫粭?(50鐎涙濡?
const UCHAR g_ReportDescriptor[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x02,       // Usage (Mouse)
    0xA1, 0x01,       // Collection (Application)
    
    0x85, 0x01,       //   Report ID (1)
    
    0x09, 0x01,       //   Usage (Pointer)
    0xA1, 0x00,       //   Collection (Physical)
    
    // 閹稿鎸?(3娴?
    0x05, 0x09,       //     Usage Page (Button)
    0x19, 0x01,       //     Usage Minimum (Button 1)
    0x29, 0x03,       //     Usage Maximum (Button 3)
    0x15, 0x00,       //     Logical Minimum (0)
    0x25, 0x01,       //     Logical Maximum (1)
    0x95, 0x03,       //     Report Count (3)
    0x75, 0x01,       //     Report Size (1)
    0x81, 0x02,       //     Input (Data, Variable, Absolute)
    
    // 婵夘偄鍘?(5娴?
    0x95, 0x01,       //     Report Count (1)
    0x75, 0x05,       //     Report Size (5)
    0x81, 0x03,       //     Input (Constant, Variable, Absolute)
    
    // X/Y 閸ф劖鐖?(閸?娴ｅ稄绱濋張澶岊儊閸?
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

// ============================================================================
// DriverEntry - 妞瑰崬濮╃粙瀣碍閸忋儱褰涢悙?
// ============================================================================
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    KdPrint(("[VHF Driver] DriverEntry called\n"));

    // 閸掓繂顫愰崠鏍攳閸斻劑鍘ょ純?
    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);
    
    // 閸掓稑缂揥DF妞瑰崬濮╃€电钖?
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

// ============================================================================
// EvtDeviceAdd - 鐠佹儳顦ǎ璇插閸ョ偠鐨?
// ============================================================================
NTSTATUS EvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);
    
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES attr;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    WDF_IO_QUEUE_CONFIG queueConfig;
    NTSTATUS status;

    KdPrint(("[VHF Driver] EvtDeviceAdd called\n"));

    // 鐠佸墽鐤嗙拋鎯ь槵缁鐎?
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);

    // 閸掓繂顫愰崠鏈燦P/Power娴滃娆㈤崶鐐剁殶
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
    pnpCallbacks.EvtDeviceReleaseHardware = EvtDeviceReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);

    // 閸掓繂顫愰崠鏍啎婢跺洣绗傛稉瀣瀮鐏炵偞鈧?
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, DEVICE_CONTEXT);
    attr.EvtCleanupCallback = EvtDeviceContextCleanup;

    // 閸掓稑缂撶拋鎯ь槵鐎电钖?
    status = WdfDeviceCreate(&DeviceInit, &attr, &device);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] WdfDeviceCreate failed: 0x%x\n", status));
        return status;
    }

    // 閸掓稑缂撶拋鎯ь槵閹恒儱褰涚粭锕€褰块柧鐐复
    UNICODE_STRING symbolicLink;
    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\VirtualMouse");
    status = WdfDeviceCreateSymbolicLink(device, &symbolicLink);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] WdfDeviceCreateSymbolicLink failed: 0x%x\n", status));
        return status;
    }

    // 閸掓稑缂撴妯款吇IO闂冪喎鍨?
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

// ============================================================================
// EvtDevicePrepareHardware - 绾兛娆㈤崙鍡楊槵閸ョ偠鐨?
// ============================================================================
NTSTATUS EvtDevicePrepareHardware(WDFDEVICE Device, 
                                   WDFCMRESLIST ResourcesRaw, 
                                   WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PDEVICE_CONTEXT context = DeviceGetContext(Device);
    NTSTATUS status;

    KdPrint(("[VHF Driver] EvtDevicePrepareHardware called\n"));

    // 閸掓繂顫愰崠鏍︾瑐娑撳鏋?
    context->VhfHandle = NULL;
    context->IsStarted = FALSE;

    // 閸掓稑缂撻獮璺烘儙閸?VHF
    status = CreateAndStartVhf(context, Device);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] CreateAndStartVhf failed: 0x%x\n", status));
        return status;
    }

    KdPrint(("[VHF Driver] VHF started successfully\n"));
    return STATUS_SUCCESS;
}

// ============================================================================
// EvtDeviceReleaseHardware - 绾兛娆㈤柌濠冩杹閸ョ偠鐨?
// ============================================================================
NTSTATUS EvtDeviceReleaseHardware(WDFDEVICE Device, WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PDEVICE_CONTEXT context = DeviceGetContext(Device);

    KdPrint(("[VHF Driver] EvtDeviceReleaseHardware called\n"));

    // 閸嬫粍顒涢獮璺哄灩闂?VHF
    StopAndDeleteVhf(context);

    return STATUS_SUCCESS;
}

// ============================================================================
// EvtDeviceContextCleanup - 鐠佹儳顦稉濠佺瑓閺傚洦绔婚悶鍡楁礀鐠?
// ============================================================================
VOID EvtDeviceContextCleanup(WDFOBJECT DeviceObject)
{
    WDFDEVICE device = (WDFDEVICE)DeviceObject;
    PDEVICE_CONTEXT context = DeviceGetContext(device);

    KdPrint(("[VHF Driver] EvtDeviceContextCleanup called\n"));

    // 绾喕绻?VHF 瀹稿弶绔婚悶?
    StopAndDeleteVhf(context);
}

// ============================================================================
// CreateAndStartVhf - 閸掓稑缂撻獮璺烘儙閸?VHF
// ============================================================================
NTSTATUS CreateAndStartVhf(PDEVICE_CONTEXT Context, WDFDEVICE Device)
{
    VHF_CONFIG vhfConfig;
    NTSTATUS status;

    KdPrint(("[VHF Driver] Creating VHF...\n"));

    // 閸掓繂顫愰崠?VHF 闁板秶鐤?
    VHF_CONFIG_INIT(&vhfConfig,
                    WdfDeviceWdmGetDeviceObject(Device),
                    sizeof(g_ReportDescriptor),
                    (PUCHAR)g_ReportDescriptor);

    // 鐠佸墽鐤?HID 鐠佹儳顦仦鐐粹偓?
    vhfConfig.VendorID = 0x1234;          // 娓氭稑绨查崯?ID
    vhfConfig.ProductID = 0x5678;         // 娴溠冩惂 ID
    vhfConfig.VersionNumber = 0x0001;     // 閻楀牊婀伴崣?

    // 閸掓稑缂?VHF 鐎电钖?
    status = VhfCreate(&vhfConfig, &Context->VhfHandle);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] VhfCreate failed: 0x%x\n", status));
        Context->VhfHandle = NULL;
        return status;
    }

    KdPrint(("[VHF Driver] VhfCreate succeeded\n"));

    // 閸氼垰濮?VHF
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

// ============================================================================
// StopAndDeleteVhf - 閸嬫粍顒涢獮璺哄灩闂?VHF
// ============================================================================
VOID StopAndDeleteVhf(PDEVICE_CONTEXT Context)
{
    if (Context->VhfHandle != NULL)
    {
        KdPrint(("[VHF Driver] Deleting VHF...\n"));
        
        // 閸掔娀娅?VHF 鐎电钖?(WaitForComplete=TRUE 娴兼氨鐡戝鍛閺堝鎼锋担婊冪暚閹?
        VhfDelete(Context->VhfHandle, TRUE);
        Context->VhfHandle = NULL;
        Context->IsStarted = FALSE;
        
        KdPrint(("[VHF Driver] VHF deleted\n"));
    }
}

// ============================================================================
// EvtIoDeviceControl - IO閹貉冨煑閸ョ偠鐨?
// ============================================================================
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

    // 閼惧嘲褰囩拋鎯ь槵娑撳﹣绗呴弬?
    device = WdfIoQueueGetDevice(Queue);
    context = DeviceGetContext(device);

    switch (IoControlCode)
    {
        case IOCTL_VIRTUAL_MOUSE_MOVE:
        {
            PMOUSE_INPUT_DATA mouseData;
            size_t bytesReturned;

            // 妤犲矁鐦夋潏鎾冲弳缂傛挸鍟块崠鍝勩亣鐏?
            if (InputBufferLength < sizeof(MOUSE_INPUT_DATA))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                KdPrint(("[VHF Driver] Buffer too small for MOUSE_INPUT_DATA\n"));
                break;
            }

            // 閼惧嘲褰囨潏鎾冲弳缂傛挸鍟块崠?
            status = WdfRequestRetrieveInputBuffer(Request, 
                                                   sizeof(MOUSE_INPUT_DATA), 
                                                   &mouseData, 
                                                   &bytesReturned);
            if (!NT_SUCCESS(status))
            {
                KdPrint(("[VHF Driver] WdfRequestRetrieveInputBuffer failed: 0x%x\n", status));
                break;
            }

            // 閸欐垿鈧線绱堕弽鍥ㄥГ閸?
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

            // 閸欐垿鈧線绱堕弽鍥╁仯閸戠粯濮ら崨?
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

    // 鐎瑰本鍨氱拠閿嬬湴
    WdfRequestComplete(Request, status);
}

// ============================================================================
// SendMouseReport - 閸欐垿鈧線绱堕弽鍥ㄥГ閸?
// ============================================================================
VOID SendMouseReport(PDEVICE_CONTEXT Context, PMOUSE_INPUT_DATA MouseData)
{
    UCHAR report[4];
    HID_XFER_PACKET packet;
    NTSTATUS status;

    // 妤犲矁鐦?VHF 閸欍儲鐒?
    if (Context->VhfHandle == NULL || !Context->IsStarted)
    {
        KdPrint(("[VHF Driver] VHF not initialized or started\n"));
        return;
    }

    // 閺嬪嫬缂撴Η鐘崇垼閹躲儱鎲?(Report ID + Buttons + X + Y)
    report[0] = 0x01;                 // Report ID (韫囧懘銆忔稉搴㈠Г閸涘﹥寮挎潻鎵儊閸栧綊鍘?
    report[1] = MouseData->buttons;   // 閹稿鎸抽悩鑸碘偓?
    report[2] = MouseData->dx;        // X 鏉炲些閸?
    report[3] = MouseData->dy;        // Y 鏉炲些閸?

    // 閸掓繂顫愰崠?HID 娴肩姾绶崠?
    RtlZeroMemory(&packet, sizeof(packet));
    packet.reportBuffer = report;
    packet.reportBufferLen = sizeof(report);
    packet.reportId = 0x01;           // Report ID

    // 閹绘劒姘﹂幎銉ユ啞閸?VHF
    status = VhfReadReportSubmit(Context->VhfHandle, &packet);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("[VHF Driver] VhfReadReportSubmit failed: 0x%x\n", status));
    }
}
