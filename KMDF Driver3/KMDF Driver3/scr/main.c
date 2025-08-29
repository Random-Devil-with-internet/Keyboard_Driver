#include <ntddk.h>
#include <wdm.h>
#include <hidport.h>

typedef struct _DEVICE_EXTENSION {
    PVOID DeviceContext;
    ULONG DeviceId;
    PDEVICE_OBJECT LowerDeviceObject;
} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

static
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
}

static
NTSTATUS DispatchPower(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static
NTSTATUS DispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static
NTSTATUS AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_EXTENSION devExt;
    NTSTATUS status;

    status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    devExt = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
    devExt->LowerDeviceObject = IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
    if (devExt->LowerDeviceObject == NULL) {
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

static
NTSTATUS DispatchInternalControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    IoCopyCurrentIrpStackLocationToNext(Irp);
    return IoCallDriver(devExt->LowerDeviceObject, Irp);
}

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