	.686p
	.model flat, stdcall
	option casemap:none
	
	.Data
dwEip	dword	0h
dwEsp	dword	0h
dwTemp  dword	0h
;===========================================================================	
	.Code
	;16位段规模的情况下，其缺省的数据类型是WORD；在32位段规模的情况下，其缺省的数据类型是DWORD。


	; 在C语言中，函数的返回值存放在寄存器EAX和EDX中，EAX存放低32位，EDX存放高32位，如果返回值不足32位，则只用到EAX。
 
_readmsr  PROC dwExc:dword
		mov ecx, dwExc
		rdmsr
		ret
_readmsr ENDP


_writemsr	Proc	Index:dword,LowPart,HighPart
	mov	ecx, Index
	mov	eax, LowPart
	mov	edx, HighPart
	wrmsr
	ret
_writemsr 	Endp


_setcr0 Proc  dwCr0:dword

		mov eax, dwCr0
		mov cr0, eax
		ret
_setcr0 	Endp


_getcr0 Proc
	mov eax, cr0
	ret
_getcr0 Endp

_setcr3 Proc  dwCr3:dword

		mov eax, dwCr3
		mov cr3, eax
		ret
_setcr3 	Endp


_getcr3 Proc
	mov eax, cr3
	ret
_getcr3 Endp

_setcr4 Proc  dwCr4:dword

		mov eax, dwCr4
		mov cr4, eax
		ret
_setcr4 	Endp


_getcr4 Proc
	mov eax, cr4
	ret
_getcr4 Endp


_setdr7 Proc  dwDr7:dword
		mov eax, dwDr7
		mov dr7, eax
		ret
_setdr7 	Endp


_getdr7 Proc
	mov eax, dr7
	ret
_getdr7 Endp


_vmxon Proc  dwLow:dword,dwHigh:dword
push dwHigh
push dwLow
vmxon qword ptr [esp]
add	esp,8
pushfd
pop eax
ret
_vmxon Endp


_vmclear Proc  dwLow:dword,dwHigh:dword
push dwHigh
push dwLow
vmclear qword ptr [esp]
add	esp,8
ret
_vmclear Endp


_vmptrld Proc  dwLow:dword,dwHigh:dword
push dwHigh
push dwLow
vmptrld qword ptr [esp]
add	esp,8
pushfd
pop eax
ret
_vmptrld Endp

_vmwrite Proc  dwEncode:dword,dwValue:dword
mov eax,dwEncode
vmwrite eax,dwValue
ret
_vmwrite Endp

_vmread Proc  dwEncode:dword
mov ebx,dwEncode
vmread eax,ebx
ret
_vmread Endp

_getGDTR Proc  pgdt:dword
mov eax,[pgdt]
sgdt [eax]
ret
_getGDTR Endp

_getIDTR Proc  pidt:dword
mov eax,[pidt]
sidt [eax]
ret
_getIDTR Endp

_getCS Proc
Xor eax,eax
mov ax,CS
ret
_getCS Endp

_getSS Proc
Xor eax,eax
mov ax,SS
ret
_getSS Endp

_getDS Proc
Xor eax,eax
mov ax,DS
ret
_getDS Endp

_getES Proc
Xor eax,eax
mov ax,ES
ret
_getES Endp

_getFS Proc
Xor eax,eax
mov ax,FS
ret
_getFS Endp

_getGS Proc
Xor eax,eax
mov ax,GS
ret
_getGS Endp

_getLDTR	Proc
	xor	eax, eax
	sldt	ax
	ret
_getLDTR	Endp

_getTR	Proc
	xor	eax, eax
	str	ax
	ret
_getTR	Endp




_init_vmcs Proc pvcpu:dword,InitVmcs:dword
push eax
push ebx
push ecx
push edx
push esi
push edi
push ebp
pushfd
push esp
pushfd
push pvcpu
call	InitVmcs

popfd
pop ebp
pop edi
pop esi
pop edx
pop ecx
pop ebx
pop eax

ret

_init_vmcs Endp

; guest 出口
_guest_ret Proc
popfd
pop ebp
pop edi
pop esi
pop edx
pop ecx
pop ebx
pop eax
leave
ret 8
_guest_ret Endp




; exit 入口
extern uFuncExitHandle:dword
_exit_handle Proc
CLI
push esp
push eax
push ebx
push ecx
push edx
push esi
push edi
push ebp
pushfd


push esp	; 保存环境到影子寄存器，，实际上是不存在的，，这里直接操作栈内存 充当影子寄存器
call uFuncExitHandle




STI			; 恢复影子寄存器，到 Guest 寄存器
popfd
pop ebp
pop edi

pop esi
pop edx
pop ecx
pop ebx
pop eax
pop esp
vmresume
; 如果失败了 调用它自己 并在 uFuncExitHandle 中检查错误信息
; 同时触发 KdBugCheckDebug,仅在调试模式下有效
call _exit_handle
_exit_handle Endp


_vmlaunch Proc
vmlaunch
ret
_vmlaunch Endp


;cpuid使用eax作为输入参数，eax，ebx，ecx，edx作为输出参数
_getcpuid Proc uEncodeEax:dword,uEax:dword,uEbx:dword,uEcx:dword,uEdx:dword

; 这里需要把 ebx 也保存了
push ebx
push esi
mov eax,[uEncodeEax]
cpuid
mov esi,uEax
mov [esi],eax
mov esi,uEbx
mov [esi],ebx
mov esi,uEcx
mov [esi],ecx
mov esi,uEdx
mov [esi],edx
pop esi
pop ebx

ret
_getcpuid Endp



_vmxstop Proc
push eax
push ecx
mov eax,41414141h
mov ecx,42424242h
cpuid
pop ecx
pop eax
ret
_vmxstop Endp



_vmxoff Proc
vmxoff
ret
_vmxoff Endp

; 这个函数不返回
_vmxoff_no_root Proc pEsp:dword,pEip:dword,pGuestEsp:dword

vmxoff

mov eax,pEip
mov dwEip,eax
mov eax,pGuestEsp
mov dwEsp,eax

mov esp,pEsp
STI			; 恢复影子寄存器，到 Guest 寄存器
popfd
pop ebp
pop edi
pop esi
pop edx
pop ecx
pop ebx
pop eax
pop esp
mov esp , dwEsp
jmp dwEip

ret
_vmxoff_no_root Endp


End	