#include "stdafx.h"
#include "VMX.h"


// 虚拟化计数
CCHAR g_processors = 0;
// 回掉函数注册
extern ULONG uFuncExitHandle;



//************************************
// Method:    AllocRegionMem
// FullName:  AllocRegionMem
// Access:    public 
// Returns:   NTSTATUS
// Qualifier:
// Parameter: PVCPU_STC pCurVCPU
// Remark:    分配空间
//************************************
NTSTATUS AllocRegionMem(PVCPU_STC pCurVCPU)
{
	// 分配空间 
	// 分配 VMXON 空间
	pCurVCPU->pVirtualAddr_VMXON_region = MmAllocateNonCachedMemory(VMX_PAGE_SIZE);
	if (!(pCurVCPU->pVirtualAddr_VMXON_region))
	{
		KdPrint(("-[VM-Base-Frame]- : pVirtualAddr_VMXON_region 分配失败!\n"));
		return STATUS_UNSUCCESSFUL;
	}
	// 分配 VMXCS 空间
	pCurVCPU->pVirtualAddr_VMXCS_region = MmAllocateNonCachedMemory(VMX_PAGE_SIZE);
	if (!pCurVCPU->pVirtualAddr_VMXCS_region)
	{
	KdPrint(("-[VM-Base-Frame]- : pVirtualAddr_VMXCS_region 分配失败!\n"));
	return STATUS_UNSUCCESSFUL;
	}

	// 分配 Host_Stack
	pCurVCPU->pVirtualAddr_Host_Stack = MmAllocateNonCachedMemory(VMX_PAGE_SIZE_MORE);
	if (!pCurVCPU->pVirtualAddr_Host_Stack)
	{
		KdPrint(("-[VM-Base-Frame]- : pVirtualAddr_Host_Stack 分配失败!\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// 分配 MSR 位图
	pCurVCPU->pVirtualAddr_MSR_BitMap = MmAllocateNonCachedMemory(VMX_PAGE_SIZE_MORE);
	if (!pCurVCPU->pVirtualAddr_MSR_BitMap)
	{
		KdPrint(("-[VM-Base-Frame]- : pVirtualAddr_Host_Stack 分配失败!\n"));
		return STATUS_UNSUCCESSFUL;
	}
	//////////////////////////////////////////////////////////////////////////




	RtlZeroMemory(pCurVCPU->pVirtualAddr_VMXON_region, VMX_PAGE_SIZE);
	RtlZeroMemory(pCurVCPU->pVirtualAddr_VMXCS_region, VMX_PAGE_SIZE);
	RtlZeroMemory(pCurVCPU->pVirtualAddr_Host_Stack,   VMX_PAGE_SIZE_MORE);
	RtlZeroMemory(pCurVCPU->pVirtualAddr_MSR_BitMap,   VMX_PAGE_SIZE_MORE);
	// 获取物理地址
	pCurVCPU->PhysicalAddr_VMXON_region = MmGetPhysicalAddress(pCurVCPU->pVirtualAddr_VMXON_region);
	pCurVCPU->PhysicalAddr_VMXCS_region = MmGetPhysicalAddress(pCurVCPU->pVirtualAddr_VMXCS_region);
	pCurVCPU->PhysicalAddr_MSR_BitMap = MmGetPhysicalAddress(pCurVCPU->pVirtualAddr_MSR_BitMap);
	return STATUS_SUCCESS;
}



//************************************
// Method:    InitVmcs
// FullName:  InitVmcs
// Access:    public 
// Returns:   VOID
// Qualifier:
// Parameter: PVCPU_STC pCurVCPU
// Parameter: ULONG uEsp
// Remark:    初始化 VMCS 并执行 VMLaunch
//************************************
VOID InitVmcs(PVCPU_STC pCurVCPU, ULONG uFlag, ULONG uEsp)
{
	// 清理并指定 VMCS
	_vmclear(pCurVCPU->PhysicalAddr_VMXCS_region.LowPart,\
		pCurVCPU->PhysicalAddr_VMXCS_region.HighPart);

	if (GetBit(_vmptrld(pCurVCPU->PhysicalAddr_VMXCS_region.LowPart, \
		pCurVCPU->PhysicalAddr_VMXCS_region.HighPart), 0) != 0)
	{
		// 执行失败,,这是测试阶段留下的 
		// 所以 vmclear 没检查
		KdBreakPoint();
	}
	KdPrint(("-[VM-Base-Frame]- : 开始初始化 VMCS!\n"));
	// 开始填充 VMCS 结构
	//  按照 Intel 手册填充
	/*
	VMWRITE—Write Field to Virtual-Machine Control Structure

	Opcode Instruction Description
	0F 79 VMWRITE r64, r/m64 Writes a specified VMCS field (in 64-bit mode)
	0F 79 VMWRITE r32, r/m32 Writes a specified VMCS field (outside 64-bit mode)

	Writes the contents of a primary source operand (register or memory) to a specified field in a VMCS.
	*/
	//////////////////////////////////////////////////////////////////////////
	//  24.4 GUEST - STATE AREA

	/*
	The following fields in the guest-state area correspond to processor registers:
	• Control registers CR0, CR3, and CR4 (64 bits each; 32 bits on processors that do not support Intel 64 archi-
	tecture).
	• Debug register DR7 (64 bits; 32 bits on processors that do not support Intel 64 architecture).
	*/


	_vmwrite(GUEST_CR0, FixCr0());
	_vmwrite(GUEST_CR4, FixCr4());

	_vmwrite(GUEST_CR3, _getcr3());
	_vmwrite(GUEST_DR7, _getdr7());

	KdPrint(("-[VM-Base-Frame]- : Fix_GUEST_CR0 = 0x%08X\n", FixCr0()));
	KdPrint(("-[VM-Base-Frame]- : Fix_GUEST_CR4 = 0x%08X\n", FixCr4()));
	KdPrint(("-[VM-Base-Frame]- : GUEST_CR3		= 0x%08X\n", _getcr3()));
	KdPrint(("-[VM-Base-Frame]- : GUEST_DR7		= 0x%08X\n", _getdr7()));

	///*
	//• RSP, RIP, and RFLAGS (64 bits each; 32 bits on processors that do not support Intel 64 architecture).*/
	_vmwrite(GUEST_RSP, uEsp);
	_vmwrite(GUEST_RFLAGS, uFlag);
	_vmwrite(GUEST_RIP,(ULONG)_guest_ret);
	KdPrint(("-[VM-Base-Frame]- : GUEST_RSP    = 0x%08X\n", uEsp));
	KdPrint(("-[VM-Base-Frame]- : GUEST_RFLAGS = 0x%08X\n", uFlag));
	KdPrint(("-[VM-Base-Frame]- : GUEST_RIP    = 0x%08X\n", (ULONG)_guest_ret));
	/*
	• The following fields for each of the registers CS, SS, DS, ES, FS, GS, LDTR, and TR:
	— Selector (16 bits).
	— Base address (64 bits; 32 bits on processors that do not support Intel 64 architecture). The base-address
	fields for CS, SS, DS, and ES have only 32 architecturally-defined bits; nevertheless, the corresponding
	VMCS fields have 64 bits on processors that support Intel 64 architecture.
	— Segment limit (32 bits). The limit field is always a measure in bytes.
	— Access rights (32 bits). The format of this field is given in Table 24-2 and detailed as follows:
	• The low 16 bits correspond to bits 23:8 of the upper 32 bits of a 64-bit segment descriptor. While bits
	19:16 of code-segment and data-segment descriptors correspond to the upper 4 bits of the segment
	limit, the corresponding bits (bits 11:8) are reserved in this VMCS field.
	• Bit 16 indicates an unusable segment. Attempts to use such a segment fault except in 64-bit mode.
	In general, a segment register is unusable if it has been loaded with a null selector.
	2
	• Bits 31:17 are reserved.
	*/
	// 软件调试 2.6.3
	_vmwrite(GUEST_CS_SELECTOR, _getCS());
	_vmwrite(GUEST_SS_SELECTOR, _getSS());
	_vmwrite(GUEST_DS_SELECTOR, _getDS());
	_vmwrite(GUEST_ES_SELECTOR, _getES());
	_vmwrite(GUEST_FS_SELECTOR, _getFS());
	_vmwrite(GUEST_GS_SELECTOR, _getGS());
	_vmwrite(GUEST_TR_SELECTOR, _getTR());
	_vmwrite(GUEST_LDTR_SELECTOR, _getLDTR());
	KdPrint(("-[VM-Base-Frame]- :GUEST_CS_SELECTOR    Is : 0x%08X\n", _getCS()));
	KdPrint(("-[VM-Base-Frame]- :GUEST_SS_SELECTOR    Is : 0x%08X\n", _getSS()));
	KdPrint(("-[VM-Base-Frame]- :GUEST_DS_SELECTOR    Is : 0x%08X\n", _getDS()));
	KdPrint(("-[VM-Base-Frame]- :GUEST_ES_SELECTOR    Is : 0x%08X\n", _getES()));
	KdPrint(("-[VM-Base-Frame]- :GUEST_FS_SELECTOR    Is : 0x%08X\n", _getFS()));
	KdPrint(("-[VM-Base-Frame]- :GUEST_GS_SELECTOR    Is : 0x%08X\n", _getCS()));
	KdPrint(("-[VM-Base-Frame]- :GUEST_TR_SELECTOR    Is : 0x%08X\n", _getTR()));
	KdPrint(("-[VM-Base-Frame]- :GUEST_LDTR_SELECTOR  Is : 0x%08X\n", _getLDTR()));

	GIDTR gdt = { 0 };
	_getGDTR(&gdt);
	_vmwrite(GUEST_GDTR_BASE, gdt.gidt_addr);
	_vmwrite(GUEST_GDTR_LIMIT, gdt.gidt_limit);
	KdPrint(("-[VM-Base-Frame]- : Gdt Base Addr Is  : 0x%08X\n", gdt.gidt_addr));
	KdPrint(("-[VM-Base-Frame]- : Gdt limit  Is		: 0x%08X\n", gdt.gidt_limit));

	GIDTR idt = { 0 };
	_getIDTR(&idt);
	_vmwrite(GUEST_IDTR_BASE, idt.gidt_addr);
	_vmwrite(GUEST_IDTR_LIMIT, idt.gidt_limit);
	KdPrint(("-[VM-Base-Frame]- : Idt Base Addr Is  : 0x%08X\n", idt.gidt_addr));
	KdPrint(("-[VM-Base-Frame]- : Idt limit  Is		: 0x%08X\n", idt.gidt_limit));

	// CS, SS, DS, ES, FS, GS, LDTR, and TR
	ENCODE_SEGMENGT segGuestCS = { GUEST_CS_BASE, GUEST_CS_LIMIT, GUEST_CS_ACCESS_RIGHTS, "GUEST_CS" };
	SetSegmentDescriptor(gdt.gidt_addr, _getCS(), segGuestCS);

	ENCODE_SEGMENGT segGuestSS = { GUEST_SS_BASE, GUEST_SS_LIMIT, GUEST_SS_ACCESS_RIGHTS, "GUEST_SS" };
	SetSegmentDescriptor(gdt.gidt_addr, _getSS(), segGuestSS);

	ENCODE_SEGMENGT segGuestDS = { GUEST_DS_BASE, GUEST_DS_LIMIT, GUEST_DS_ACCESS_RIGHTS, "GUEST_DS" };
	SetSegmentDescriptor(gdt.gidt_addr, _getDS(), segGuestDS);

	ENCODE_SEGMENGT segGuestES = { GUEST_ES_BASE, GUEST_ES_LIMIT, GUEST_ES_ACCESS_RIGHTS, "GUEST_ES" };
	SetSegmentDescriptor(gdt.gidt_addr, _getES(), segGuestES);

	ENCODE_SEGMENGT segGuestFS = { GUEST_FS_BASE, GUEST_FS_LIMIT, GUEST_FS_ACCESS_RIGHTS, "GUEST_FS" };
	SetSegmentDescriptor(gdt.gidt_addr, _getFS(), segGuestFS);

	ENCODE_SEGMENGT segGuestGS = { GUEST_GS_BASE, GUEST_GS_LIMIT, GUEST_GS_ACCESS_RIGHTS, "GUEST_GS" };
	SetSegmentDescriptor(gdt.gidt_addr, _getGS(), segGuestGS);

	ENCODE_SEGMENGT segGuestTR = { GUEST_TR_BASE, GUEST_TR_LIMIT, GUEST_TR_ACCESS_RIGHTS, "GUEST_TR" };
	SetSegmentDescriptor(gdt.gidt_addr, _getTR(), segGuestTR);

	ENCODE_SEGMENGT segGuestLDTR = { GUEST_LDTR_BASE, GUEST_LDTR_LIMIT, GUEST_LDTR_ACCESS_RIGHTS, "GUEST_LDTR" };
	SetSegmentDescriptor(gdt.gidt_addr, _getLDTR(), segGuestLDTR);


	/*
	• The following fields for each of the registers GDTR and IDTR:
	— Base address (64 bits; 32 bits on processors that do not support Intel 64 architecture).
	— Limit (32 bits). The limit fields contain 32 bits even though these fields are specified as only 16 bits in the
	architecture.
	• The following MSRs:
	— IA32_DEBUGCTL (64 bits)
	— IA32_SYSENTER_CS (32 bits)
	— IA32_SYSENTER_ESP and IA32_SYSENTER_EIP (64 bits; 32 bits on processors that do not support Intel 64
	architecture)
	— IA32_PERF_GLOBAL_CTRL (64 bits). This field is supported only on processors that support the 1-setting
	of the “load IA32_PERF_GLOBAL_CTRL” VM-entry control.
	— IA32_PAT (64 bits). This field is supported only on processors that support either the 1-setting of the “load
	IA32_PAT” VM-entry control or that of the “save IA32_PAT” VM-exit control.
	— IA32_EFER (64 bits). This field is supported only on processors that support either the 1-setting of the “load
	IA32_EFER” VM-entry control or that of the “save IA32_EFER” VM-exit control.
	• The register SMBASE (32 bits). This register contains the base address of the logical processor’s SMRAM image.
	*/
	KdPrint(("-[VM-Base-Frame]- : GUEST_IA32_SYSENTER_CS   Is : 0x%08X\n", \
		SetMsrToEncode(SYSENTER_CS_MSR, GUEST_IA32_SYSENTER_CS)));

	KdPrint(("-[VM-Base-Frame]- : GUEST_IA32_SYSENTER_ESP  Is : 0x%08X\n", \
		SetMsrToEncode(SYSENTER_ESP_MSR, GUEST_IA32_SYSENTER_ESP)));

	KdPrint(("-[VM-Base-Frame]- : GUEST_IA32_SYSENTER_EIP  Is : 0x%08X\n", \
		SetMsrToEncode(SYSENTER_EIP_MSR, GUEST_IA32_SYSENTER_EIP)));

	KdPrint(("-[VM-Base-Frame]- : GUEST_DEBUGCTL_FULL	   Is : 0x%08X\n", \
		SetMsrToEncode(MSR_DEBUGCTLB, GUEST_DEBUGCTL_FULL)));

	KdPrint(("-[VM-Base-Frame]- : GUEST_IA32_PERF_GLOBAL_CTRL_FULL  Is : 0x%08X\n", \
		SetMsrToEncode(IA32_PERF_GLOBAL_CTRL, GUEST_IA32_PERF_GLOBAL_CTRL_FULL)));

	KdPrint(("-[VM-Base-Frame]- : GUEST_IA32_PAT_FULL	   Is : 0x%08X\n", \
		SetMsrToEncode(IA32_PAT_MSR, GUEST_IA32_PAT_FULL)));

	KdPrint(("-[VM-Base-Frame]- : GUEST_IA32_EFER_FULL     Is : 0x%08X\n", \
		SetMsrToEncode(IA32_EFER_MSR, GUEST_IA32_EFER_FULL)));
	/*
	#ifdef _X64
	GUEST_IA32_DEBUGCTL_HIGH
	GUEST_IA32_PERF_GLOBAL_CTRL_HIGH
	GUEST_IA32_PAT_HIGH
	GUEST_IA32_EFER_HIGH
	#endif

	Guest SMBASE
	*/
	_vmwrite(GUEST_SMBASE, 0);
	KdPrint(("-[VM-Base-Frame]- : GUEST_SMBASE  Is : 0x%08X\n", 0));

	//////////////////////////////////////////////////////////////////////////
	//  24.4.2 Guest Non-Register State
	// 这几个区域我全部设置为 默认 作为基本框架的一部分 方便扩充
	/*
	In addition to the register state described in Section 24.4.1, the guest-state area includes the following fields that
	characterize guest state but which do not correspond to processor registers:
	• Activity state (32 bits). This field identifies the logical processor’s activity state. When a logical processor is
	executing instructions normally, it is in the active state. Execution of certain instructions and the occurrence
	of certain events may cause a logical processor to transition to an inactive state in which it ceases to execute
	instructions.
	The following activity states are defined:
	—0: Active. The logical processor is executing instructions normally.
	—1: HLT. The logical processor is inactive because it executed the HLT instruction.
	—2: Shutdown. The logical processor is inactive because it incurred a triple fault1 or some other serious
	error.
	—3: Wait-for-SIPI. The logical processor is inactive because it is waiting for a startup-IPI (SIPI).

	Future processors may include support for other activity states. Software should read the VMX capability MSR
	IA32_VMX_MISC (see Appendix A.6) to determine what activity states are supported.
	*/
	_vmwrite(GUEST_ACTIVITY_STATE, 0);
	KdPrint(("-[VM-Base-Frame]- : GUEST_ACTIVITY_STATE			  Is : 0x%08X\n", 0));
	/*
	• Interruptibility state (32 bits). The IA-32 architecture includes features that permit certain events to be
	blocked for a period of time. This field contains information about such blocking. Details and the format of this
	field are given in Table 24-3.
	*/
	_vmwrite(GUEST_INTERRUPTIBILITY_STATE, 0);
	KdPrint(("-[VM-Base-Frame]- : GUEST_INTERRUPTIBILITY_STATE    Is : 0x%08X\n", 0));
	/*
	• Pending debug exceptions (64 bits; 32 bits on processors that do not support Intel 64 architecture). IA-32
	processors may recognize one or more debug exceptions without immediately delivering them.
	2 This field
	contains information about such exceptions. This field is described in Table 24-4.
	*/
	// 
	_vmwrite(GUEST_PENDING_DEBUG_EXCEPTIONS, 0);
	KdPrint(("-[VM-Base-Frame]- : GUEST_PENDING_DEBUG_EXCEPTIONS  Is : 0x%08X\n", 0));
	
	/*
	• VMCS link pointer (64 bits). If the “VMCS shadowing” VM-execution control is 1, the VMREAD and VMWRITE
	instructions access the VMCS referenced by this pointer (see Section 24.10). Otherwise, software should set
	this field to FFFFFFFF_FFFFFFFFH to avoid VM-entry failures (see Section 26.3.1.5).
	*/
	_vmwrite(VMCS_LINK_POINTER_FULL, 0xFFFFFFFF);
	KdPrint(("-[VM-Base-Frame]- : VMCS_LINK_POINTER_FULL  Is : 0x%08X\n", 0xFFFFFFFF));

	// 这个必须设置上不然会爆出 12 错误
	_vmwrite(VMCS_LINK_POINTER_HIGH, 0xFFFFFFFF);
	KdPrint(("-[VM-Base-Frame]- : VMCS_LINK_POINTER_HIGH  Is : 0x%08X\n", 0xFFFFFFFF));
	/*
	#ifdef _X64
	_vmwrite(VMCS_LINK_POINTER_HIGH, 0xFFFFFFFF);
	KdPrint(("-[VM-Base-Frame]- : VMCS_LINK_POINTER_HIGH  Is : 0x%08X\n", 0xFFFFFFFF));
	#endif
	*/
	/*
	• VMX-preemption timer value (32 bits). This field is supported only on processors that support the 1-setting
	of the “activate VMX-preemption timer” VM-execution control. This field contains the value that the VMX-
	preemption timer will use following the next VM entry with that setting. See Section 25.5.1 and Section 26.6.4.
	*/
	// 如果未启用该字段 对该字段设值 会爆出 12 错误
	//_vmwrite(VMX_PREEMPTION_TIMER_VALUE, 0);
	//KdPrint(("-[VM-Base-Frame]- : VMX_PREEMPTION_TIMER_VALUE  Is : 0x%08X\n", 0));

	/*
	• Page-directory-pointer-table entries (PDPTEs; 64 bits each). These four (4) fields (PDPTE0, PDPTE1,
	PDPTE2, and PDPTE3) are supported only on processors that support the 1-setting of the “enable EPT” VM-
	execution control. They correspond to the PDPTEs referenced by CR3 when PAE paging is in use (see Section
	4.4 in the Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 3A). They are used only if
	the “enable EPT” VM-execution control is 1.
	*/

	// 注意 只有 控制区启用 EPT 的时候 这个设置才有效, 这里我不打算启用
	_vmwrite(GUEST_PDPTE0_FULL, 0);
	_vmwrite(GUEST_PDPTE1_FULL, 0);
	_vmwrite(GUEST_PDPTE2_FULL, 0);
	_vmwrite(GUEST_PDPTE3_FULL, 0);
	KdPrint(("-[VM-Base-Frame]- : GUEST_PDPTE0_FULL  Is : 0x%08X\n", 0));
	KdPrint(("-[VM-Base-Frame]- : GUEST_PDPTE1_FULL  Is : 0x%08X\n", 0));
	KdPrint(("-[VM-Base-Frame]- : GUEST_PDPTE2_FULL  Is : 0x%08X\n", 0));
	KdPrint(("-[VM-Base-Frame]- : GUEST_PDPTE3_FULL  Is : 0x%08X\n", 0));
	/*
	#ifdef _X64
	_vmwrite(GUEST_PDPTE0_HIGH, 0);
	_vmwrite(GUEST_PDPTE1_HIGH, 0);
	_vmwrite(GUEST_PDPTE2_HIGH, 0);
	_vmwrite(GUEST_PDPTE3_HIGH, 0);
	#endif
	*/
	
	/*
	• Guest interrupt status (16 bits). This field is supported only on processors that support the 1-setting of the
	“virtual-interrupt delivery” VM-execution control. It characterizes part of the guest’s virtual-APIC state and
	does not correspond to any processor or APIC registers. It comprises two 8-bit subfields:
	— Requesting virtual interrupt (RVI). This is the low byte of the guest interrupt status. The processor
	treats this value as the vector of the highest priority virtual interrupt that is requesting service. (The value
	0 implies that there is no such interrupt.)
	— Servicing virtual interrupt (SVI). This is the high byte of the guest interrupt status. The processor
	treats this value as the vector of the highest priority virtual interrupt that is in service. (The value 0 implies
	that there is no such interrupt.)
	See Chapter 29 for more information on the use of this field.
	*/
	// 如果未启用该字段 对该字段设值 会爆出 12 错误
	//_vmwrite(GUEST_INTERRUPT_STATUS, 0);
	//KdPrint(("-[VM-Base-Frame]- : GUEST_INTERRUPT_STATUS  Is : 0x%08X\n", 0));
	/*
	• PML index (16 bits). This field is supported only on processors that support the 1-setting of the “enable PML”
	VM-execution control. It contains the logical index of the next entry in the page-modification log. Because the
	page-modification log comprises 512 entries, the PML index is typically a value in the range 0–511. Details of
	the page-modification log and use of the PML index are given in Section 28.2.5.
	*/
	// 如果未启用该字段 对该字段设值 会爆出 12 错误
	//_vmwrite(PML_INDEX, 0);
	//KdPrint(("-[VM-Base-Frame]- : PML_INDEX  Is : 0x%08X\n", 0));
	
	//////////////////////////////////////////////////////////////////////////
	// 24.5 HOST-STATE AREA
	/*
	This section describes fields contained in the host-state area of the VMCS. As noted earlier, processor state is
	loaded from these fields on every VM exit (see Section 27.5).
	All fields in the host-state area correspond to processor registers:
	• CR0, CR3, and CR4 (64 bits each; 32 bits on processors that do not support Intel 64 architecture).
	*/
	_vmwrite(HOST_CR0, FixCr0());
	_vmwrite(HOST_CR4, FixCr4());
	_vmwrite(HOST_CR3, _getcr3());
	KdPrint(("-[VM-Base-Frame]- : HOST_CR0  Is : 0x%08X\n", FixCr0()));
	KdPrint(("-[VM-Base-Frame]- : HOST_CR4  Is : 0x%08X\n", FixCr4()));
	KdPrint(("-[VM-Base-Frame]- : HOST_CR3  Is : 0x%08X\n", _getcr3()));

	/*
	• RSP and RIP (64 bits each; 32 bits on processors that do not support Intel 64 architecture).
	*/
	_vmwrite(HOST_RSP, (ULONG)pCurVCPU->pVirtualAddr_Host_Stack + VMX_HOST_STACK_SIZE);
	_vmwrite(HOST_RIP, (ULONG)_exit_handle);
	KdPrint(("-[VM-Base-Frame]- : HOST_RSP  Is : 0x%08X\n", (ULONG)pCurVCPU->pVirtualAddr_Host_Stack + VMX_HOST_STACK_SIZE));
	KdPrint(("-[VM-Base-Frame]- : HOST_RIP  Is : 0x%08X\n", _exit_handle));

	/*
	• Selector fields (16 bits each) for the segment registers CS, SS, DS, ES, FS, GS, and TR. There is no field in the
	host-state area for the LDTR selector.
	• Base-address fields for FS, GS, TR, GDTR, and IDTR (64 bits each; 32 bits on processors that do not support
	Intel 64 architecture).
	*/
	_vmwrite(HOST_CS_SELECTOR, _getCS() & VMX_HOST_SELECTOR_MASK);
	_vmwrite(HOST_SS_SELECTOR, _getSS() & VMX_HOST_SELECTOR_MASK);
	_vmwrite(HOST_DS_SELECTOR, _getDS() & VMX_HOST_SELECTOR_MASK);
	_vmwrite(HOST_ES_SELECTOR, _getES() & VMX_HOST_SELECTOR_MASK);
	_vmwrite(HOST_FS_SELECTOR, _getFS() & VMX_HOST_SELECTOR_MASK);
	_vmwrite(HOST_GS_SELECTOR, _getGS() & VMX_HOST_SELECTOR_MASK);
	_vmwrite(HOST_TR_SELECTOR, _getTR() & VMX_HOST_SELECTOR_MASK);
	KdPrint(("-[VM-Base-Frame]- :HOST_CS_SELECTOR  Is : 0x%08X\n", _getCS() & VMX_HOST_SELECTOR_MASK));
	KdPrint(("-[VM-Base-Frame]- :HOST_SS_SELECTOR  Is : 0x%08X\n", _getSS() & VMX_HOST_SELECTOR_MASK));
	KdPrint(("-[VM-Base-Frame]- :HOST_DS_SELECTOR  Is : 0x%08X\n", _getDS() & VMX_HOST_SELECTOR_MASK));
	KdPrint(("-[VM-Base-Frame]- :HOST_ES_SELECTOR  Is : 0x%08X\n", _getES() & VMX_HOST_SELECTOR_MASK));
	KdPrint(("-[VM-Base-Frame]- :HOST_FS_SELECTOR  Is : 0x%08X\n", _getFS() & VMX_HOST_SELECTOR_MASK));
	KdPrint(("-[VM-Base-Frame]- :HOST_GS_SELECTOR  Is : 0x%08X\n", _getGS() & VMX_HOST_SELECTOR_MASK));
	KdPrint(("-[VM-Base-Frame]- :HOST_TR_SELECTOR  Is : 0x%08X\n", _getTR() & VMX_HOST_SELECTOR_MASK));
	_vmwrite(HOST_GDTR_BASE, gdt.gidt_addr);
	_vmwrite(HOST_IDTR_BASE, idt.gidt_addr);

	ENCODE_SEGMENGT segHostFS = { HOST_FS_BASE, 0, 0, "HOST_FS" };
	SetSegmentDescriptor(gdt.gidt_addr, _getFS(), segHostFS);

	ENCODE_SEGMENGT segHostGS = { HOST_GS_BASE, 0, 0, "HOST_GS" };
	SetSegmentDescriptor(gdt.gidt_addr, _getGS(), segHostGS);

	ENCODE_SEGMENGT segHostTR = { HOST_TR_BASE, 0, 0, "HOST_TR" };
	SetSegmentDescriptor(gdt.gidt_addr, _getTR(), segHostTR);

	/*
	• The following MSRs:
	— IA32_SYSENTER_CS (32 bits)
	— IA32_SYSENTER_ESP and IA32_SYSENTER_EIP (64 bits; 32 bits on processors that do not support Intel 64
	architecture).
	— IA32_PERF_GLOBAL_CTRL (64 bits). This field is supported only on processors that support the 1-setting of
	the “load IA32_PERF_GLOBAL_CTRL” VM-exit control.
	— IA32_PAT (64 bits). This field is supported only on processors that support the 1-setting of the “load
	IA32_PAT” VM-exit control.
	— IA32_EFER (64 bits). This field is supported only on processors that support the 1-setting of the “load
	IA32_EFER” VM-exit control.
	In addition to the state identified here, some processor state components are loaded with fixed values on every
	VM exit; there are no fields corresponding to these components in the host-state area. See Section 27.5 for details
	of how state is loaded on VM exits.
	*/
	KdPrint(("-[VM-Base-Frame]- : HOST_IA32_SYSENTER_CS  Is : 0x%08X\n", \
		SetMsrToEncode(SYSENTER_CS_MSR, HOST_IA32_SYSENTER_CS)));

	KdPrint(("-[VM-Base-Frame]- : HOST_IA32_SYSENTER_ESP  Is : 0x%08X\n", \
		SetMsrToEncode(SYSENTER_ESP_MSR, HOST_IA32_SYSENTER_ESP)));

	KdPrint(("-[VM-Base-Frame]- : HOST_IA32_SYSENTER_EIP  Is : 0x%08X\n", \
		SetMsrToEncode(SYSENTER_EIP_MSR, HOST_IA32_SYSENTER_EIP)));

	KdPrint(("-[VM-Base-Frame]- : HOST_IA32_PERF_GLOBAL_CTRL_FULL  Is : 0x%08X\n", \
		SetMsrToEncode(IA32_PERF_GLOBAL_CTRL, HOST_IA32_PERF_GLOBAL_CTRL_FULL)));

	KdPrint(("-[VM-Base-Frame]- : HOST_IA32_PAT_FULL   Is : 0x%08X\n", \
		SetMsrToEncode(IA32_PAT_MSR, HOST_IA32_PAT_FULL)));

	KdPrint(("-[VM-Base-Frame]- : HOST_IA32_EFER_FULL  Is : 0x%08X\n", \
		SetMsrToEncode(IA32_EFER_MSR, HOST_IA32_EFER_FULL)));
	/*
	#ifdef _X64
	High ...
	#endif
	*/


	//////////////////////////////////////////////////////////////////////////
	// 24.6 VM-EXECUTION CONTROL FIELDS

	// 24.6.1 Pin-Based VM-Execution Controls

	/*
	All other bits in this field are reserved, some to 0 and some to 1. Software should consult the VMX capability MSRs
	IA32_VMX_PINBASED_CTLS and IA32_VMX_TRUE_PINBASED_CTLS (see Appendix A.3.1) to determine how to
	set reserved bits. Failure to set reserved bits properly causes subsequent VM entries to fail (see Section 26.2.1.1).
	The first processors to support the virtual-machine extensions supported only the 1-settings of bits 1, 2, and 4.
	The VMX capability MSR IA32_VMX_PINBASED_CTLS will always report that these bits must be 1. Logical proces-
	sors that support the 0-settings of any of these bits will support the VMX capability MSR
	IA32_VMX_TRUE_PINBASED_CTLS MSR, and software should consult this MSR to discover support for the 0-
	settings of these bits. Software that is not aware of the functionality of any one of these bits should set that bit to 1.
	*/
	ULONG uPinBased = 0;
	// 暂时不处理这些
	// SetBit(uPinBased, EXTERNAL_INTERRUP_EXITING,TRUE);

	// 不可屏蔽中断请求信号NMI用来通知CPU
	// SetBit(uPinBased, NMI_EXITING, TRUE);


	// SetBit(uPinBased, ACTIVATE_VMX_PREEMPTION_TIMER, TRUE);

	MSR_REGISTER msrBasic = { 0 };
	msrBasic = _readmsr(IA32_VMX_BASIC);
	BOOLEAN bIsTrue = GetBit(msrBasic.uHigh, 55 - 32);



	MSR_REGISTER msrPinBasedCtls = { 0 };
	if (bIsTrue)
	{
		msrPinBasedCtls = _readmsr(IA32_VMX_TRUE_PINBASED_CTLS);
	}
	else
	{
		msrPinBasedCtls = _readmsr(IA32_VMX_PINBASED_CTLS);
	}
	uPinBased |= msrPinBasedCtls.uLow;
	uPinBased &= msrPinBasedCtls.uHigh;
	_vmwrite(PIN_BASED_VM_EXECUTION_CONTROLS, uPinBased);
	KdPrint(("-[VM-Base-Frame]- : uPinBased  Is : 0x%08X\n", uPinBased));
	//////////////////////////////////////////////////////////////////////////
	// 24.6.2 Processor-Based VM-Execution Controls
	/*
	The processor - based VM - execution controls constitute two 32 - bit vectors that govern the handling of synchronous
	events, mainly those caused by the execution of specific instructions.
	1 These are the primary processor - based
	VM - execution controls and the secondary processor - based VM - execution controls.
	Table 24 - 6 lists the primary processor - based VM - execution controls.See Chapter 25 for more details of how these
	controls affect processor behavior in VMX non - root operation.

	All other bits in this field are reserved, some to 0 and some to 1. Software should consult the VMX capability MSRs
	IA32_VMX_PROCBASED_CTLS and IA32_VMX_TRUE_PROCBASED_CTLS (see Appendix A.3.2) to determine how
	to set reserved bits. Failure to set reserved bits properly causes subsequent VM entries to fail (see Section
	26.2.1.1).
	The first processors to support the virtual-machine extensions supported only the 1-settings of bits 1, 4–6, 8, 13–
	16, and 26. The VMX capability MSR IA32_VMX_PROCBASED_CTLS will always report that these bits must be 1.
	Logical processors that support the 0-settings of any of these bits will support the VMX capability MSR
	IA32_VMX_TRUE_PROCBASED_CTLS MSR, and software should consult this MSR to discover support for the 0-
	settings of these bits. Software that is not aware of the functionality of any one of these bits should set that bit to 1.
	Bit 31 of the primary processor-based VM-execution controls determines whether the secondary processor-based
	VM-execution controls are used. If that bit is 0, VM entry and VMX non-root operation function as if all the
	secondary processor-based VM-execution controls were 0. Processors that support only the 0-setting of bit 31 of
	the primary processor-based VM-execution controls do not support the secondary processor-based VM-execution
	controls.

	All other bits in this field are reserved to 0. Software should consult the VMX capability MSR
	IA32_VMX_PROCBASED_CTLS2 (see Appendix A.3.3) to determine which bits may be set to 1. Failure to clear
	reserved bits causes subsequent VM entries to fail (see Section 26.2.1.1).
	*/
	ULONG uPrimary = 0;
	// 开启 DRX 访问 exit
	uPrimary = SetBit(uPrimary, DRX_ACCESS, TRUE);

	// uPrimary = SetBit(uPrimary, MSR_BITMAPS, TRUE);
	MSR_REGISTER msrProcbasedCtls = { 0 };
	if (bIsTrue)
	{
		msrProcbasedCtls = _readmsr(IA32_VMX_TRUE_PROCBASED_CTLS);
	}
	else
	{
		msrProcbasedCtls = _readmsr(IA32_VMX_PROCBASED_CTLS);
	}
	uPrimary |= msrProcbasedCtls.uLow;
	uPrimary &= msrProcbasedCtls.uHigh;
	_vmwrite(PRIMARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, uPrimary);
	KdPrint(("-[VM-Base-Frame]- : uPrimary  Is : 0x%08X\n", uPrimary));
	// 当处理器控制区域 bit 31 设为 1 时, 二级处理器控制区域有效 ,, 这里留空
	// IA32_VMX_PROCBASED_CTLS2

	// 当 IA32_VMX_PROCBASED_CTLS2 相应位为 1 时 代表支持



	//////////////////////////////////////////////////////////////////////////
	//  24.6.3 Exception Bitmap
	/*
	The exception bitmap is a 32-bit field that contains one bit for each exception. When an exception occurs, its
	vector is used to select a bit in this field. If the bit is 1, the exception causes a VM exit. If the bit is 0, the exception
	is delivered normally through the IDT, using the descriptor corresponding to the exception’s vector.
	Whether a page fault (exception with vector 14) causes a VM exit is determined by bit 14 in the exception bitmap
	as well as the error code produced by the page fault and two 32-bit fields in the VMCS (the page-fault error-code
	mask and page-fault error-code match). See Section 25.2 for details.
	*/
	ULONG uExceptionBitMap = 0;

	// 拦截 #DB 异常
	// uExceptionBitMap = SetBit(uExceptionBitMap, DEBUG_EXCEPTION, TRUE);
	// 拦截 #BP 异常
	//uExceptionBitMap = SetBit(uExceptionBitMap, BREAKPOINT_EXCEPTION, TRUE);
	// 拦截 #PF 异常
	/*       
	*/
	_vmwrite(EXCEPTION_BITMAP, uExceptionBitMap);
	KdPrint(("-[VM-Base-Frame]- : EXCEPTION_BITMAP  Is : 0x%08X\n", uExceptionBitMap));
	//////////////////////////////////////////////////////////////////////////
	// 处理 Msr BitMap  因为整个位图为 0 所以不会触发 任何 由 Msr 导致的 Vm-Exit
	_vmwrite(ADDRESS_OF_MSR_BITMAPS_FULL, pCurVCPU->PhysicalAddr_MSR_BitMap.LowPart);
	_vmwrite(ADDRESS_OF_MSR_BITMAPS_HIGH, pCurVCPU->PhysicalAddr_MSR_BitMap.HighPart);

	//////////////////////////////////////////////////////////////////////////
	//  后面不填了 都在这章  需要哪个 读哪个就好了
	//  启用 EPT 

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//  24.7 VM-EXIT CONTROL FIELDS

	/*
	24.7.1 VM-Exit Controls
	The VM-exit controls constitute a 32-bit vector that governs the basic operation of VM exits. Table 24-10 lists the
	controls supported. See Chapter 27 for complete details of how these controls affect VM exits.

	All other bits in this field are reserved, some to 0 and some to 1. Software should consult the VMX capability MSRs
	IA32_VMX_EXIT_CTLS and IA32_VMX_TRUE_EXIT_CTLS (see Appendix A.4) to determine how it should set the
	reserved bits. Failure to set reserved bits properly causes subsequent VM entries to fail (see Section 26.2.1.2).
	The first processors to support the virtual-machine extensions supported only the 1-settings of bits 0–8, 10, 11,
	13, 14, 16, and 17. The VMX capability MSR IA32_VMX_EXIT_CTLS always reports that these bits must be 1.
	Logical processors that support the 0-settings of any of these bits will support the VMX capability MSR
	IA32_VMX_TRUE_EXIT_CTLS MSR, and software should consult this MSR to discover support for the 0-settings of
	these bits. Software that is not aware of the functionality of any one of these bits should set that bit to 1.
	1. Since Intel 64 architecture specifies that IA32_EFER.LMA is always set to the logical-AND of CR0.PG and IA32_EFER.LME, and since
	CR0.PG is always 1 in VMX operation, IA32_EFER.LMA is always identical to IA32_EFER.LME in VMX operation.

	*/
	ULONG uVmExit = 0;
	MSR_REGISTER msrExitCtls = { 0 };
	if (bIsTrue)
	{
		msrExitCtls = _readmsr(IA32_VMX_TRUE_EXIT_CTLS);
	}
	else
	{
		msrExitCtls = _readmsr(IA32_VMX_EXIT_CTLS);
	}
	uVmExit |= msrExitCtls.uLow;
	uVmExit &= msrExitCtls.uHigh;
	_vmwrite(VM_EXIT_CONTROLS, uVmExit);
	KdPrint(("-[VM-Base-Frame]- : VM_EXIT_CONTROLS  Is : 0x%08X\n", uVmExit));

	//////////////////////////////////////////////////////////////////////////
	// 24.8 VM-ENTRY CONTROL FIELDS
	/*
	The VM - entry control fields govern the behavior of VM entries.They are discussed in Sections 24.8.1 through	24.8.3.
	*/
	/*
	smm 是个好玩的东西，可以给CPU打补丁，具体怎么用不知道
	*/
	ULONG uVmEntry = 0;
	MSR_REGISTER msrEntryCtls = { 0 };
	if (bIsTrue)
	{
		msrEntryCtls = _readmsr(IA32_VMX_TRUE_ENTRY_CTLS);
	}
	else
	{
		msrEntryCtls = _readmsr(IA32_VMX_ENTRY_CTLS);
	}
	uVmEntry |= msrEntryCtls.uLow;
	uVmEntry &= msrEntryCtls.uHigh;
	_vmwrite(VM_ENTRY_CONTROLS, uVmEntry);
	KdPrint(("-[VM-Base-Frame]- : VM_ENTRY_CONTROLS  Is : 0x%08X\n", uVmEntry));

	// 增加虚拟化计数
	g_processors++;
	// 提高状态等级
	pCurVCPU->uStatus = VMX_STEP_2;
	_vmlaunch();
	// 减少虚拟化计数--执行失败
	g_processors--;
	// 降低状态等级
	pCurVCPU->uStatus = VMX_STEP_1;
	KdPrint(("-[VM-Base-Frame]- : _vmlaunch  Is Failed!\n"));
	KdPrint(("-[VM-Base-Frame]- : ErrorCode  Is 0x%08X!\n", _vmread(VM_INSTRUCTION_ERROR)));


	return;
}




//************************************
// Method:    InitVmxInThisCPU
// FullName:  InitVmxInThisCPU
// Access:    public 
// Returns:   NTSTATUS
// Qualifier:
// Remark:    每个核心都要独立运行
//************************************
NTSTATUS InitVmxInThisCPU(PVCPU_STC pCurVCPU)
{

	pCurVCPU->uCPUID = KeGetCurrentProcessorNumber();
	KdPrint(("-[VM-Base-Frame]- : 当前CPUID为:%d\n", pCurVCPU->uCPUID));

	// 检查处理器支持
	if (!CheckVmxSupported())
	{
		KdPrint(("-[VM-Base-Frame]- : 当前 处理器 不支持!\n", pCurVCPU->uCPUID));
		return STATUS_UNSUCCESSFUL;
	}

	// 在 二者头部设置 VMX 版本
	MSR_REGISTER msrVmonId = _readmsr(IA32_VMX_BASIC);
	*(ULONG*)pCurVCPU->pVirtualAddr_VMXON_region = msrVmonId.uLow;
	*(ULONG*)pCurVCPU->pVirtualAddr_VMXCS_region = msrVmonId.uLow;




	// 修复Cr0
	_setcr0(FixCr0());
	// 设置 CR4.VMXE[bit 13] = 1
	_setcr4(FixCr4());
	// 检查CR0
	ULONG uCr0 = _getcr0();
	// 检查 PG NE PE
	if (GetBit(uCr0, 31) == 0)
	{
		KdPrint(("-[VM-Base-Frame]- : PG Is't Set!\n"));
		return STATUS_UNSUCCESSFUL;
	}
	if (GetBit(uCr0, 0) == 0)
	{
		KdPrint(("-[VM-Base-Frame]- : PG Is't Set!\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// 执行 VMXON
	ULONG uFlag = _vmxon(pCurVCPU->PhysicalAddr_VMXON_region.LowPart, pCurVCPU->PhysicalAddr_VMXON_region.HighPart);

	if (GetBit(uFlag, 0) != 0)
	{
		KdPrint(("-[VM-Base-Frame]- : _vmxon 失败! %d\n", *(ULONG*)((ULONG)pCurVCPU->pVirtualAddr_VMXON_region + 4)));
		return STATUS_UNSUCCESSFUL;
	}
	KdPrint(("-[VM-Base-Frame]- : _vmxon 成功!\n"));

	// 第一阶段完成
	pCurVCPU->uStatus = VMX_STEP_1;


	// 我打算让 程序 执行完 VMLaunch 之后回到这里
	// 开始第二阶段 ==> 这里采取回调方式
	_init_vmcs(pCurVCPU, (ULONG)InitVmcs);

	
	return STATUS_SUCCESS;
}



//************************************
// Method:    VMXSTOP
// FullName:  VMXSTOP
// Access:    public 
// Returns:   NTSTATUS
// Qualifier:
// Remark:    卸载驱动 卸载Vmx
//************************************
NTSTATUS  VMXSTOP()
{
	KMUTEX				AffinityMutex;
	KIRQL				OldIrql;

	if (!g_pAllCpuStc)
	{
		KdPrint(("-[VM-Base-Frame]- :  pAllCpuStc 不需要释放!\n"));
		return STATUS_SUCCESS;
	}
	KeInitializeMutex(&AffinityMutex, 0);
	KeWaitForSingleObject(&AffinityMutex, Executive, KernelMode, FALSE, NULL);
	for (int i = 0; i < KeNumberProcessors; i++)
	{
		// 设置线程亲和性
		KeSetSystemAffinityThread((KAFFINITY)(1 << i));
		OldIrql = KeRaiseIrqlToDpcLevel();
		// 根模式下退出
		if (g_pAllCpuStc[i].uStatus == VMX_STEP_1)
		{
			_vmxoff();
		}
		// 非根模式下退出 开始陷入并关闭当前 
		if (g_pAllCpuStc[i].uStatus == VMX_STEP_2)
		{
			// 陷入 清理
			_vmxstop();
		}
		KeLowerIrql(OldIrql);
		KeRevertToUserAffinityThread();
		__try
		{
			// 释放空间  MmFreeNonCachedMemory   IRQL		<= APC_LEVEL
			if (g_pAllCpuStc[i].pVirtualAddr_VMXON_region)
			{
				MmFreeNonCachedMemory(g_pAllCpuStc[i].pVirtualAddr_VMXON_region, VMX_PAGE_SIZE);
				g_pAllCpuStc[i].pVirtualAddr_VMXON_region = NULL;
				KdPrint(("-[VM-Base-Frame]- : 处理器编号 - 0x%08X - pVirtualAddr_VMXON_region  已经释放!\n", g_pAllCpuStc[i].uCPUID));
			}
			if (g_pAllCpuStc[i].pVirtualAddr_VMXCS_region)
			{
				MmFreeNonCachedMemory(g_pAllCpuStc[i].pVirtualAddr_VMXCS_region, VMX_PAGE_SIZE);
				g_pAllCpuStc[i].pVirtualAddr_VMXCS_region = NULL;
				KdPrint(("-[VM-Base-Frame]- : 处理器编号 - 0x%08X - pVirtualAddr_VMXCS_region  已经释放!\n", g_pAllCpuStc[i].uCPUID));
			}
			if (g_pAllCpuStc[i].pVirtualAddr_Host_Stack)
			{
				MmFreeNonCachedMemory(g_pAllCpuStc[i].pVirtualAddr_Host_Stack, VMX_PAGE_SIZE_MORE);
				g_pAllCpuStc[i].pVirtualAddr_Host_Stack = NULL;
				KdPrint(("-[VM-Base-Frame]- : 处理器编号 - 0x%08X - pVirtualAddr_Host_Stack  已经释放!\n", g_pAllCpuStc[i].uCPUID));
			}
			/*
			MsrBitMap
			*/
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			KdPrint(("-[VM-Base-Frame]- : 释放时时出现异常 - 0x%08X!\n",  GetExceptionCode()));
			KdPrint(("-[VM-Base-Frame]- : 处理器编号      - 0x%08X!\n",  g_pAllCpuStc[i].uCPUID));
		}
	}
	KeReleaseMutex(&AffinityMutex, FALSE);
	ExFreePool(g_pAllCpuStc);
	g_pAllCpuStc = NULL;
	KdPrint(("-[VM-Base-Frame]- :  pAllCpuStc 已经释放!全部释放完成!\n"));
	return STATUS_SUCCESS;
}




//************************************
// Method:    VMXSTART
// FullName:  VMXSTART
// Access:    public 
// Returns:   NTSTATUS
// Qualifier:
// Remark:    启动Vmx
//************************************
NTSTATUS VMXSTART()
{
	KMUTEX				kAffinityMutex;
	KIRQL				OldIrql;
	// 初始化 退出 处理
	uFuncExitHandle = (ULONG)Exit_Handle;
	RegisterExitHandle();

	// 初步检查处理器支持
	if (!CheckVmxSupported())
	{
		return STATUS_UNSUCCESSFUL;
	}

	// 查询 Paging 支持
	//if (PAEMode == GetPagingModeType())
	//{
	//	KdPrint(("-[VM-Base-Frame]- : PAEMode 初始化开始!\n"));

	//	if (s4KMode == GetPagingSizeMode())
	//	{
	//		KdPrint(("-[VM-Base-Frame]- : s4KMode 初始化开始!\n"));

	//		// 初始化 4K 页面


	//	}
	//	else
	//	{
	//		KdPrint(("-[VM-Base-Frame]- : 暂不支持的 EPT Size 模式!\n"));
	//	}
	//}
	//else
	//{
	//	KdPrint(("-[VM-Base-Frame]- : 暂不支持的 EPT Paging 模式!\n"));
	//}


	g_pAllCpuStc = (PVCPU_STC)ExAllocatePool(NonPagedPool, sizeof(VCPU_STC) * KeNumberProcessors);

	if (!g_pAllCpuStc)
	{
		KdPrint(("-[VM-Base-Frame]- : 开辟 VCPU 空间失败!\n"));
		return STATUS_UNSUCCESSFUL;
	}
	RtlZeroMemory(g_pAllCpuStc, sizeof(VCPU_STC) * KeNumberProcessors);


	KeInitializeMutex(&kAffinityMutex, 0);
	// 防止重复进入,实际上没卵用的感觉,因为下面设置了线程的亲和性
	KeWaitForSingleObject(&kAffinityMutex, Executive, KernelMode, FALSE, NULL);

	for (int i = 0; i < KeNumberProcessors; i++)
	{
		// 开辟所需空间   IRQL		<= APC_LEVEL
		if (!NT_SUCCESS(AllocRegionMem(&g_pAllCpuStc[i])))
		{
			break;
		}

		// 设置线程亲和性
		KeSetSystemAffinityThread((KAFFINITY)(1 << i));
		OldIrql = KeRaiseIrqlToDpcLevel();

		// 开始虚拟化核心
		InitVmxInThisCPU(&g_pAllCpuStc[i]);


		KeLowerIrql(OldIrql);
		KeRevertToUserAffinityThread();
	}

	KeReleaseMutex(&kAffinityMutex, FALSE);

	// 判断虚拟化状态
	if (KeNumberProcessors != g_processors)
	{
		KdPrint(("-[VM-Base-Frame]- : 虚拟化核数错误!\n"));
		// 执行卸载操作
		VMXSTOP();
		return STATUS_UNSUCCESSFUL;
	}
	KdPrint(("-[VM-Base-Frame]- : 启动数 %d\n", g_processors));
	return STATUS_SUCCESS;
}