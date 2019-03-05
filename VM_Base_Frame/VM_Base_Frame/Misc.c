#include "stdafx.h"
#include "Misc.h"

//************************************
// Method:    GetBit
// FullName:  GetBit
// Access:    public 
// Returns:   CHAR
// Qualifier:
// Parameter: _In_ ULONG uSrc
// Parameter: _In_ ULONG uBit
// Remark:    获取bit位
//************************************
CHAR GetBit(_In_ ULONG uSrc, _In_ ULONG uBit)
{
	return (uSrc & (1 << uBit)) >> uBit;
}


//************************************
// Method:    GetLLBit
// FullName:  GetLLBit
// Access:    public 
// Returns:   CHAR
// Qualifier:
// Parameter: _In_ ULARGE_INTEGER uSrc
// Parameter: _In_ ULONG uBit
// Remark:    获取 ULARGE_INTEGER 的 Bit位
//************************************
CHAR GetLLBit(_In_ ULARGE_INTEGER uSrc, _In_ ULONG uBit)
{
	if (uBit > 32)
	{
		return (uSrc.HighPart & (1 << (uBit - 32))) >> (uBit - 32);
	}
	return (uSrc.LowPart & (1 << uBit)) >> uBit;
}

//************************************
// Method:    SetBit
// FullName:  SetBit
// Access:    public 
// Returns:   ULONG
// Qualifier:
// Parameter: ULONG uSrc
// Parameter: ULONG uBit
// Parameter: BOOLEAN bSet1
// Describe:  设置 bit 注意 非指针,通过返回值返回
//************************************
ULONG SetBit(_In_ ULONG uSrc, _In_ ULONG uBit, _In_ BOOLEAN bSet1)
{
	if (bSet1)
	{
		return (uSrc | (1 << uBit));
	}
	return (uSrc & ~(1 << uBit));
}

//************************************
// Method:    GetSegmentDescriptor
// FullName:  GetSegmentDescriptor
// Access:    public 
// Returns:   VOID
// Qualifier:
// Parameter: ULONG gdtBase
// Parameter: USHORT uSeg
// Parameter: PSEGMENT_DESCRIPTOR_ME curDesMe
// Describe:  获取并整理段描述符
//************************************
VOID GetSegmentDescriptor(ULONG gdtBase, USHORT uSeg, PSEGMENT_DESCRIPTOR_ME curDesMe)
{
	PSEGMENT_SELETOR psegSeletor = (PSEGMENT_SELETOR)&uSeg;
	PSEGMENT_DESCRIPTOR curDes = (PSEGMENT_DESCRIPTOR)(gdtBase + psegSeletor->Seletor * sizeof(SEGMENT_DESCRIPTOR));
	USHORT uShort = curDes->BaseAddr12;
	UCHAR uChar = curDes->BaseAddr3;
	curDesMe->BaseAddr |= uShort;
	curDesMe->BaseAddr |= uChar << 16;
	uChar = curDes->BaseAddr4;
	curDesMe->BaseAddr |= uChar << 24;

	// 无效描述符 或 粒度为页
	if (curDes->Other1.G == 1 || curDes->Other.P == 0)
	{
		curDesMe->SegLimit = 0xFFFFFFFF;
	}
	else
	{
		curDesMe->SegLimit |= curDes->SegLimit12;
		curDesMe->SegLimit |= curDes->Other1.SegLimit3 << 16;
	}
	uChar = *(PCHAR)&curDes->Other;
	curDesMe->Attributes |= uChar;

	uChar = (*(PCHAR)&curDes->Other1) & 0xF0;
	curDesMe->Attributes |= uChar << 8;
}


//************************************
// Method:    SetSegmentDescriptor
// FullName:  SetSegmentDescriptor
// Access:    public 
// Returns:   VOID
// Qualifier:
// Parameter: ULONG gdtBase
// Parameter: USHORT uSeg
// Parameter: ENCODE_SEGMENGT segEncode
// Remark:    根据段选择子 设置对应 Encode
//************************************
VOID SetSegmentDescriptor(ULONG gdtBase, USHORT uSeg, ENCODE_SEGMENGT segEncode)
{
	SEGMENT_DESCRIPTOR_ME SegMent_Descriptor = { 0 };
	GetSegmentDescriptor(gdtBase, uSeg, &SegMent_Descriptor);

	if (segEncode.uEncode_Base)
	{
		_vmwrite(segEncode.uEncode_Base, SegMent_Descriptor.BaseAddr);
		KdPrint(("-[VM-Base-Frame]- :%s Addr        Is  : 0x%08X\n", segEncode.szName, SegMent_Descriptor.BaseAddr));
	}
	if (segEncode.uEncode_Limit)
	{
		_vmwrite(segEncode.uEncode_Limit, SegMent_Descriptor.SegLimit);
		KdPrint(("-[VM-Base-Frame]- :%s limit       Is  : 0x%08X\n", segEncode.szName, SegMent_Descriptor.SegLimit));
	}
	if (segEncode.uEncode_Attributes)
	{
		if (uSeg == 0)
		{
			SegMent_Descriptor.Attributes = SetBit(SegMent_Descriptor.Attributes, 16, TRUE);
		}
		_vmwrite(segEncode.uEncode_Attributes, SegMent_Descriptor.Attributes);
		KdPrint(("-[VM-Base-Frame]- :%s Attributes  Is  : 0x%08X\n", segEncode.szName, SegMent_Descriptor.Attributes));
	}
}



//************************************
// Method:    SetMsrToEncode
// FullName:  SetMsrToEncode
// Access:    public 
// Returns:   ULONG
// Qualifier:
// Parameter: ULONG uEncode_Msr
// Parameter: ULONG uVmEncode
// Remark:    设置 Msr 到 Encode
//************************************
ULONG SetMsrToEncode(ULONG uEncode_Msr, ULONG uVmEncode)
{
	MSR_REGISTER msr = { 0 };
	msr = _readmsr(uEncode_Msr);
	_vmwrite(uVmEncode, msr.uLow);
	return msr.uLow;
}



//************************************
// Method:    FixCr4
// FullName:  FixCr4
// Access:    public 
// Returns:   ULONG
// Qualifier:
// Remark:    修复 Cr4
//************************************
ULONG FixCr4()
{
	MSR_REGISTER msr = { 0 };
	ULONG uCr4 = _getcr4();
	uCr4 = SetBit(uCr4, 13, TRUE);
	msr = _readmsr(IA32_VMX_CR4_FIXED0);
	uCr4 |= msr.uLow;
	msr = _readmsr(IA32_VMX_CR4_FIXED1);
	uCr4 &= msr.uLow;
	return uCr4;
}



//************************************
// Method:    FixCr0
// FullName:  FixCr0
// Access:    public 
// Returns:   ULONG
// Qualifier:
// Remark:    修复 Cr0
//************************************
ULONG FixCr0()
{
	MSR_REGISTER msr = { 0 };
	// 修复 CR0 
	ULONG uCr0 = _getcr0();
	msr = _readmsr(IA32_VMX_CR0_FIXED0);
	uCr0 |= msr.uLow;
	msr = _readmsr(IA32_VMX_CR0_FIXED1);
	uCr0 &= msr.uLow;
	return uCr0;
}




//************************************
// Method:    CheckVmxSupported
// FullName:  CheckVmxSupported
// Access:    public 
// Returns:   BOOLEAN
// Qualifier:
// Remark:    检查处理器支持
//************************************
BOOLEAN CheckVmxSupported()
{

	REGISTERS  regs = { 0 };
	ULONG uEcx = 0;
	_getcpuid(1, &regs.uEAX, &regs.uEBX, &regs.uECX, &regs.uEDX);
	uEcx = regs.uECX;
	if (!GetBit(uEcx, ECXVMX))
	{
		KdPrint(("-[VM-Base-Frame]- : CPU Is't Supported!\n"));
		return FALSE;
	}
	MSR_REGISTER msr = { 0 };
	msr = _readmsr(IA32_FEATURE_CONTROL);

	if (GetBit(msr.uLow, 0) == 1)
	{
		if (GetBit(msr.uLow, 2) == 0)
		{
			KdPrint(("-[VM-Base-Frame]- : BOIS Is Set Disenable!\n"));
			return FALSE;
		}
	}
	else
	{
		msr.uLow = SetBit(msr.uLow, 0, TRUE);
		msr.uLow = SetBit(msr.uLow, 1, TRUE);
		msr.uLow = SetBit(msr.uLow, 2, TRUE);
		_writemsr(IA32_FEATURE_CONTROL, msr);
	}
	return TRUE;
}