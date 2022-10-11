/************************************************************************/
/*                     This Is VM_Base_Frame                            */
/************************************************************************/

//
// 这个文件仅仅作为入口文件
//
#include "stdafx.h"

VOID unload_drv(_In_ struct _DRIVER_OBJECT *DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	if (NT_SUCCESS(!VMXSTOP()))
	{
		KdPrint(("-[VM-Base-Frame]- : 卸载成功,请检测有无异常!\n"));
	}
}

//************************************
// Method:    DriverEntry
// FullName:  DriverEntry
// Access:    public
// Returns:   NTSTATUS
// Qualifier:
// Parameter: _In_ PDRIVER_OBJECT DriverObject
// Parameter: _In_ PUNICODE_STRING RegistryPath
// Remark:
//************************************
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject,	_In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	VMXSTART();
	DriverObject->DriverUnload = unload_drv;

	return STATUS_SUCCESS;
}