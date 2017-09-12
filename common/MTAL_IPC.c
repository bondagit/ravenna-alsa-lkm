///////////////////////////////////////////////////////////////////////////////
// Copyright 1993-2017 Merging Technologies S.A., All Rights Reserved
///////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
	#pragma warning(disable : 4996)
#else
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdbool.h>
	#include <string.h>
	//#include <sys/msg.h>
	//#include <sys/ipc.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>


	#include <errno.h>
	
	#include <pthread.h>
#endif

//#include "../../../Pyramidion/Sources/Ravenna/ALC/rvstd/src/rv_log.h"

#define __FILE_NAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)
#define LOG_MAX_LEVEL LOG_DEBUG

#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */

#define RV_LOG_BLACK(x)		{printf("\033[30m\033[1m");	printf x; printf("\033[0m");}
#define RV_LOG_RED(x)		{printf("\033[31m\033[1m");	printf x; printf("\033[0m");}
#define RV_LOG_GREEN(x)		{printf("\033[32m\033[1m");	printf x; printf("\033[0m");}
#define RV_LOG_YELLOW(x)	{printf("\033[33m\033[1m");	printf x; printf("\033[0m");}
#define RV_LOG_BLUE(x)		{printf("\033[34m\033[1m");	printf x; printf("\033[0m");}
#define RV_LOG_MAGENTA(x)	{printf("\033[35m\033[1m");	printf x; printf("\033[0m");}
#define RV_LOG_CYAN(x)		{printf("\033[36m\033[1m");	printf x; printf("\033[0m");}
#define RV_LOG_WHITE(x)		{printf("\033[37m\033[1m");	printf x; printf("\033[0m");}

#include <syslog.h>
#define rv_log(log_level, ...)  \
	{\
	char cBuffer[512];\
	snprintf(cBuffer, 512, "In %s, line %i: ", __FILE_NAME__, __LINE__);\
	snprintf(cBuffer + strlen(cBuffer), 512 - strlen(cBuffer), __VA_ARGS__);\
	syslog(log_level, "%s", cBuffer);\
	if(log_level <= LOG_MAX_LEVEL)\
		{\
		printf("In %s, line %i: ", __FILE_NAME__, __LINE__); \
		if(log_level < LOG_WARNING) \
			RV_LOG_RED((__VA_ARGS__)) \
		else if(log_level == LOG_WARNING) \
			RV_LOG_YELLOW((__VA_ARGS__)) \
		else if(log_level == LOG_INFO) \
			RV_LOG_GREEN((__VA_ARGS__)) \
		else if(log_level == LOG_DEBUG) \
			RV_LOG_MAGENTA((__VA_ARGS__)) \
		else \
			printf(__VA_ARGS__);\
		/*printf("\r\n");*/ fflush(stdout);\
		}\
			}


#include "MTAL_IPC.h"

#define DISPLAY_ELAPSETIME_THRESHOLD 500 // [us]

#ifdef __linux__ 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// LINUX
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// send command
#define PTPV2D_PTP_SERVER_FIFO "/tmp/PTPv2d_PTP_server_fifo"
#define PTPV2D_ZMAN_SERVER_FIFO "/tmp/PTPv2d_ZMAN_server_fifo"

	// send command's answer
#define PTPV2D_PTP_CLIENT_FIFO "/tmp/PTPv2d_PTP_client_fifo"
#define PTPV2D_ZMAN_CLIENT_FIFO "/tmp/PTPv2d_ZMAN_client_fifo"


#define MAX_CONTROLLER_BUFFER_SIZE	2048
typedef struct
{
	uint32_t ui32MsgSize;
	uint32_t ui32MsgSeqId;
	uint32_t ui32MsgId;

	int		iResult;

} MTPTP_IPC_MsgBlockBase;

typedef struct
{	
	MTPTP_IPC_MsgBlockBase	base;

	uint8_t					pui8Buffer[MAX_CONTROLLER_BUFFER_SIZE];

} MTPTP_IPC_MsgBlock;


typedef struct
{
	int s_bInitialized;
	int s_bPTPv2d;

	uint32_t ui32LocalServerPrefix;
	uint32_t ui32PeerServerPrefix;

	MTAL_IPC_IOCTL_CALLBACK s_callback;
	void* s_callback_user;

	int s_server_fd; // it's the local server   RX
	int s_client_fd; // it's the client of the local server   TX

	int s_peer_server_fd; // it's the peer/remote server TX
	int s_peer_client_fd; // it's the client of the peer server -> in fact it's the local client   RX

	uint32_t s_ui32MsgSeqId;

	pthread_t s_pthread;
	pthread_mutex_t s_lock;

} TMTAL_IPC_Instance;


static int private_init(int bPTPMode, uint32_t ui32LocalServerPrefix, uint32_t ui32PeerServerPrefix, MTAL_IPC_IOCTL_CALLBACK cb, void* cb_user, uintptr_t* pptrHandle);
static int create_thread(TMTAL_IPC_Instance* pTMTAL_IPC_Instance);
static void destroy_thread(TMTAL_IPC_Instance* pTMTAL_IPC_Instance);
static void* thread_proc(void *pParam);



////////////////////////////////////////////////////////////////
int MTAL_IPC_init(uint32_t ui32LocalServerPrefix, uint32_t ui32PeerServerPrefix, MTAL_IPC_IOCTL_CALLBACK cb, void* cb_user, uintptr_t* pptrHandle)
{
	return private_init(0, ui32LocalServerPrefix, ui32PeerServerPrefix, cb, cb_user, pptrHandle);
}

////////////////////////////////////////////////////////////////
//int MTAL_IPC_init(int bPTPv2d, .... MTAL_IPC_IOCTL_CALLBACK cb, void* cb_user)
//{
//	return private_init(1, ui32LocalServerPrefix, ui32PeerServerPrefix,  cb, cb_user, pptrHandle);
//}

int private_init(int bPTPv2dMode, uint32_t ui32LocalServerPrefix, uint32_t ui32PeerServerPrefix, MTAL_IPC_IOCTL_CALLBACK cb, void* cb_user, uintptr_t* pptrHandle)
{
	// static init
	{
		struct timespec clock_res;
		clock_getres(CLOCK_MONOTONIC, &clock_res);
		rv_log(LOG_DEBUG, "clock_res = %u [s] %u [ns]\n", (unsigned int)clock_res.tv_sec, (unsigned int)clock_res.tv_nsec);
	}


	// arguments validation
	if (!pptrHandle)
	{
		return -1;
	}
	if (!cb)
	{
		//rv_log(LOG_ERR, "error: in server mode, callback function is mandatory\n");
		return -2;
	}
	*pptrHandle = 0;


	TMTAL_IPC_Instance* pTMTAL_IPC_Instance = (TMTAL_IPC_Instance*)malloc(sizeof(TMTAL_IPC_Instance));
	if (!pTMTAL_IPC_Instance)
	{
		return -3;
	}

	// reset
	memset(pTMTAL_IPC_Instance, 0, sizeof(TMTAL_IPC_Instance));	
	pTMTAL_IPC_Instance->s_server_fd = -1;
	pTMTAL_IPC_Instance->s_client_fd = -1;

	pTMTAL_IPC_Instance->s_peer_client_fd = -1;
	pTMTAL_IPC_Instance->s_peer_server_fd = -1;
	


	// initialization
	pTMTAL_IPC_Instance->ui32LocalServerPrefix = ui32LocalServerPrefix;
	pTMTAL_IPC_Instance->ui32PeerServerPrefix = ui32PeerServerPrefix;
	pTMTAL_IPC_Instance->s_bPTPv2d = bPTPv2dMode;
	pTMTAL_IPC_Instance->s_callback = cb;
	pTMTAL_IPC_Instance->s_callback_user = cb_user;
	
	char szFIFO_Name[256];
	

	////////////////////////////////////////////
	// local server FIFOs
	////////////////////////////////////////////
	// name of the fifo which is the server for this instance
	{
		sprintf(szFIFO_Name, "/tmp/MTAL_IPC_Request_%u", ui32LocalServerPrefix);
		// create fifo if not exist
		if ((mkfifo(szFIFO_Name, 0664) == -1) && (errno != EEXIST)) {
			rv_log(LOG_ERR, "Unable to create fifo %s", szFIFO_Name);
			return -1;
		}
		// open fifo
		if ((pTMTAL_IPC_Instance->s_server_fd = open(szFIFO_Name, O_RDWR | (bPTPv2dMode ? O_NONBLOCK : 0))) == -1)
		{
			rv_log(LOG_ERR, "\n open file(%s) failed; errno = %i\n", szFIFO_Name, errno);
			return -4;
		}
	}

	////////////////////////////////////////////	
	// name of the fifo which is the client to server for this instance
	{
		sprintf(szFIFO_Name, "/tmp/MTAL_IPC_Answer_%u", ui32LocalServerPrefix);
		// create fifo if not exist
		if ((mkfifo(szFIFO_Name, 0664) == -1) && (errno != EEXIST)) {
			rv_log(LOG_ERR, "Unable to create fifo %s", szFIFO_Name);
			return -1;
		}
		// open fifo
		if ((pTMTAL_IPC_Instance->s_client_fd = open(szFIFO_Name, O_RDWR)) == -1)
		{
			rv_log(LOG_ERR, "\n open file(%s) failed; errno = %i\n", szFIFO_Name, errno);
			return -4;
		}
	}



	////////////////////////////////////////////
	// peer/remote server FIFOs
	////////////////////////////////////////////
	// name of the fifo which is the server for this instance
	{
		sprintf(szFIFO_Name, "/tmp/MTAL_IPC_Request_%u", ui32PeerServerPrefix);
		// create fifo if not exist
		if ((mkfifo(szFIFO_Name, 0664) == -1) && (errno != EEXIST)) {
			rv_log(LOG_ERR, "Unable to create fifo %s", szFIFO_Name);
			return -1;
		}
		// open fifo
		if ((pTMTAL_IPC_Instance->s_peer_server_fd = open(szFIFO_Name, O_RDWR)) == -1)
		{
			rv_log(LOG_ERR, "\n open file(%s) failed; errno = %i\n", szFIFO_Name, errno);
			return -4;
		}
	}
	
	// name of the fifo which is the client to server for this instance
	{
		sprintf(szFIFO_Name, "/tmp/MTAL_IPC_Answer_%u", ui32PeerServerPrefix);
		// create fifo if not exist
		if ((mkfifo(szFIFO_Name, 0664) == -1) && (errno != EEXIST)) {
			rv_log(LOG_ERR, "Unable to create fifo %s", szFIFO_Name);
			return -1;
		}
		// open fifo
		if ((pTMTAL_IPC_Instance->s_peer_client_fd = open(szFIFO_Name, O_RDWR)) == -1)
		{
			rv_log(LOG_ERR, "\n open file(%s) failed; errno = %i\n", szFIFO_Name, errno);
			return -4;
		}
	}
	

	// init mutex
	if (pthread_mutex_init(&pTMTAL_IPC_Instance->s_lock, NULL) != 0)
	{
		rv_log(LOG_ERR, "\n mutex init failed\n");
		return -4;
	}

	// create thread if needed
	if (!bPTPv2dMode)
	{
		if (create_thread(pTMTAL_IPC_Instance))
		{
			rv_log(LOG_ERR, "Failed to create thread\n");
			return -2;
		}
	}

	pTMTAL_IPC_Instance->s_bInitialized = 1;
	
	*pptrHandle = (uintptr_t)pTMTAL_IPC_Instance;
	return 0;
}



////////////////////////////////////////////////////////////////
void MTAL_IPC_destroy(uintptr_t ptrHandle)
{
	if (!ptrHandle)
	{
		// message
		return;
	}

	TMTAL_IPC_Instance* pTMTAL_IPC_Instance = (TMTAL_IPC_Instance*)ptrHandle;

	if (!pTMTAL_IPC_Instance->s_bPTPv2d)
	{
		destroy_thread(pTMTAL_IPC_Instance);
	}

	if (pTMTAL_IPC_Instance->s_server_fd >= 0)
	{
		close(pTMTAL_IPC_Instance->s_server_fd);
		pTMTAL_IPC_Instance->s_server_fd = -1;
	}
	if (pTMTAL_IPC_Instance->s_client_fd >= 0)
	{
		close(pTMTAL_IPC_Instance->s_client_fd);
		pTMTAL_IPC_Instance->s_client_fd = -1;
	}

	if (pTMTAL_IPC_Instance->s_peer_server_fd >= 0)
	{
		close(pTMTAL_IPC_Instance->s_peer_server_fd);
		pTMTAL_IPC_Instance->s_peer_server_fd = -1;
	}
	if (pTMTAL_IPC_Instance->s_peer_client_fd >= 0)
	{
		close(pTMTAL_IPC_Instance->s_peer_client_fd);
		pTMTAL_IPC_Instance->s_peer_client_fd = -1;
	}

	pthread_mutex_destroy(&pTMTAL_IPC_Instance->s_lock);

	pTMTAL_IPC_Instance->s_bInitialized = 0;

	free(pTMTAL_IPC_Instance);
}


static int send_buffer_to_fifo(int fd, uint8_t* pBuffer, uint32_t ui32BufferSize);
static int process_message_from_fifo(TMTAL_IPC_Instance* pTMTAL_IPC_Instance, int fd);


static uint32_t get_current_time(); // [us]
static uint32_t get_elapse_time(uint32_t ui32startTime); // [us]
static void display_elapse_time(char* pcText, uint32_t ui32startTime);


static int read_answer_from_fifo(int fd_client, uint32_t ui32SeqId, void* pvOutBuffer, uint32_t* pui32OutBufferSize);



//////////////////////////////////////////////////////////////////
//int MTPTP_IPC_get_FIFO_fd(uintptr_t ptrHandle)
//{
//	return pTMTAL_IPC_Instance->s_server_fd;
//}
//
//////////////////////////////////////////////////////////////////
//int MTPTP_IPC_process_FIFO(uintptr_t ptrHandle, void* user)
//{
//	pTMTAL_IPC_Instance->s_callback_user = user;
//	return process_message_from_fifo(s_server_fd);
//}

////////////////////////////////////////////////////////////////
int MTAL_IPC_SendIOCTL(uintptr_t ptrHandle, uint32_t ui32MsgId, void const * pvInBuffer, uint32_t ui32InBufferSize, void* pvOutBuffer, uint32_t* pui32OutBufferSize)
{
	if (ui32InBufferSize > MAX_CONTROLLER_BUFFER_SIZE || ui32InBufferSize > MAX_CONTROLLER_BUFFER_SIZE)
	{
		rv_log(LOG_ERR, "error: buffer size error\n");
		return -1;
	}
    
    TMTAL_IPC_Instance* pTMTAL_IPC_Instance = (TMTAL_IPC_Instance*)ptrHandle;
	uint32_t ui32StartTime = get_current_time(); // [us]

	int iRet = -1;
	pthread_mutex_lock(&pTMTAL_IPC_Instance->s_lock);
	{
		// send message
		{
			MTPTP_IPC_MsgBlock MTPTP_IPC_MsgBlock_tmp;

			MTPTP_IPC_MsgBlock_tmp.base.ui32MsgSize = sizeof(MTPTP_IPC_MsgBlockBase) + ui32InBufferSize;
			MTPTP_IPC_MsgBlock_tmp.base.ui32MsgSeqId = ++pTMTAL_IPC_Instance->s_ui32MsgSeqId;
			MTPTP_IPC_MsgBlock_tmp.base.ui32MsgId = ui32MsgId;
			MTPTP_IPC_MsgBlock_tmp.base.iResult = 0;
			if (pvInBuffer && ui32InBufferSize > 0)
			{
				memcpy(&MTPTP_IPC_MsgBlock_tmp.pui8Buffer, pvInBuffer, ui32InBufferSize);
			}
			ssize_t bytes_to_write = MTPTP_IPC_MsgBlock_tmp.base.ui32MsgSize;
			iRet = send_buffer_to_fifo(pTMTAL_IPC_Instance->s_peer_server_fd, (uint8_t*)&MTPTP_IPC_MsgBlock_tmp, bytes_to_write);
		}

		if (iRet == 0)
		{
			// wait answer
			iRet = read_answer_from_fifo(pTMTAL_IPC_Instance->s_peer_client_fd, pTMTAL_IPC_Instance->s_ui32MsgSeqId, pvOutBuffer, pui32OutBufferSize);
		}
		else
		{
			rv_log(LOG_ERR, "send_buffer_to_fifo(%u) failed\n", iRet);
		}
	}
	pthread_mutex_unlock(&pTMTAL_IPC_Instance->s_lock);

	if (get_elapse_time(ui32StartTime) > DISPLAY_ELAPSETIME_THRESHOLD)
	{
		char display[] = "MTPTP_IPC_SendIOCTL";
		display_elapse_time(display, ui32StartTime);
	}

	if (iRet < 0)
	{
		rv_log(LOG_ERR, "MTPTP_IPC_SendIOCTL(%u) failed\n", ui32MsgId);
	}
	return iRet;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// ui32Timeout in [s]
static int netSelect(int fd, uint32_t ui32Timeout_sec, uint32_t ui32Timeout_usec)
{
	int    ret;
	int nfds;
	fd_set readfds;
	struct timeval tv, *tv_ptr;


	/* Setup fd_set structure for select function */

	FD_ZERO(&readfds);

	FD_SET(fd, &readfds);

	/* Set time to wait if any, else setup NULL pointer */


	if (ui32Timeout_sec > 0 || ui32Timeout_usec > 0)
	{
		tv.tv_sec = ui32Timeout_sec; // [s]
		tv.tv_usec = ui32Timeout_usec;
		tv_ptr = &tv;
	}
	else
	{
		tv_ptr = 0;
	}

	/* Find highest Number Socket for select() function */
	nfds = fd;


	/* Call select function to check all receive sockets with optional timeout */
	ret = select(nfds + 1,  // nfds (highest socket number + 1)
		&readfds,  // readfds
		0,         // writefds
		0,         // exceptfds
		tv_ptr     // timeout structure pointer or NULL
	);

	if (ret == -1)
	{
		if (errno == EAGAIN || errno == EINTR)
		{
			rv_log(LOG_ERR, "netSelect: errno EAGAIN or EINTR (%d)\n", errno);
			ret = 0;
		}
		else
		{
			rv_log(LOG_ERR, "netSelect: unexpected errno: %d, select returns %d\n", errno, ret);
		}
	}
	else if (ret == 0)
	{
		//rv_log(LOG_DEBUG, "netSelect: errno: %d, select returns %d\n", errno, ret);
	}
	else if(!FD_ISSET(nfds, &readfds))
	{
		rv_log(LOG_ERR, "netSelect: errno: %d, select returns %d but fd not part of rfds!?!?!?\n", errno, ret);
	}
	else
	{
		//rv_log(LOG_DEBUG, "netSelect:select returns %d; timeout is %u [s] %u [us]\n", ret, tv.tv_sec, tv.tv_usec);
	}
	//rv_log(LOG_DEBUG, "netSelect: return: %d\n", ret);
	return ret;
}

////////////////////////////////////////////////////////////////
int send_buffer_to_fifo(int fd, uint8_t* pBuffer, uint32_t ui32BufferSize)
{
	int iRet = 0;
	ssize_t bytes_written = write(fd, pBuffer, ui32BufferSize);
	if (bytes_written != ui32BufferSize) 
	{
		rv_log(LOG_ERR, "send_answer:write(%i) error: EBADF = %u, %u\n", fd, EBADF, errno);
		iRet = -1;
	}
	return iRet;
}

////////////////////////////////////////////////////////////////
// return latest message in the fifo.
int read_answer_from_fifo(int fd_client, uint32_t ui32SeqId, void* pvOutBuffer, uint32_t* pui32OutBufferSize)
{
	uint8_t pui8Buffer[sizeof(MTPTP_IPC_MsgBlock )];

	uint32_t ui32MaxRetryDuration = 500000; // [us]
	uint32_t ui32StartTime = get_current_time(); // [us]
		
	int iRetryCounter = 0;
	do
	{
		if (netSelect(fd_client, 0, 100000) > 0)
		{
			ssize_t bytes_read = read(fd_client, pui8Buffer, sizeof(pui8Buffer));
			if (bytes_read <= 0)
			{
				rv_log(LOG_ERR, "failed to read from fifo: bytes read: %u errno: %u\n", (unsigned int)bytes_read, errno);
			}
			////rv_log(LOG_DEBUG, "bytes_read = %i\n", bytes_read);

			if (bytes_read > 0 && (unsigned)bytes_read < sizeof(MTPTP_IPC_MsgBlockBase))
			{
				rv_log(LOG_ERR, "read too less bytes from fifo bytes read: %u errno: %u\n", (unsigned int)bytes_read, errno);
				return -1;
			}

			MTPTP_IPC_MsgBlockBase* pMTPTP_IPC_MsgBlockBase = (MTPTP_IPC_MsgBlockBase*)pui8Buffer;
			int32_t i32RemainingBytes = bytes_read;
			while ((unsigned)i32RemainingBytes >= sizeof(MTPTP_IPC_MsgBlockBase))
			{
				uint32_t ui32MsgSize = pMTPTP_IPC_MsgBlockBase->ui32MsgSize;

				//rv_log(LOG_DEBUG, "SeqId = %u, %u\n", pMTPTP_IPC_MsgBlockBase->ui32MsgSeqId, ui32SeqId);
				if (pMTPTP_IPC_MsgBlockBase->ui32MsgSeqId != ui32SeqId)
				{
					rv_log(LOG_ERR, "Answer doesn't match our message SeqId = %u, %u\n", pMTPTP_IPC_MsgBlockBase->ui32MsgSeqId, ui32SeqId);
				}
				else
				{
					if (pvOutBuffer && pui32OutBufferSize)
					{
						*pui32OutBufferSize = ui32MsgSize < *pui32OutBufferSize ? ui32MsgSize : *pui32OutBufferSize;
						memcpy(pvOutBuffer, (uint8_t*)pMTPTP_IPC_MsgBlockBase + sizeof(MTPTP_IPC_MsgBlockBase), *pui32OutBufferSize);
					}
					if (pMTPTP_IPC_MsgBlockBase->iResult != 0)
					{
						rv_log(LOG_ERR, "pMTPTP_IPC_MsgBlockBase->iResult = %i\n", pMTPTP_IPC_MsgBlockBase->iResult);
					}
					//display_elapse_time("read_answer_from_fifo", ui32StartTime);
					return pMTPTP_IPC_MsgBlockBase->iResult;
				}

				pMTPTP_IPC_MsgBlockBase = (MTPTP_IPC_MsgBlockBase*)((uint8_t*)pMTPTP_IPC_MsgBlockBase + ui32MsgSize);
				i32RemainingBytes -= ui32MsgSize;
			}
		}		
		
		uint32_t ui32ElapseTime = get_elapse_time(ui32StartTime); // [us];
		if (ui32ElapseTime > ui32MaxRetryDuration)
		{
			rv_log(LOG_ERR, "read_answer_from_fifo error: maximum retry time reached\n");
			break;
		}
		
		rv_log(LOG_WARNING, "retry %u, %u [us]\n", ++iRetryCounter, ui32ElapseTime);
	} while (1);
	return -1;
}

////////////////////////////////////////////////////////////////
// return: 
//		-1: fail
//		0: success
//		1: means we received the END message (thread must died)
static int process_message_from_fifo(TMTAL_IPC_Instance* pTMTAL_IPC_Instance, int fd)
{
	MTPTP_IPC_MsgBlock MTPTP_IPC_MsgBlock_tmp;
	ssize_t bytes_read;
	
	//uint32_t ui32StartTime = get_current_time(); // [us]
	// get a message
	bytes_read = read(fd, &MTPTP_IPC_MsgBlock_tmp, sizeof(MTPTP_IPC_MsgBlock_tmp));
	if ((bytes_read == -1 && (errno == EINTR || errno == EAGAIN))
		|| bytes_read == 0)
	{
		//display_elapse_time("fifo read", ui32StartTime);
		return 0;
	}
	else if (bytes_read == -1)
	{
		rv_log(LOG_ERR, "read error: errno = %i\n", errno);
	}
	else if (bytes_read == 1)
	{ // it is time to die
		rv_log(LOG_INFO, "it is time to die\n");
		return 1;
	}
	else if ((unsigned)bytes_read < sizeof(MTPTP_IPC_MsgBlockBase))
	{
		// wrong message
		rv_log(LOG_ERR, "wrong message size: %u expected >= %lu", (unsigned int)bytes_read, sizeof(MTPTP_IPC_MsgBlockBase));
		return -1;
	}
	//display_elapse_time("fifo read", ui32StartTime);
	
	
	//rv_log(LOG_DEBUG, "SeqId = %u\n", MTPTP_IPC_MsgBlock_tmp.base.ui32MsgSeqId);
	// call message_proc()	
	uint8_t pui8OutBuffer[MAX_CONTROLLER_BUFFER_SIZE];
	uint32_t ui32OutBufferSize = sizeof(pui8OutBuffer);
	MTPTP_IPC_MsgBlock_tmp.base.iResult = pTMTAL_IPC_Instance->s_callback(pTMTAL_IPC_Instance->s_callback_user, 
        MTPTP_IPC_MsgBlock_tmp.base.ui32MsgId,
		MTPTP_IPC_MsgBlock_tmp.pui8Buffer, bytes_read - sizeof(MTPTP_IPC_MsgBlockBase),
		pui8OutBuffer, &ui32OutBufferSize);
	MTPTP_IPC_MsgBlock_tmp.base.ui32MsgSize = sizeof(MTPTP_IPC_MsgBlockBase) + ui32OutBufferSize;
	memcpy(MTPTP_IPC_MsgBlock_tmp.pui8Buffer, pui8OutBuffer, ui32OutBufferSize);

	// send answer
	uint32_t ui32Seq = MTPTP_IPC_MsgBlock_tmp.base.ui32MsgSeqId;
	MTPTP_IPC_MsgBlock_tmp.base.ui32MsgSeqId = ui32Seq;
	send_buffer_to_fifo(pTMTAL_IPC_Instance->s_client_fd, (uint8_t*)&MTPTP_IPC_MsgBlock_tmp, sizeof(MTPTP_IPC_MsgBlockBase) + ui32OutBufferSize);

	return 0;
}

////////////////////////////////////////////////////////////////
static int create_thread(TMTAL_IPC_Instance* pTMTAL_IPC_Instance)
{
	int iRet = 0;
	//set up thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	//increase the stack size (because the threads in ravenna daemon use more than the default amount)
	//pthread_attr_setstacksize(&attr, 1024 * 1024 * 16);

	//create and start the thread
	if (pthread_create(&pTMTAL_IPC_Instance->s_pthread, &attr, thread_proc, pTMTAL_IPC_Instance))
	{
		rv_log(LOG_ERR, "pthread_create failed\n");
		
		iRet = -1;
	}
	pthread_attr_destroy(&attr);
	return iRet;
}

////////////////////////////////////////////////////////////////
static void destroy_thread(TMTAL_IPC_Instance* pTMTAL_IPC_Instance)
{
	if (pTMTAL_IPC_Instance->s_pthread)
	{
		rv_log(LOG_DEBUG, "waiting on thread exit...\n");

		// send short message to himself to force thread to exit
		uint8_t buf[1];
		send_buffer_to_fifo(pTMTAL_IPC_Instance->s_server_fd, buf, sizeof(buf));


		pthread_join(pTMTAL_IPC_Instance->s_pthread, 0);
		pTMTAL_IPC_Instance->s_pthread = 0;
	}	
}

////////////////////////////////////////////////////////////////
static void* thread_proc(void *pParam)
{
	TMTAL_IPC_Instance* pTMTAL_IPC_Instance = (TMTAL_IPC_Instance*)pParam;

	rv_log(LOG_DEBUG, "thread start...\n");
	
	while (1)
	{
		if (netSelect(pTMTAL_IPC_Instance->s_server_fd, 10, 0))
		{
			if (process_message_from_fifo(pTMTAL_IPC_Instance, pTMTAL_IPC_Instance->s_server_fd) == 1)
			{ // it is time to die
				break;
			}
		}
	}

	rv_log(LOG_DEBUG, "thread end...\n");
	return NULL;
}

////////////////////////////////////////////////////////////////
static uint32_t get_current_time() // [us]
{
	struct timespec clock;
	clock_gettime(CLOCK_MONOTONIC, &clock);
	return clock.tv_sec * 1000000 + clock.tv_nsec / 1000;
}

////////////////////////////////////////////////////////////////
static uint32_t get_elapse_time(uint32_t ui32StartTime) // [us]
{
	return get_current_time() - ui32StartTime; // [us];
}

////////////////////////////////////////////////////////////////
static void display_elapse_time(char* pcText, uint32_t ui32StartTime) // [us]
{
	if (!pcText)
	{
		rv_log(LOG_ERR, "Error: pcText is NULL\n");
		return;
	}
	rv_log(LOG_DEBUG, "Duration %s: %u [us]\n", pcText, get_elapse_time(ui32StartTime));
}

#elif WIN32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WIN32
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
int MTAL_IPC_init(uint32_t ui32LocalServerPrefix, uint32_t ui32PeerServerPrefix, MTAL_IPC_IOCTL_CALLBACK cb, void* cb_user, uintptr_t* pptrHandle)
{
	return 0;
}

////////////////////////////////////////////////////////////////
void MTAL_IPC_destroy(uintptr_t ptrHandle)
{
}

////////////////////////////////////////////////////////////////
int MTAL_IPC_SendIOCTL(uintptr_t ptrHandle, uint32_t  ui32MsgId, void const * pui8InBuffer, uint32_t ui32InBufferSize, void* pui8OutBuffer, uint32_t* pui32OutBufferSize)
{
	return 0;
}

#endif