#include <ntddk.h>
#include <wdm.h>
#include <hidport.h>
#include <usbioctl.h>
#include <wchar.h>

#define PDX(fdo) ((PDEVICE_EXTENSION) (((PHID_DEVICE_EXTENSION)(fdo)->DeviceExtension)->MiniDeviceExtension))
#define PDO(fdo) (((PHID_DEVICE_EXTENSION) ((fdo)->DeviceExtension))->PhysicalDeviceObject)
#define LDO(fdo) (((PHID_DEVICE_EXTENSION) ((fdo)->DeviceExtension))->NextDeviceObject)

typedef struct _DEVICE_EXTENSION {
    NTSTATUS AddDeviceStatus;
    DEVICE_POWER_STATE devpower;
    PDEVICE_OBJECT LowerDeviceObject;
    void* pgx;
} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

char ReportDescriptor[64] = {
 0x05, 0x05, // USAGE_PAGE (Gaming Controls)
 0x09, 0x03, // USAGE (Gun Device )
 0xa1, 0x01, // COLLECTION (Application)
 0xa1, 0x02, // COLLECTION (Logical)
 0x85, 0x01, // REPORT_ID (1)
 0x05, 0x09, // USAGE_PAGE (Button)
 0x09, 0x01, // USAGE (Button 1)
 0x15, 0x00, // LOGICAL_MINIMUM (0)
 0x25, 0x01, // LOGICAL_MAXIMUM (1)
 0x75, 0x01, // REPORT_SIZE (1)
 0x95, 0x01, // REPORT_COUNT (1)
 0x81, 0x02, // INPUT (Data,Var,Abs)
 0x75, 0x07, // REPORT_SIZE (7)
 0x81, 0x03, // INPUT (Cnst,Var,Abs)
 0xc0, // END_COLLECTION
 0xa1, 0x02, // COLLECTION (Logical)
 0x85, 0x02, // REPORT_ID (2)
 0x05, 0x01, // USAGE_PAGE (Generic Desktop)
 0x09, 0x30, // USAGE (X)
 0x25, 0xff, // LOGICAL_MAXIMUM (-1)
 0x75, 0x20, // REPORT_SIZE (32)
 0xb1, 0x02, // FEATURE (Data,Var,Abs)
 0xc0, // END_COLLECTION
 0xa1, 0x02, // COLLECTION (Logical)
 0x85, 0x03, // REPORT_ID (3)
 0x05, 0x09, // USAGE_PAGE (Button)
 0x09, 0x01, // USAGE (Button 1)
 0x25, 0x01, // LOGICAL_MAXIMUM (1)
 0x75, 0x01, // REPORT_SIZE (1)
 0xb1, 0x02, // FEATURE (Data,Var,Abs)
 0x75, 0x07, // REPORT_SIZE (7)
 0xb1, 0x03, // FEATURE (Cnst,Var,Abs)
 0xc0, // END_COLLECTION
 0xc0 // END_COLLECTION
};

/*
typedef struct _HID_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT bcdHID;
    UCHAR bCountry;
    UCHAR bNumDescriptors;
    struct _HID_DESCRIPTOR_DESC_LIST {
        UCHAR bReportType;
        USHORT wReportLength;
    } DescriptorList[1];
} HID_DESCRIPTOR, * PHID_DESCRIPTOR;
*/

//////////////////////////////////////
////       CompleteRequest       ///// 
//////////////////////////////////////

static
NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status, ULONG_PTR info)
{
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}


//////////////////////////////////////
////           AddDevice         ///// 
//////////////////////////////////////

static
NTSTATUS AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT fdo)
{
    PDEVICE_EXTENSION pdx = PDX(fdo);
    NTSTATUS status = STATUS_SUCCESS;
    status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &fdo);
    pdx->AddDeviceStatus = status;
    return status;
}




//////////////////////////////////////
////        DriverUnload         ///// 
//////////////////////////////////////

static
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
}


/*
NTSTATUS HandleQueryStop(PDEVICE_OBJECT fdo, PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
    if (pdx->state != WORKING)
        return DefaultPnpHandler(fdo, Irp);
    if (!OkayToStop(pdx))
        return CompleteRequest(Irp, STATUS_UNSUCCESSFUL, 0);
    StallRequests(&pdx->dqReadWrite);
    WaitForCurrentIrp(&pdx->dqReadWrite);
    pdx->state = PENDINGSTOP;
    return DefaultPnpHandler(fdo, Irp);
}


static
NTSTATUS HandleStartDevice(PDEVICE_OBJECT fdo, PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    NTSTATUS status = ForwardAndWait(fdo, Irp);
    if (!NT_SUCCESS(status))
        return CompleteRequest(Irp, status, Irp->IoStatus.Information);
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    status = StartDevice(fdo, <additional args>);
    return CompleteRequest(Irp, status, Irp->IoStatus.Information);
}

static
NTSTATUS?HandleStopDevice(PDEVICE_OBJECT fdo, PIRP Irp)
{
    <complicated stuff>
    StopDevice(fdo, oktouch);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return?DefaultPnpHandler(fdo, Irp);
}

static
NTSTATUS HandleStopDevice(PDEVICE_OBJECT fdo, PIRP Irp)
{
    <complicated stuff>
        StopDevice(fdo, oktouch);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return DefaultPnpHandler(fdo, Irp);
}

VOID RemoveDevice(PDEVICE_OBJECT fdo)
{
    PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
    IoDetachDevice(pdx->LowerDeviceObject);
    IoDeleteDevice(fdo);
}


static
NTSTATUS HandleRemoveDevice(PDEVICE_OBJECT fdo, PIRP Irp)
{
    PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
    <complicated stuff>
        DeregisterAllInterfaces(pdx);
    StopDevice(fdo, oktouch);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    NTSTATUS status = DefaultPnpHandler(fdo, Irp);
    RemoveDevice(fdo);
    return status;
}
*/

static
NTSTATUS DispatchPnp(PDEVICE_OBJECT fdo, PIRP Irp)
{
    UNREFERENCED_PARAMETER(fdo);
    UNREFERENCED_PARAMETER(Irp);
    /*
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG fcn = stack->MinorFunction;
    static NTSTATUS(*fcntab[])(PDEVICE_OBJECT, PIRP) = {
        HandleStartDevice(fdo, Irp);
        HandleStopDevice(fdo, Irp);
        HandleSurpriseRemoval(fdo, Irp);
        HandleRemoveDevice(fdo, Irp);
    };
    if (fcn >= arraysize(fcntab))
    {
        return DefaultPnpHandler(fdo, Irp);
    }
    return (*fcntab[fcn])(fdo, Irp);
    */
}

static
NTSTATUS PowerUpCompletionRoutine(PDEVICE_OBJECT fdo, PIRP Irp, PDEVICE_EXTENSION pdx)
{
    UNREFERENCED_PARAMETER(fdo);
    UNREFERENCED_PARAMETER(Irp);
    pdx->devpower = PowerDeviceD0;
    return STATUS_SUCCESS;
}


//////////////////////////////////////
////        DispatchPower        ///// 
//////////////////////////////////////

static
NTSTATUS DispatchPower(PDEVICE_OBJECT fdo, PIRP Irp)
{
    PDEVICE_EXTENSION pdx = PDX(fdo);
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    if (stack->MinorFunction == IRP_MN_SET_POWER && stack->Parameters.Power.Type == DevicePowerState)
    {
        DEVICE_POWER_STATE newstate = stack->Parameters.Power.State.DeviceState;
        if (newstate == PowerDeviceD0)
        {
            IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) PowerUpCompletionRoutine, (PVOID)pdx, TRUE, TRUE, TRUE);
        }
        else if (pdx->devpower == PowerDeviceD0)
        {
            // TODO save context information, if any
            pdx->devpower = newstate;
        }
    }
    return PoCallDriver(LDO(fdo), Irp);
}






//////////////////////////////////////
////   DispatchInternalControl   ///// 
//////////////////////////////////////

#pragma LOCKEDCODE
static
NTSTATUS DispatchInternalControl(PDEVICE_OBJECT fdo, PIRP Irp)
{
    PDEVICE_EXTENSION pdx = PDX(fdo);
    NTSTATUS status = STATUS_SUCCESS;
    size_t info = 0;
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    PVOID buffer = Irp->UserBuffer;
    ULONG BytesToRead;

    switch (code)
    {
        case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        {
            if (cbout < sizeof(HID_DEVICE_ATTRIBUTES))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            #define p ((PHID_DEVICE_ATTRIBUTES) buffer)
            RtlZeroMemory(p, sizeof(HID_DEVICE_ATTRIBUTES));
            p->Size = sizeof(HID_DEVICE_ATTRIBUTES);
            p->VendorID = 0;
            p->ProductID = 0;
            p->VersionNumber = 1;
            #undef p
            info = sizeof(HID_DEVICE_ATTRIBUTES);
            break;
        }

        case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        {
            #define p ((PHID_DESCRIPTOR) buffer)
            if (cbout < sizeof(HID_DESCRIPTOR))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            RtlZeroMemory(p, sizeof(HID_DESCRIPTOR));
            p->bLength = sizeof(HID_DESCRIPTOR);
            p->bDescriptorType = HID_HID_DESCRIPTOR_TYPE;
            p->bcdHID = HID_REVISION;
            p->bCountry = 0;
            p->bNumDescriptors = 1;
            p->DescriptorList[0].bReportType = HID_REPORT_DESCRIPTOR_TYPE;
            p->DescriptorList[0].wReportLength = sizeof(ReportDescriptor);
            #undef p
            info = sizeof(HID_DESCRIPTOR);
            break;
        }

        case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        {
            if (cbout < sizeof(ReportDescriptor))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            RtlCopyMemory(buffer, ReportDescriptor, sizeof(ReportDescriptor));
            info = sizeof(ReportDescriptor);
            break;
        }

        case IOCTL_HID_READ_REPORT:
        {
            if (stack->MajorFunction == IRP_MJ_READ)
            {
                BytesToRead = stack->Parameters.Read.Length;
                if (cbout <= BytesToRead)
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
                RtlCopyMemory(buffer, Irp->AssociatedIrp.SystemBuffer, BytesToRead);
                info = sizeof(IOCTL_HID_READ_REPORT);
                break;
            }
        }

        /*
        case IOCTL_HID_GET_FEATURE:
        {
            #define p ((PHID_XFER_PACKET) buffer)
            #define FEATURE_CODE_XX 0
            switch (p->reportId)
            {
                case FEATURE_CODE_XX:
                {
                    if (p->reportBufferLen < sizeof(p->reportBuffer))
                    {
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }
                    RtlCopyMemory(p->reportBuffer, FEATURE_REPORT_Xx, sizeof(p->reportBuffer));
                    info = sizeof(p->reportBuffer);
                    break;
                    }
                    default:
                        status = STATUS_INVALID_PARAMETER;
                        break;
            }
            #undef p
            break;
        }

        case IOCTL_HID_SET_FEATURE:
        {
            typedef struct {
                uint8_t reportId; // Identifier byte
                uint8_t data[8];  // 8 bytes of report data
            } FEATURE_REPORT_yy;

            #define p ((PHID_XFER_PACKET) buffer)
            #define FEATURE_CODE_YY 0
            switch (p->reportId)
            {
                case FEATURE_CODE_YY:
                {
                    if (p->reportBufferLen > sizeof(p->reportBuffer))
                    {
                        status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                    RtlCopyMemory(FeatureReportYy, p->reportBuffer, p->reportBufferLen);
                    break;
                }
                default:
                    status = STATUS_INVALID_PARAMETER;
                    break;
            }
            #undef p
            break;
        }
        */

        case IOCTL_HID_GET_STRING:
        {
            USHORT* p = (USHORT*)&stack->Parameters.DeviceIoControl.Type3InputBuffer;
            USHORT istring = p[0];
            //LANGID langid = p[1];
            #undef p
            PWCHAR string = NULL;
            HID_DEVICE_ATTRIBUTES* attrs = (HID_DEVICE_ATTRIBUTES*)buffer;
            WCHAR tempString[32];
            switch (istring)
            {
                case HID_STRING_ID_IMANUFACTURER:
                    swprintf_s(tempString, sizeof(tempString) / sizeof(WCHAR), L"%04X", attrs->VendorID);
                    string = tempString;
                    break;
                case HID_STRING_ID_IPRODUCT:
                    swprintf_s(tempString, sizeof(tempString) / sizeof(WCHAR), L"%04X", attrs->ProductID);
                    string = tempString;
                    break;
                case HID_STRING_ID_ISERIALNUMBER:
                    swprintf_s(tempString, sizeof(tempString) / sizeof(WCHAR), L"%04X", attrs->VersionNumber);
                    string = tempString;
                    break;
                default:
                    status = STATUS_INVALID_PARAMETER;
                    break;
            }

            if (string == NULL)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            size_t lstring = wcslen(string);
            if (cbout < lstring * sizeof(WCHAR))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            RtlCopyMemory(buffer, string, lstring * sizeof(WCHAR));
            info = lstring * sizeof(WCHAR);

            if (cbout >= info + sizeof(WCHAR))
            {
                ((PWCHAR)buffer)[lstring] = UNICODE_NULL;
                info += sizeof(WCHAR);
            }
            break;
        }

        case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
        {
            IoCopyCurrentIrpStackLocationToNext(Irp);
            stack = IoGetNextIrpStackLocation(Irp);
            stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;
            return IoCallDriver(pdx->LowerDeviceObject, Irp);
        }

        default:
        {
            status = STATUS_NOT_SUPPORTED;
            CompleteRequest(Irp, status, info);
            return status;
        }
    }
}




//////////////////////////////////////
////         DriverEntry         ///// 
//////////////////////////////////////

static
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    DriverObject->DriverExtension->AddDevice = AddDevice;
    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = DispatchInternalControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;

    HID_MINIDRIVER_REGISTRATION reg;
    RtlZeroMemory(&reg, sizeof(reg));
    reg.Revision = HID_REVISION;
    reg.DriverObject = DriverObject;
    reg.RegistryPath = RegistryPath;
    reg.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
    reg.DevicesArePolled = FALSE;

    return HidRegisterMinidriver(&reg);
}