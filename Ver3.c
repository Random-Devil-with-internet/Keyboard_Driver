#include <ntddk.h>
#include <wdm.h>
#include <hidport.h>
#include <usbioctl.h>

typedef struct _DEVICE_EXTENSION {
    NTSTATUS AddDeviceStatus;
    DEVICE_POWER_STATE devpower;
    void* pgx;
} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

PDEVICE_OBJECT fdo;
PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
#define PDX(fdo) ((PDEVICE_EXTENSION) (((PHID_DEVICE_EXTENSION)(fdo)->DeviceExtension)->MiniDeviceExtension))
#define PDO(fdo) (((PHID_DEVICE_EXTENSION) ((fdo)->DeviceExtension))->PhysicalDeviceObject)
#define LDO(fdo) (((PHID_DEVICE_EXTENSION) ((fdo)->DeviceExtension))->NextDeviceObject)

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
typedef struct _HID_DEVICE_ATTRIBUTES {
    ULONG Size;
    USHORT VendorID;
    USHORT ProductID;
    USHORT VersionNumber;
    USHORT Reserved[11];
} HID_DEVICE_ATTRIBUTES, * PHID_DEVICE_ATTRIBUTES;
*/

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

static
NTSTATUS AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT fdo)
{
    PDEVICE_EXTENSION pdx = PDX(fdo);
    NTSTATUS status = STATUS_SUCCESS;
    pdx->AddDeviceStatus = status;
    return status;
}

static
NTSTATUS GenericDispatchPnp(PDEVICE_OBJECT fdo, PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;
}

static
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
}

static
NTSTATUS DispatchPnp(PDEVICE_OBJECT fdo, PIRP Irp)
{
    return GenericDispatchPnp(PDX(fdo)->pgx, Irp);
}

static
NTSTATUS PowerUpCompletionRoutine(PDEVICE_OBJECT fdo, PIRP Irp, PDEVICE_EXTENSION pdx)
{
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
    if (stack->MinorFunction == IRP_MN_SET_POWER
        && stack->Parameters.Power.Type == DevicePowerState)
    {
        DEVICE_POWER_STATE newstate =
            stack->Parameters.Power.State.DeviceState;
        if (newstate == PowerDeviceD0)
        {
            IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE)
                PowerUpCompletionRoutine, (PVOID)pdx, TRUE, TRUE, TRUE);
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
////   DispatchInternalControl   ///// 
//////////////////////////////////////

#pragma LOCKEDCODE
static
NTSTATUS DispatchInternalControl(PDEVICE_OBJECT fdo, PIRP Irp)
{
    PDEVICE_EXTENSION pdx = PDX(fdo);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG info = 0;
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    PVOID buffer = Irp->UserBuffer;

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

    case IOCTL_HID_GET_REPORT_DESCRIPTOR :
    {
        if (cbout < sizeof(ReportDescriptor))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        RtlCopyMemory(buffer, ReportDescriptor,
            sizeof(ReportDescriptor));
        info = sizeof(ReportDescriptor);
        break;
    }

    case IOCTL_HID_READ_REPORT:
    {
        if (cbout < <size of report>)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        <obtain report data>
            RtlCopyMemory(buffer, <report>, <size of report>);
        info = <size of report>;
        break;
    }

    case IOCTL_HID_GET_FEATURE:
    {
        #define p ((PHID_XFER_PACKET) buffer)
        switch (p->reportId)
        {
        case FEATURE_CODE_XX:
            if (p->reportBufferLen < sizeof(FEATURE_REPORT_XX))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            RtlCopyMemory(p->reportBuffer, FeatureReportXx,
                sizeof(FEATURE_REPORT_XX));
            info = sizeof(FEATURE_REPORT_XX);
            break;
        }
        break;
        #undef p
    }

    case IOCTL_HID_SET_FEATURE:
    {
        #define p ((PHID_XFER_PACKET) buffer)
        switch (p->reportId)
        {
        case FEATURE_CODE_YY:
            if (p->reportBufferLen > sizeof(FEATURE_REPORT_YY))
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }
            RtlCopyMemory(FeatureReportYy, p->reportBuffer,
                p->reportBufferLen);
            break;
        }
        break;
        #undef p
    }

    case IOCTL_GET_PHYSICAL_DESCRIPTOR:
    {
        if (cbout < sizeof(PhysicalDescriptor))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        PUCHAR p =
            (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority);
        if (!p)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        RtlCopyMemory(p,
            PhysicalDescriptor, sizeof(PhysicalDescriptor));
        info = sizeof(PhysicalDescriptor);
        break;
    }

    case IOCTL_HID_GET_STRING:
    {
        #define Vendor_ID 1A2C
        USHORT* p = (USHORT*)
        &stack->Parameters.DeviceIoControl.Type3InputBuffer;
        USHORT istring = p[0];
        LANGID langid = p[1];
        #undef p
        PWCHAR string = NULL;
        switch (istring)
        {
        case HID_STRING_ID_IMANUFACTURER:
            string = Vendor_ID;
            break;
        case HID_STRING_ID_IPRODUCT:
            string = <product name>;
            break;
        case HID_STRING_ID_ISERIALNUMBER:
            string = <serial number>;
            break;
        }
        if (!string)
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        ULONG lstring = wcslen(string);
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
        status = STATUS_NOT_SUPPORTED;
        break;

    if (status != STATUS_PENDING) {
            CompleteRequest(Irp, status, info);
            return status;
    }
}




//////////////////////////////////////
////         DriverEntry         ///// 
//////////////////////////////////////

static
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
;{
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
