#pragma once

#include <ntddk.h>

enum PagingType
{
	NonSupport,
	BIT32PagingMode,
	PAEMode,
	IA32eMode
};


enum PagingSzie
{
	s1GMode,
	s2MMode,
	s4KMode
};




ULONG GetPagingModeType();
ULONG GetPagingSizeMode();