/****************************************************************************
*
*  Module Name    : RTP_streams_manager.h
*  Version        : 
*
*  Abstract       : RAVENNA/AES67 ALSA LKM
*
*  Written by     : van Kempen Bertrand
*  Date           : 25/07/2010
*  Modified by    : Baume Florian
*  Date           : 13/01/2017
*  Modification   : C port (source: RTP_streams_manager.hpp)
*  Known problems : None
*
* Copyright(C) 2017 Merging Technologies
*
* RAVENNA/AES67 ALSA LKM is free software; you can redistribute it and / or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* RAVENNA/AES67 ALSA LKM is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with RAVAENNA ALSA LKM ; if not, see <http://www.gnu.org/licenses/>.
*
****************************************************************************/


#pragma once

#include "EtherTubeInterfaces.h"
#include "MTAL_DP.h"
#include "RTP_audio_stream.h"
#include "RTP_stream_info.h"

#define _MAX_NICS 2	

//////////////////////////////////////////////////////////////
typedef struct {
	// CRTP_audio_streams
	// Sources

	// Sources are only used by RTXCore Process so we not have to use a multi-processor mutex (CMTAL_RTSSDLLMutex)
    void* m_csSourceRTPStreams;

    volatile unsigned short	m_usNumberOfRTPSourceStreams;
    TRTP_audio_stream_handler m_apRTPSourceStreams[MAX_SOURCE_STREAMS * _MAX_NICS * 2]; // double the size because remove/add all stream, we potentially need object that are being use. We then ensure that object are able to be used
    TRTP_audio_stream_handler* m_apRTPSourceOrderedStreams[MAX_SOURCE_STREAMS * _MAX_NICS];

	uint32_t m_ui32RTCPPacketCountdown;	// in [audio frame]

	// Sinks
	void* m_csSinkRTPStreams;
    //void* m_acsSinkRTPStreams[_MAX_NICS]; // ST2022-7: we use two locks to be able to receive concurrently on both NICS

	unsigned short m_usNumberOfNICS;

	unsigned short m_ausNumberOfRTPSinkStreams[_MAX_NICS];
	TRTP_audio_stream_handler m_apRTPSinkStreams[MAX_SINK_STREAMS * _MAX_NICS * 2]; // double the size because remove/add all stream, we potentially need object that are being use. We then ensure that object are able to be used
	TRTP_audio_stream_handler* m_apRTPSinkOrderedStreams[_MAX_NICS][MAX_SINK_STREAMS];


	//f10bCMTAL_PerfMonMinMax<uint32_t> m_pmmmLastProcessedRTPDeltaFromTIC;
	//f10bCMTAL_PerfMonMinMax<uint32_t> m_pmmmLastSentRTPDeltaFromTIC;


	rtp_audio_stream_ops* m_pManager;
	TEtherTubeNetfilter* m_pEth_netfilter[_MAX_NICS];

} TRTP_streams_manager;


//////////////////////////////////////////////////////////////
//int init_(TRTP_streams_manager* self, rtp_audio_stream_ops* pManager, TEtherTubeNetfilter* pEth_netfilter);
int init_(TRTP_streams_manager* self, rtp_audio_stream_ops* pManager, TEtherTubeNetfilter* pEth_netfilter);
void destroy_(TRTP_streams_manager* self);

EDispatchResult process_UDP_packet(TRTP_streams_manager* self, unsigned char byNICId, TUDPPacketBase* pUDPPacketBase, uint32_t packetsize);

int add_RTP_stream_(TRTP_streams_manager* self, TRTP_stream_info* pRTPStreamInfo, uint64_t* phRTPStream);
int remove_RTP_stream_(TRTP_streams_manager* self, uint64_t hRTPStream);
void remove_all_RTP_streams(TRTP_streams_manager* self);
int update_RTP_stream_name(TRTP_streams_manager* self, const TRTP_stream_update_name* pRTP_stream_update_name);
int get_RTPStream_status_(TRTP_streams_manager* self, uint64_t hRTPStream, TRTP_stream_status* pstream_status);

void attached_stream(TRTP_streams_manager* self, TRTP_stream* pRTPStream);
void detached_stream(TRTP_streams_manager* self, TRTP_stream* pRTPStream);

uint8_t GetNumberOfSources(TRTP_streams_manager* self);
uint8_t GetNumberOfSinks(TRTP_streams_manager* self);

//int GetSinkStats(uint8_t ui8StreamIdx, TRTPStreamStats* pRTPStreamStats);
int GetSinkStatsFromTIC(TRTP_streams_manager* self, uint8_t ui8StreamIdx, TRTPStreamStatsFromTIC* pRTPStreamStatsFromTIC);
int GetMinSinkAheadTime(TRTP_streams_manager* self, TSinkAheadTime* pSinkAheadTime);
int GetMinMaxSinksJitter(TRTP_streams_manager* self, TSinksJitter* pSinksJitter);

//f10bint GetLastProcessedSinkFromTIC(TRTP_streams_manager* self, TLastProcessedRTPDeltaFromTIC* pLastProcessedRTPDeltaFromTIC);
//f10bint GetLastSentSourceFromTIC(TRTP_streams_manager* self, TLastSentRTPDeltaFromTIC* pLastSentRTPDeltaFromTIC);

// Process
// following methods must be call by the AudioEngine thread
void prepare_buffer_lives(TRTP_streams_manager* self);
void frame_process_begin(TRTP_streams_manager* self);
void frame_process_end(TRTP_streams_manager* self);
#ifdef UNDER_RTSS
	friend class CRTPStreamsOutgoingThread;
#endif
void send_outgoing_packets(TRTP_streams_manager* self);




#ifdef UNDER_RTSS
public:
	HRESULT	GetLiveInInfo(TRTP_streams_manager* self, DWORD dwIndexAt1FS, TRTXLiveInfo* pRTXLiveInfo) const;
	HRESULT	GetLiveOutInfo(TRTP_streams_manager* self, DWORD dwIndexAt1FS, TRTXLiveInfo* pRTXLiveInfo) const;
#endif //UNDER_RTSS


