#pragma once
#include <ntddk.h>

#pragma disable(warning:4214) 

#define VMX_PAGE_SIZE		(0x1000)
#define VMX_PAGE_SIZE_MORE	(0x1000 * 4)
#define VMX_HOST_STACK_SIZE (0x3F00)

/*
VMX_FLAG_0			未启动
VMX_FLAG_1			VMX_ON		阶段完成
VMX_FLAG_2			VMX_Launch  阶段完成  首次 Vm-Entry 完成
*/
#define VMX_STEP_0		(0)
#define VMX_STEP_1		(1)
#define VMX_STEP_2		(2)


#define VMX_HOST_SELECTOR_MASK		(0xFFF8)


#pragma pack(push)  //保存对齐状态
#pragma pack(1)		//设定为4字节对齐
typedef struct _VCPU_STC
{
	// 记录当前 CPU 的编号
	ULONG uCPUID;

	// 记录 虚拟 区域指针
	PVOID pVirtualAddr_VMXON_region;
	PVOID pVirtualAddr_VMXCS_region;
	PVOID pVirtualAddr_MSR_BitMap;

	PVOID pVirtualAddr_Host_Stack;

	// 记录 物理地址
	PHYSICAL_ADDRESS PhysicalAddr_VMXON_region;
	PHYSICAL_ADDRESS PhysicalAddr_VMXCS_region;
	PHYSICAL_ADDRESS PhysicalAddr_MSR_BitMap;



	// 记录 VMM 状态
	ULONG uStatus;
}VCPU_STC, *PVCPU_STC;
#pragma pack(pop)



PVCPU_STC g_pAllCpuStc;
NTSTATUS  VMXSTART();
NTSTATUS  VMXSTOP();