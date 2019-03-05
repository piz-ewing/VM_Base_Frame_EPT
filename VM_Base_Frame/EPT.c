#include "EPT.h"
#include "stdafx.h"


ULONG GetMaxPhyaddr()
{
	REGISTERS  regs = { 0 };
	ULONG uEax = 0;
	_getcpuid(0x80000008, &regs.uEAX, &regs.uEBX, &regs.uECX, &regs.uEDX);
	uEax = regs.uEAX;
	// 取 Eax 的 7:0 即为 MaxPhyaddr
	uEax &= 0xF;
	return uEax;
}


// 416 下方检查是否支持4级页表，检查 IA32_VMX_EPT_VPID_CAP 的 bit 6 为1时 为 4级页表 每个 EPT 大小为 4K,,同时检查 bit 16 bit 17 是否为1 作为效验  bit 8 和 bit 14 查询 EPTP 支持的类型
// EPTP 结构的 bit 5:3 必须设置为3 表示需要经过4级页表的 walk

// bit 6 是 dirty 脏页面 和 权限位，为1时开启，开启时  EPT 页表项的 bit 8 bit 9 视为标志位

/*
PML4T(Page Map Level4 Table)及表内的PML4E结构，每个表为4K，内含512个 PML4E 结构，每个8字节

PDPT (Page Directory Pointer Table)及表内的PDPTE 结构，每个表 4K，内含512个 PDPTE 结构，每个8字节

PDT (Page Directory Table)  及表内的PDE结构，每个表4K，内含512 个 PDE 结构，每个 8 字节

PT(Page Table)及表内额 PTE 结构，每个表4K，内含512个 PTE 结构，每个8字节
*/

/*
static_assert(sizeof(EptPointer) == 8, "Size check");


*/

ULONG GetPagingSizeMode()
{
	// 根据 IA32_VMX_EPT_VPID_CAP 来查看 页面大小支持

	MSR_REGISTER msrEPTVPIDCAP = { 0 };
	msrEPTVPIDCAP = _readmsr(IA32_VMX_EPT_VPID_CAP);

	// 当 bit 6 == 1 支持 4级页表
	// bit 16 == 1 支持 2M 页表
	// bit 17 == 1 支持 1G 页表

	// 优先级别 从上至下
	if (GetBit(msrEPTVPIDCAP.uLow,6) == 1)
	{
		return s4KMode;
	}

	if (GetBit(msrEPTVPIDCAP.uLow, 16) == 1)
	{
		return s2MMode;
	}

	if (GetBit(msrEPTVPIDCAP.uLow, 17) == 1)
	{
		return s1GMode;
	}
	return -1;
}




ULONG GetPagingModeType()
{
	// 检查 EPT 能力

	// CPUID.01H:ECX[5] = 1
	// IA32_VMX_PROCBASED_CTLS[63] = 1
	// IA32_VMX_PROCBASED_CTLS2[33] = 1
	
	// ECX[5] 开始就检查了 这里不重复检查

	KdBreakPoint();

	MSR_REGISTER msrPROCBASEDCTLS = { 0 };
	msrPROCBASEDCTLS = _readmsr(IA32_VMX_PROCBASED_CTLS);

	MSR_REGISTER msrPROCBASEDCTLS2 = { 0 };
	msrPROCBASEDCTLS2 = _readmsr(IA32_VMX_PROCBASED_CTLS2);

	if (GetBit(msrPROCBASEDCTLS.uHigh, 63 - 32) != 1 || GetBit(msrPROCBASEDCTLS2.uHigh, 33 - 32) != 1)
	{
		KdPrint(("-[VM-Base-Frame]- : IA32_VMX_PROCBASED_CTLS OR  IA32_VMX_PROCBASED_CTLS2 Is't Support EPT!\n"));
		return NonSupport;
	}


	// 获取寻址模式

	// 这里预留 3 种模式


	ULONG uCr0 = _getcr0();

	// 检查 PG NE PE
	if (GetBit(uCr0, 31) == 0)
	{
		KdPrint(("-[VM-Base-Frame]- : PG Is't Support EPT!\n"));
		return NonSupport;
	}

	// 32 位分页这里不管了 那个应该是实模式


	// CR4.PAE[bit 5].
	ULONG uCr4 = _getcr4();

	if (GetBit(uCr0, 5) == 0)
	{
		KdPrint(("-[VM-Base-Frame]- : Cr4.PAE Is't Support EPT!\n"));
		return NonSupport;
	}


	MSR_REGISTER msrIA32EFER = { 0 };
	msrIA32EFER = _readmsr(IA32_EFER_MSR);

	// 	IA32_EFER.LMA[bit 10] == 0
	if (GetBit(msrIA32EFER.uLow, 10))
	{
		KdPrint(("-[VM-Base-Frame]- : Use IA32e Mode!\n"));
		return IA32eMode;
	}
	else
	{
		KdPrint(("-[VM-Base-Frame]- : Use PAEMode!\n"));
		return PAEMode;
	}
	return NonSupport;
}