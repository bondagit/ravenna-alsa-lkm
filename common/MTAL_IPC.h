/****************************************************************************
* 
*  Module Name    : MT_PTPv2d_IPC.h
*  
*  Abstract       : IPC for communication between Horus/zman and PTPv2d processes
* 
*  Written by     : van Kempen Bertrand
*  Date           : 22/03/2017
*  Modified by    : 
*  Date           : 
*  Modification   : 
*  Known problems : None
* 
*  Copyright (c) 1993-2017 Merging Technologies S.A., All rights reserved
****************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/////////////////////////////////////////////////////////
// Error
#define MTAL_IPC_ERR_SUCCESS		0
#define MTAL_IPC_ERR_FAIL		-1

typedef int(*MTAL_IPC_IOCTL_CALLBACK)(void* cb_user, uint32_t  ui32MsgId, void const * pui8InBuffer, uint32_t ui32InBufferSize, void* pui8OutBuffer, uint32_t* pui32OutBufferSize);

////////////////////////////////////////////////////////////////
// inputs:
//	ui32LocalServerPrefix: if 0 means no server capability
//	ui32PeerServerPrefix: if 0 means no client capability
// outputs:
//	pptrHandle handle of the IPC which must be pass to MTAL_IPC_destroy(), MTAL_IPC_SendIOCTL()
int MTAL_IPC_init(uint32_t ui32LocalServerPrefix, uint32_t ui32PeerServerPrefix, MTAL_IPC_IOCTL_CALLBACK cb, void* cb_user, uintptr_t* pptrHandle);
//int MTAL_IPC_init(int bPTPv2d, MTAL_IPC_IOCTL_CALLBACK cb, void* cb_user);
void MTAL_IPC_destroy(uintptr_t ptrHandle);

// PTPv2d only
#ifdef MTAL_IPC_PTPV2D
	int MTAL_IPC_get_FIFO_fd();
	int MTAL_IPC_process_FIFO(void* user);
#endif

////////////////////////////////////////////////////////////////
// inputs:
// ....
//	pui32OutBufferSize: input; size of pui8OutBuffer buffer [byte]
//	pui32OutBufferSize: output; number of bytes written to pui8OutBuffer 
int MTAL_IPC_SendIOCTL(uintptr_t ptrHandle, uint32_t  ui32MsgId, void const * pui8InBuffer, uint32_t ui32InBufferSize, void* pui8OutBuffer, uint32_t* pui32OutBufferSize);

#ifdef __cplusplus
}
#endif
