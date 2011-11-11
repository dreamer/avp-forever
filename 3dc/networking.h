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
#include "stratdef.h"
#include "equipmnt.h"
#include "pldnet.h"

int Net_Initialise();
int Net_Deinitialise();
int Net_ConnectingToSession();
int Net_Send(int fromID, int toID, int flags, uint8_t *messageData, size_t dataSize);
int Net_Receive(int *fromID, int *toID, int flags, uint8_t *messageData, size_t *dataSize);
int Net_SendSystemMessage(int messageType, int fromID, int toID, uint8_t *messageData, size_t dataSize);
int Net_InitLobbiedGame();
void Net_ServiceNetwork();
int Net_OpenSession(const char *hostName);
int Net_JoinGame();
int Net_ConnectToSession(int sessionNumber, char *playerName);
int Net_HostGame(char *playerName, char *sessionName, int species, int gamestyle, int level);

extern uint32_t AvPNetID;

struct MessageHeader
{
	uint32_t messageType;
	uint32_t fromID;
	uint32_t toID;
};

/*
Version 0 - Original multiplayer + save patch
Version 100 - Added pistol,skeeter (and new levels)
*/
const int kMultiplayerVersion = 101; // bjd - my version, not directplay compatible

struct MessageHeader; // forward declare the structure
extern const uint32_t kMessageHeaderSize;

// system messages
enum eMessageType
{
	nRequestSessionDetails,
	nSendConnectRequest
};

enum
{
	NET_OK,
	NET_FAIL,
	NET_NO_MESSAGES,
	NET_CREATEPLAYERORGROUP,
	NET_DESTROYPLAYERORGROUP,
	NET_ADDPLAYERTOGROUP,
	NET_DELETEPLAYERFROMGROUP,
	NET_SESSIONLOST,
	NET_HOST,
	NET_SETPLAYERORGROUPDATA,
	NET_SETPLAYERORGROUPNAME,
	NET_SETSESSIONDESC,
	NET_ADDGROUPTOGROUP,
	NET_DELETEGROUPFROMGROUP,
	NET_SECUREMESSAGE,
	NET_STARTSESSION,
	NET_CHAT,
	NET_SETGROUPOWNER,
	NET_SENDCOMPLETE,
	NET_PLAYERTYPE_GROUP,
	NET_PLAYERTYPE_PLAYER,
	NET_RECEIVE_ALL,
	NET_SYSTEM_MESSAGE,
	NET_ID_ALLPLAYERS,
	NET_ID_SERVERPLAYER,
	NET_ERR_BUSY,
	NET_ERR_CONNECTIONLOST,
	NET_ERR_INVALIDPARAMS,
	NET_ERR_INVALIDPLAYER,
	NET_ERR_NOTLOGGEDIN,
	NET_ERR_SENDTOOBIG,
	NET_ERR_BUFFERTOOSMALL
};

const int kBroadcastID = 255;

const int kPlayerNameSize  = 40;
const int kSessionNameSize = 40;

// enum for message types
enum
{
	AVP_BROADCAST,
	AVP_REQUEST_SESSION_DATA,
	AVP_SYSTEMMESSAGE,
	AVP_SESSIONDATA,
	AVP_GAMEDATA = 666,
	AVP_PING,
	AVP_GETPLAYERNAME,
	AVP_SENTPLAYERNAME
};

#pragma pack(1)

struct SessionDescription
{
	uint32_t	size;
	GUID		guidInstance;
	GUID		guidApplication;
	uint8_t		maxPlayers;
	uint8_t		currentPlayers;
	uint8_t		version;
	uint32_t	level;
	char		sessionName[kSessionNameSize];
};

struct PlayerDetails
{
	uint32_t	playerID;
	uint8_t		playerType;
	char		name[kPlayerNameSize];
};

struct DPMSG_DESTROYPLAYERORGROUP
{
	uint32_t	type;
	uint32_t	ID;
	uint32_t	playerType;
};

#pragma pack()

extern PlayerDetails thisClientPlayer;

#endif // #ifndef _NETWORKING_
