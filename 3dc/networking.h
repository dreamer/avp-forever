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
enum
{
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

// error return types
#define NET_OK                          0
#define NET_FAIL                        1

#define NET_BROADCAST_ID                255

#define PLAYER_NAME_SIZE                40
#define PLAYER_CLANTAG_SIZE             5
#define SESSION_NAME_SIZE               40

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
	char		sessionName[SESSION_NAME_SIZE];
};

struct PlayerDetails
{
	uint32_t	playerID;
	uint8_t		playerType;
	char		name[PLAYER_NAME_SIZE];
	char		clanTag[PLAYER_CLANTAG_SIZE];
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
