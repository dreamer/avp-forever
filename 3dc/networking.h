// Copyright (C) 2010 Barry Duncan. All Rights Reserved.
// The original author of this code can be contacted at: bduncan22@hotmail.com

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// DEVELOPERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _NETWORKING_H_
#define _NETWORKING_H_

#include "module.h"
#include "strategy_def.h"
#include "equipment.h"
#include "pldnet.h"

enum NetResult
{
	NET_OK,
	NET_FAIL,
	NET_NO_MESSAGES,
	NET_ERR_BUSY,
	NET_ERR_CONNECTIONLOST,
	NET_ERR_INVALIDPARAMS,
	NET_ERR_INVALIDPLAYER,
	NET_ERR_NOTLOGGEDIN,
	NET_ERR_SENDTOOBIG,
	NET_ERR_BUFFERTOOSMALL
};

NetResult Net_Initialise();
void      Net_Deinitialise();
void      Net_Disconnect();
void      Net_ConnectToAddress();
void      Net_FindAvPSessions();
int       Net_ConnectingToSession();
NetID     Net_GetNextPlayerID();
NetResult Net_Send(uint32_t messageType, NetID fromID, NetID toID, uint8_t *messageData, size_t dataSize);
NetResult Net_Receive(uint8_t *messageData, size_t &dataSize);
NetResult Net_SendSystemMessage(uint32_t messageType, NetID fromID, NetID toID, const void *messageData, size_t dataSize);
int       Net_InitLobbiedGame();
void      Net_ServiceNetwork();
uint32_t  Net_JoinGame();
NetResult Net_ConnectToSession(int sessionNumber, char *playerName);
NetResult Net_HostGame(char *playerName, char *sessionName, int species, uint16_t gameStyle, uint16_t level);
int       Net_ConnectingToLobbiedGame(char *playerName);

extern NetID AvPNetID;

static const int kMarker = 'xPvA'; // AvPx

struct MessageHeader
{
	int marker;
	bool isSystemMessage;
	uint32_t messageType;
	NetID fromID;
	NetID toID;
};

/*
Version 0 - Original multiplayer + save patch
Version 100 - Added pistol,skeeter (and new levels)
*/
const int kMultiplayerVersion = 101; // bjd - my version, not directplay compatible

struct MessageHeader; // forward declare the structure
extern const uint32_t kMessageHeaderSize;

enum NetMessageTypes
{
	NET_SYSTEM_MESSAGE,
	NET_GAME_MESSAGE
};

enum NetSystemMessages
{
	NETSYS_REQUEST_SESSION_INFO,
	NETSYS_SESSION_INFO,
	NET_CREATEPLAYERORGROUP,

	NET_SETPLAYERID,

	NET_DESTROYPLAYERORGROUP,
	NET_ADDPLAYERTOGROUP,
	NET_DELETEPLAYERFROMGROUP,
	NET_SESSIONLOST,
	NET_HOST,
//	NET_SETPLAYERORGROUPDATA,
//	NET_SETPLAYERORGROUPNAME,
//	NET_SETSESSIONDESC,
//	NET_ADDGROUPTOGROUP,
//	NET_DELETEGROUPFROMGROUP,
//	NET_SECUREMESSAGE,
//	NET_STARTSESSION,
//	NET_CHAT,
//	NET_SETGROUPOWNER,
//	NET_SENDCOMPLETE,
//	NET_PLAYERTYPE_GROUP,
	NET_PLAYERTYPE_PLAYER,
//	NET_RECEIVE_ALL,
	NET_ID_ALLPLAYERS
//	NET_ID_SERVERPLAYER,
};

const int kPlayerNameSize  = 40;
const int kLevelNameSize   = 40;
const int kSessionNameSize = 40;

#pragma pack(1)

struct SessionDescription
{
	GUID      guidInstance;
	GUID      guidApplication;
	uint32_t  host;
	uint16_t  port;
	uint32_t  version;
	uint16_t  gameStyle;
	uint16_t  level;
	uint8_t   nPlayers;
	uint8_t   maxPlayers;
	char      levelName[kLevelNameSize];
	char      sessionName[kSessionNameSize];
};

struct PlayerDetails
{
	NetID	ID;
	uint8_t	type;
	char	name[kPlayerNameSize];
};

struct DestroyPlayerOrGroup
{
	NetID	ID;
	uint8_t	type;
};

#pragma pack()

extern PlayerDetails thisClientPlayer;

#endif // #ifndef _NETWORKING_
