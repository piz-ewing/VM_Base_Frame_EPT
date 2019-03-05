#pragma once



typedef struct _MSR_REGISTER
{
	ULONG uLow;
	ULONG uHigh;
}MSR_REGISTER, *PMSR_REGISTER;

MSR_REGISTER _readmsr(ULONG uEcx);
VOID _writemsr(ULONG uEcx, MSR_REGISTER Msr);





VOID _setcr0(ULONG uCr0);
ULONG _getcr0();

VOID _setcr3(ULONG uCr3);
ULONG _getcr3();

VOID _setcr4(ULONG uCr4);
ULONG _getcr4();

VOID _setdr7(ULONG uDr7);
ULONG _getdr7();

VOID _getGDTR(PVOID pgdt);
VOID _getIDTR(PVOID pidt);
VOID _getcpuid(ULONG uEncodeEax, PVOID uEax, PVOID uEbx, PVOID uEcx, PVOID uEdx);

// CS, SS, DS, ES, FS, GS, LDTR, and TR
USHORT _getCS();
USHORT _getSS();
USHORT _getDS();
USHORT _getES();
USHORT _getFS();
USHORT _getGS();
USHORT _getLDTR();
USHORT _getTR();

// VMX ЦёБо
ULONG	 _vmxon(ULONG uLow, ULONG uHigh);
VOID	 _vmlaunch();
VOID	 _vmclear(ULONG uLow, ULONG uHigh);
ULONG	 _vmptrld(ULONG uLow, ULONG uHigh);
VOID	 _vmwrite(ULONG dwEncode, ULONG dwValue);
ULONG	 _vmread(ULONG dwEncode);
VOID	 _init_vmcs(PVOID pCurVCPU, ULONG pInitVmcs);
VOID	 _guest_ret(VOID);
VOID	 _exit_handle(VOID);
VOID	 _vmxstop(VOID);
VOID     _vmxoff_no_root(PVOID pEsp, PVOID pEip, PVOID pGuestEsp);
VOID	 _vmxoff(VOID);
ULONG uFuncExitHandle;