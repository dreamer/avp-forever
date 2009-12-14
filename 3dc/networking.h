#ifndef _NETWORKING_
#define _NETWORKING_

#include "module.h"
#include "stratdef.h"
#include "equipmnt.h"
#include "pldnet.h"

int Net_Initialise();
int Net_Deinitialise();
int Net_ConnectingToSession();
int DpExtSend(int lpDP2A, int idFrom, int idTo, DWORD dwFlags, uint8_t *lpData, int dwDataSize);
int DpExtRecv(int lpDP2A, int *lpidFrom, int *lpidTo, DWORD dwFlags, uint8_t *lplpData, int *lpdwDataSize);
int Net_SendSystemMessage(int messageType, int idFrom, int idTo, uint8_t *lpData, int dwDataSize);

extern int glpDP;
extern int AvPNetID;

/*
Version 0 - Original multiplayer + save patch
Version 100 - Added pistol,skeeter (and new levels)
*/
#define AVP_MULTIPLAYER_VERSION 101 // bjd - my version, not directplay compatible

extern const int MESSAGEHEADERSIZE;

// system messages
#define NET_CREATEPLAYERORGROUP			1
#define NET_DESTROYPLAYERORGROUP		2
#define NET_ADDPLAYERTOGROUP			3
#define NET_DELETEPLAYERFROMGROUP		4
#define NET_SESSIONLOST					5
#define NET_HOST						6
#define NET_SETPLAYERORGROUPDATA		7
#define NET_SETPLAYERORGROUPNAME		8
#define NET_SETSESSIONDESC				9
#define NET_ADDGROUPTOGROUP				10
#define NET_DELETEGROUPFROMGROUP		11
#define NET_SECUREMESSAGE				12
#define NET_STARTSESSION				13
#define NET_CHAT						14
#define NET_SETGROUPOWNER				15
#define NET_SENDCOMPLETE				16
#define DPPLAYERTYPE_GROUP				17
#define DPPLAYERTYPE_PLAYER				18

// other stuff
#define DPRECEIVE_ALL					19
#define DPERR_BUSY						20
#define DPERR_CONNECTIONLOST			21
#define DPERR_INVALIDPARAMS				22
#define DPERR_INVALIDPLAYER				23
#define DPERR_NOTLOGGEDIN				24
#define DPERR_SENDTOOBIG				25
#define DPERR_BUFFERTOOSMALL			26
#define DPID_SYSMSG						27
#define DPID_ALLPLAYERS					28
#define DPERR_NOMESSAGES				29

#define DPSEND_GUARANTEED				30
#define DPEXT_NOT_GUARANTEED			31

// error return types
#define NET_OK							0
#define NET_FAIL						1

// enum for message types
enum
{
	AVP_BROADCAST,
	AVP_SYSTEMMESSAGE,
	AVP_SESSIONDATA,
	AVP_GAMEDATA,
	AVP_PING,
	AVP_GETPLAYERNAME,
	AVP_SENTPLAYERNAME
};

#pragma pack(1)

typedef struct 
{
    uint32_t	size;
    GUID		guidInstance;
    GUID		guidApplication;
    uint8_t		maxPlayers;
    uint8_t		currentPlayers;
	uint8_t		version;
	uint32_t	level;

	char		sessionName[40];
/*
	union
	{
		char lpszSessionName[40];
		char lpszSessionNameA[40];
	};
*/
} NET_SESSIONDESC;

typedef struct playerDetails
{
	uint32_t	playerID;
	uint8_t		playerType; // redundant?
	char		playerName[40];
} playerDetails;

typedef struct 
{
	uint32_t	size;
	char		*shortName;
	char		*longName;

} DPNAME;

// easily sendable version of above
typedef struct playerName
{
	uint32_t	size;
	char		shortName[40];
	char		longName[40];
} playerName;

extern DPNAME AVPDPplayerName;

/*
 * DPMSG_ADDPLAYERTOGROUP
 * System message generated when a player is being added
 * to a group.
 */
/*
typedef struct
{
    DWORD       dwType;         // Message type
    DPID        dpIdGroup;      // group ID being added to
    DPID        dpIdPlayer;     // player ID being added
} DPMSG_ADDPLAYERTOGROUP;
*/
/*
 * DPMSG_SETPLAYERORGROUPDATA
 * System message generated when remote data for a player or
 * group has changed.
 */
/*
typedef struct
{
    DWORD       dwType;         // Message type
    DWORD       dwPlayerType;   // Is it a player or group
    DPID        dpId;           // ID of player or group
    LPVOID      lpData;         // pointer to remote data
    DWORD       dwDataSize;     // size of remote data
} DPMSG_SETPLAYERORGROUPDATA;
*/
/*
 * DPMSG_SETPLAYERORGROUPNAME
 * System message generated when the name of a player or
 * group has changed.
 */
/*
typedef struct
{
    DWORD       dwType;         // Message type
    DWORD       dwPlayerType;   // Is it a player or group
    DPID        dpId;           // ID of player or group
    DPNAME      dpnName;        // structure with new name info
} DPMSG_SETPLAYERORGROUPNAME;
*/
/* The maximum length of a message that can be received by the system, in
 * bytes. DPEXT_MAX_MSG_SIZE bytes will be required to hold the message, so
 * you can't just set this to a huge number.
 */

//#define DPEXT_MAX_MSG_SIZE	3072

// Buffer used to store incoming messages.
//static uint8_t gbufDpExtRecv[ DPEXT_MAX_MSG_SIZE ];

//#define DPEXT_NEXT_GRNTD_MSG_COUNT() \
//	if( ++gnCurrGrntdMsgId < 1 ) gnCurrGrntdMsgId = 1

//static BOOL gbDpExtDoGrntdMsgs = FALSE;
//static BOOL gbDpExtDoErrChcks = FALSE;

//static int gnCurrGrntdMsgId = 1;

/*
typedef struct DPMSG_GENERIC
{
	int dwType;
} DPMSG_GENERIC;

typedef struct DPMSG_CREATEPLAYERORGROUP
{
	int dwType;
	
	DPID dpId;
	int dwPlayerType;

	DPNAME dpnName;
} DPMSG_CREATEPLAYERORGROUP;
*/
typedef struct DPMSG_DESTROYPLAYERORGROUP
{
	uint32_t	type;
	uint32_t	ID;
	uint32_t	playerType;	
} DPMSG_DESTROYPLAYERORGROUP;


#pragma pack()

#endif // #ifndef _NETWORKING_