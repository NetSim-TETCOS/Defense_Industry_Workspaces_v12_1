/************************************************************************************
* Copyright (C) 2019																*
* TETCOS, Bangalore. India															*
*																					*
* Tetcos owns the intellectual property rights in the Product and its content.		*
* The copying, redistribution, reselling or publication of any or all of the		*
* Product or its content without express prior written consent of Tetcos is			*
* prohibited. Ownership and / or any other right relating to the software and all	*
* intellectual property rights therein shall remain at all times with Tetcos.		*
*																					*
* This source code is licensed per the NetSim license agreement.					*
*																					*
* No portion of this source code may be used as the basis for a derivative work,	*
* or used, for any purpose other than its intended use per the NetSim license		*
* agreement.																		*
*																					*
* This source code and the algorithms contained within it are confidential trade	*
* secrets of TETCOS and may not be used as the basis for any other software,		*
* hardware, product or service.														*
*																					*
* Author:    Shashi Kant Suman	                                                    *
*										                                            *
* ----------------------------------------------------------------------------------*/

#include "stdafx.h"

static bool isLTENRConfigured = false;
static bool isLTENRTraceConfigured = false;
//Function prototype
int fn_NetSim_LTE_NR_Init_F();
int fn_NetSim_LTE_NR_Configure_F(void** var);
int fn_NetSim_LTE_NR_Finish_F();

_declspec(dllexport) int fn_NetSim_LTE_NR_Init()
{
	return fn_NetSim_LTE_NR_Init_F();
}

_declspec(dllexport) int fn_NetSim_LTE_NR_Run()
{
	if (pstruEventDetails->nSubEventType)
	{
		LTENR_SUBEVENT_CALL();
		return 0;
	}
	
	switch (pstruEventDetails->nEventType)
	{
		case NETWORK_OUT_EVENT:
			LTENR_CallEPCOut();
			break;
		case NETWORK_IN_EVENT:
			#pragma message(__LOC__"LTE-NR, Implement default network in event\n")
			break;
		case MAC_OUT_EVENT:
		{
			bool isMore = true;
			while (isMore)
			{
				isMore = LTENR_GET_PACKET_FROM_ACCESS_BUFFER();
				LTENR_CallSDAPOut();
			}
		}
			break;
		case MAC_IN_EVENT:
			LTENR_CallMACIn();
			fnNetSimError("LTE-NR, Implement default mac in event\n");
			break;
		case PHYSICAL_OUT_EVENT:
		{
			NetSim_PACKET* packet = pstruEventDetails->pPacket;
			NETSIM_ID d = pstruEventDetails->nDeviceId;
			NETSIM_ID in = pstruEventDetails->nInterfaceId;
			NETSIM_ID r = packet->nReceiverId;
			double endTime;
			if (r != 0) {
				NETSIM_ID rin = LTENR_FIND_ASSOCIATEINTERFACE(d, in, r);

				endTime = LTENR_PHY_GetSlotEndTime(d, in);

				packet->pstruPhyData->dArrivalTime = pstruEventDetails->dEventTime;
				packet->pstruPhyData->dEndTime = endTime - 1;
				packet->pstruPhyData->dPacketSize = packet->pstruMacData->dPacketSize;
				packet->pstruPhyData->dPayload = packet->pstruMacData->dPacketSize;
				packet->pstruPhyData->dStartTime = pstruEventDetails->dEventTime;

				pstruEventDetails->dEventTime = endTime - 1;
				pstruEventDetails->nEventType = PHYSICAL_IN_EVENT;
				pstruEventDetails->nDeviceId = r;
				pstruEventDetails->nDeviceType = DEVICE_TYPE(r);
				pstruEventDetails->nInterfaceId = rin;
				fnpAddEvent(pstruEventDetails);
			}
			else {
				for ( r = 0; r < NETWORK->nDeviceCount; r++)
				{
					for (NETSIM_ID rin = 0; rin < DEVICE(r + 1)->nNumOfInterface; rin++)
					{
						if (!isLTE_NRInterface(r + 1, rin + 1))
							continue;

						ptrLTENR_PROTODATA data = LTENR_PROTODATA_GET(r + 1, rin + 1);
						switch (data->deviceType)
						{
						case LTENR_DEVICETYPE_UE:
							endTime = LTENR_PHY_GetSlotEndTime(d, in);

							packet->pstruPhyData->dArrivalTime = pstruEventDetails->dEventTime;
							packet->pstruPhyData->dEndTime = endTime - 1;
							packet->pstruPhyData->dPacketSize = packet->pstruMacData->dPacketSize;
							packet->pstruPhyData->dPayload = packet->pstruMacData->dPacketSize;
							packet->pstruPhyData->dStartTime = pstruEventDetails->dEventTime;

							pstruEventDetails->dEventTime = endTime - 1;
							pstruEventDetails->nEventType = PHYSICAL_IN_EVENT;
							pstruEventDetails->nDeviceId = r + 1;
							pstruEventDetails->nDeviceType = DEVICE_TYPE(r + 1);
							pstruEventDetails->nInterfaceId = rin + 1;
							pstruEventDetails->pPacket->nReceiverId = d;
							fnpAddEvent(pstruEventDetails);
							break;
						default:
							break;
						}
					}
				}
			}
		}
			break;
		case PHYSICAL_IN_EVENT:
			fn_NetSim_Metrics_Add(pstruEventDetails->pPacket);
			fn_NetSim_WritePacketTrace(pstruEventDetails->pPacket);
			LTENR_CallRLCIn();
			break;
		default:
			fnNetSimError("LTE-NR, Unknown event type %d.\n",
						  pstruEventDetails->nEventType);
			break;
	}
	return 0;
}

_declspec(dllexport) char* fn_NetSim_LTE_NR_Trace(NETSIM_ID id)
{
	return LTENR_SUBEVNET_NAME(id);
}

_declspec(dllexport) int fn_NetSim_LTE_NR_FreePacket(NetSim_PACKET* packet)
{
	LTENR_MSG_FREE(packet);
	return 0;
}
_declspec(dllexport) int fn_NetSim_LTE_NR_CopyPacket(NetSim_PACKET* destPacket, const NetSim_PACKET* srcPacket)
{
	destPacket->pstruMacData->Packet_MACProtocol = LTENR_MSG_COPY(srcPacket);
	return 0;
}
_declspec(dllexport) int fn_NetSim_LTE_NR_Metrics(PMETRICSWRITER file)
{
	fn_NetSim_LTENR_SDAP_Metrics_F(file);
	// return fn_NetSim_LTE_Metrics_F(file);
	return 0;
}
_declspec(dllexport) int fn_NetSim_LTE_NR_Configure(void** var)
{
	isLTENRConfigured = true;
	return fn_NetSim_LTE_NR_Configure_F(var);
}

_declspec(dllexport) char* fn_NetSim_LTE_NR_ConfigPacketTrace(void* xmlNetSimNode)
{
	if (isLTENRConfigured)
	{
		string szStatus;
		szStatus = fn_NetSim_xmlConfigPacketTraceField(xmlNetSimNode, "LTENR_PACKET_INFO");
		if (!_stricmp(szStatus, "ENABLE"))
		{
			isLTENRTraceConfigured = true;
			free(szStatus);
			return "LTENR_PACKET_INFO,";
		}
		free(szStatus);
		return "";
	}
	else
		return "";
}

_declspec(dllexport) int fn_NetSim_LTE_NR_Finish()
{
	return fn_NetSim_LTE_NR_Finish_F();
}
_declspec(dllexport) int fn_NetSim_LTE_NR_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	if (!isLTENRTraceConfigured)
		return -1;


	char* s = calloc(BUFSIZ, sizeof * s);
	*ppszTrace = s;

	if (pstruPacket->pstruMacData->nMACProtocol != MAC_PROTOCOL_LTE_NR)
	{
		strcpy(s, "N/A,");
		return -2;
	}

	LTENR_MSG_WriteTrace(pstruPacket, s);
	return 0;
}

bool get_ltenr_log_status()
{
	return true;
}
bool get_ltenr_PRB_log_status()
{
	return false;
	//return true;
}
