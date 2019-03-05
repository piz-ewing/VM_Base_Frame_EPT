#pragma once
#include <ntddk.h>
#include "vt_x86.h"

// 检测 VMX 支持
#define ECXVMX		5


#pragma pack(push)  //保存对齐状态
#pragma pack(1)		//设定为4字节对齐
typedef struct _SEGMENT_SELETOR
{
	USHORT RPL : 2;
	USHORT TI : 1;
	USHORT Seletor : 13;
}SEGMENT_SELETOR, *PSEGMENT_SELETOR;

// 数字代表其位于第几字节
typedef struct _SEGMENT_DESCRIPTOR
{
	USHORT SegLimit12;
	USHORT BaseAddr12;

	UCHAR  BaseAddr3;

	struct
	{
		UCHAR TYPE : 4;
		UCHAR S : 1;
		UCHAR DPL : 2;
		UCHAR P : 1;
	}Other;

	struct
	{
		UCHAR  SegLimit3 : 4;
		UCHAR  AVL : 1;
		UCHAR  Reserved : 1;
		UCHAR  DB : 1;
		UCHAR  G : 1;
	}Other1;
	UCHAR BaseAddr4;
}SEGMENT_DESCRIPTOR, *PSEGMENT_DESCRIPTOR;


typedef struct _GIDTR
{
	USHORT gidt_limit;
	ULONG  gidt_addr;
}GIDTR, *PGIDTR;


typedef struct _ENCODE_SEGMENGT
{
	ULONG uEncode_Base;
	ULONG uEncode_Limit;
	ULONG uEncode_Attributes;
	char szName[20];
}ENCODE_SEGMENGT, *PENCODE_SEGMENGT;

typedef struct _SEGMENT_DESCRIPTOR_ME
{
	ULONG SegLimit;
	ULONG BaseAddr;
	ULONG Attributes;
}SEGMENT_DESCRIPTOR_ME, *PSEGMENT_DESCRIPTOR_ME;


typedef struct _REGISTERS
{
	ULONG   uFlags;
	ULONG	uEBP;
	ULONG	uEDI;
	ULONG	uESI;
	ULONG	uEDX;
	ULONG	uECX;
	ULONG	uEBX;
	ULONG	uEAX;
	ULONG	uESP;
}REGISTERS, *PREGISTERS;
#pragma pack(pop)







ULONG FixCr0();
ULONG FixCr4();


CHAR  GetBit(_In_ ULONG uSrc, _In_ ULONG uBit);
CHAR  GetLLBit(_In_ ULARGE_INTEGER uSrc, _In_ ULONG uBit);
ULONG SetBit(_In_ ULONG uSrc, _In_ ULONG uBit, _In_ BOOLEAN bSet1);

VOID SetSegmentDescriptor(ULONG gdtBase, USHORT uSeg, ENCODE_SEGMENGT segEncode);
VOID GetSegmentDescriptor(ULONG gdtBase, USHORT uSeg, PSEGMENT_DESCRIPTOR_ME curDesMe);


BOOLEAN CheckVmxSupported();
ULONG   SetMsrToEncode(ULONG uEncode_Msr, ULONG uVmEncode);
