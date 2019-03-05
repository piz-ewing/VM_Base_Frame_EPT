#pragma once

#ifdef DBG
#define  KdPrintExitReason _KdPrintExitReason
#define  KdPrintFailValid  _KdPrintFailValid
VOID _KdPrintExitReason(ULONG uExitReason);
VOID _KdPrintFailValid(ULONG uFailValid);

#endif