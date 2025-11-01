#include <ntddk.h>
#include <wdm.h>
#include <hidport.h>

#define PDX(fdo) ((PDEVICE_EXTENSION) (((PHID_DEVICE_EXTENSION)(fdo)->DeviceExtension)->MiniDeviceExtension))
#define PDO(fdo) (((PHID_DEVICE_EXTENSION) ((fdo)->DeviceExtension))->PhysicalDeviceObject)
#define LDO(fdo) (((PHID_DEVICE_EXTENSION) ((fdo)->DeviceExtension))->NextDeviceObject) 

typedef struct _DEVICE_EXTENSION {
    NTSTATUS AddDeviceStatus;
    DEVICE_POWER_STATE devpower;
    void* pgx;
} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

typedef struct _HID_DEVICE_ATTRIBUTES {
    ULONG Size;
    USHORT VendorID;
    USHORT ProductID;
    USHORT VersionNumber;
    USHORT Reserved[11];
} HID_DEVICE_ATTRIBUTES, * PHID_DEVICE_ATTRIBUTES;

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

static
NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status, ULONG_PTR info)
{
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

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

    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    if (status != STATUS_PENDING)
        CompleteRequest(Irp, status, info);
    return status;
}

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
