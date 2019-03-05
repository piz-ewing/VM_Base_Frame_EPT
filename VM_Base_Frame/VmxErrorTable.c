#ifdef DBG
#include "stdafx.h"
#include "VmxErrorTable.h"





static wchar_t* szExit_Reason_Des[MAX_EXIT_REASONS_COUNT] = 
{
	L"Vm-Exit 由异常或NMI引发",
	L"Vm-Exit 由外部中断引发",
	L"Vm-Exit 由 triple-fault 引发",
	L"由 INT 信号导致 Vm-Exit",
	L"由 SIPI 导致 Vm-Exit",
	L"I/O SMI: 指接到 SMI 而发生 SMM Vm-Exit,SMI 发生在 I/O 指令后",
	L"Other SMI: 指接到 SMI 而发生 SMM Vm-Exit,SMI 不发生在 I/O 指令后",
	L"Vm-Exit 由 'interupt-window exiting' 为1时引发",
	L"Vm-Exit 由 'NMI-window exiting' 为1时引发",
	L"尝试进行任务切换而导致 Vm-Exit",
	L"尝试执行 CPUID 指令导致 Vm-Exit",
	L"尝试执行 GETSEC 指令导致 Vm-Exit",
	L"尝试执行 HLT 指令导致 Vm-Exit",
	L"尝试执行 INVD 指令导致 Vm-Exit",
	L"尝试执行 INVLPG 指令导致 Vm-Exit",
	L"尝试执行 RDPMC 指令导致 Vm-Exit",
	L"尝试执行 RDTSC 指令导致 Vm-Exit",
	L"尝试执行 RSM 指令导致 Vm-Exit",
	L"尝试执行 VMCALL 指令导致 Vm-Exit",
	L"尝试执行 VMCLEAR 指令导致 Vm-Exit",
	L"尝试执行 VMLAUNCH 指令导致 Vm-Exit",
	L"尝试执行 VMPTRLD 指令导致 Vm-Exit",
	L"尝试执行 VMPTRST 指令导致 Vm-Exit",
	L"尝试执行 VMREAD 指令导致 Vm-Exit",
	L"尝试执行 VMRESUME 指令导致 Vm-Exit",
	L"尝试执行 VMWRITE 指令导致 Vm-Exit",
	L"尝试执行 VMXOFF 指令导致 Vm-Exit",
	L"尝试执行 VMXON 指令导致 Vm-Exit",
	L"尝试访问控制寄存器产生 Vm-Exit",
	L"尝试执行 MOV-DR 指令引发 Vm-Exit",
	L"尝试执行 I/O 指令引发 Vm-Exit",
	L"尝试执行 RDMSR 指令导致 Vm-Exit",
	L"尝试执行 WRMSR 指令导致 Vm-Exit",
	L"在 Vm-Entry 时,由于无效的 guest-state 导致 Vm-Exit",
	L"在 Vm-Entry 时,由于 MSR 加载失败导致 Vm-Exit",
	L"[---保留的 Vm-Exit REASON 35---]",
	L"尝试执行 MWAIT 指令引发 Vm-Exit",
	L"在 Vm-Entry 后,由 MTF(monitor trap flag) 导致 Vm-Exit",
	L"[---保留的 Vm-Exit REASON 38---]",
	L"尝试执行 MONITOR 指令引发 Vm-Exit",
	L"尝试执行 PAUSE 指令引发 Vm-Exit",
	L"在 Vm-Entry 时,由于 machine-check 导致 Vm-Exit",
	L"[---保留的 Vm-Exit REASON 42---]",
	L"由于 VTPR 低于 TPR threshold 值而导致 Vm-Exit",
	L"访问 APIC-access page 页面导致 Vm-Exit",
	L"执行 EOI虚拟化 导致 Vm -Exit",
	L"尝试访问 GDTR 或 IDTR 寄存器导致 Vm-Exit",
	L"尝试访问 LDTR 或 TR 寄存器导致 Vm-Exit",
	L"由于 EPT violation 而导致 Vm-Exit",
	L"由于 EPT misconfiguration 而导致 Vm-Exit",
	L"尝试执行 INVEPT 指令引发 Vm-Exit",
	L"尝试执行 RDTSCP 指令引发 Vm-Exit",
	L"由于 VMX-preemption timer 超时而导致 Vm-Exit",
	L"尝试执行 INVVPID 指令引发 Vm-Exit",
	L"尝试执行 WBINVD 指令引发 Vm-Exit",
	L"尝试执行 XSETBV 指令引发 Vm-Exit",
	L"由于尝试向 APIC-access page写入引发 Vm-Exit",
	L"尝试执行 RDRAND 指令引发 Vm-Exit",
	L"尝试执行 INVPCID 指令引发 Vm-Exit",
	L"尝试执行 VMFUNC 指令引发 Vm-Exit",
};


static wchar_t* szVmFailValid_Des[MAX_FAILVALID_COUNT] =
{
	L"VmFailValid 不存在",
	L"VMCALL 指令执行在 root 环境",
	L"VMCLEAR 指令操作数是无效的物理地址",
	L"VMCLEAR 指令操作数是 VMXON 指针",
	L"VMLAUNCH 执行时, current-VMCS 状态是 非 'clear'",
	L"VMRESULT 执行时, current-VMCS 状态是 非 'launched'",
	L"VMRESULT 在 VMXOFF 指令后",
	L"Vm-entry 操作时, current-VMCS 含有无效的 VM-execution 控制字段",
	L"Vm-entry 操作时, current-VMCS 含有无效的 host-state 字段",
	L"VMPTRLD 指令操作数是无效的物理地址",
	L"VMPTRLD 指令操作数是 VMXON 指针",
	L"VMPTRLD 指令执行时, VMCS 内的 VMCS ID值不符",
	L"VMREAD/VMWRITE 指令读写 current-VMCS 内不存在的字段",
	L"VMWRITE 指令 试图写 current-VMCS 内只读字段",
	L"VmFailValid 保留 14",
	L"VMXON 指令执行在 VMX root operation 环境",
	L"Vm-entry 操作时, current-VMCS 含有无效的 executive-VMCS 指针",
	L"Vm-entry 操作时, current-VMCS 使用了非 'launched' 状态的 executive-VMCS",
	L"Vm-entry 操作时, current-VMCS 内的 executive-VMCS 指针不是 VMXON 指针",
	L"执行 VMCALL 指令切入 SMM-transfer monitor 时,VMCS 非 'clear'",
	L"执行 VMCALL 指令切入 SMM-transfer monitor 时,VMCS 内的 Vm-Exit control(控制) 字段无效",
	L"VmFailValid 保留 21",
	L"执行 VMCALL 指令切入 SMM-transfer monitor 时,MSEG 内的 MESG ID值无效",
	L"在 SMM 双重监控处理机制下执行 VMXOFF 指令",
	L"执行 VMCALL 指令切入 SMM-transfer monitor 时,MSEG 内使用了无效的 SMM monitor 特征",
	L"进行 'Vm-Entry that return from SMM' 操作时, 在 executive VMCS 中遇到了无效的 VM-execution control 字段",
	L"VM-entry操作时,VMLAUNCH 或 VMRESUME 被 MOV-SS/POP-SS 阻塞",
	L"VmFailValid 保留 27",
	L"INVEPT/INVVPID 指令使用了无效的操作数",
};



/*
这两个函数有个区别就是一个下标从0开始一个下标从 1 开始
*/
VOID _KdPrintExitReason(ULONG uExitReason)
{
	if (uExitReason < MAX_EXIT_REASONS_COUNT)
	{
		KdPrint(("-[VM-Base-Frame]- : 0x%08X -- %S\n", uExitReason, szExit_Reason_Des[uExitReason]));
	}
	else
	{
		KdPrint(("-[VM-Base-Frame]- : _KdPrintExitReason Invalid  :  0x%08X\n", uExitReason));
	}
}


VOID _KdPrintFailValid(ULONG uFailValid)
{
	if (uFailValid != 0 && uFailValid < MAX_FAILVALID_COUNT)
	{
		KdPrint(("-[VM-Base-Frame]- :  0x%08X --%S\n", uFailValid, szVmFailValid_Des[uFailValid]));
	}
	else
	{
		KdPrint(("-[VM-Base-Frame]- : _KdPrintFailValid Invalid  :   0x%08X\n", uFailValid));
	}
}


#endif