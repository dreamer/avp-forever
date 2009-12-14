#ifndef _NETWORKING_
#define _NETWORKING_

#include "module.h"
#include "stratdef.h"
#include "equipmnt.h"
#include "pldnet.h"

int Net_Initialise();
int Net_Deinitialise();
int Net_ConnectingToSession();
int DpExtSend(int lpDP2A, DPID idFrom, DPID idTo, DWORD dwFlags, unsigned char *lpData, int dwDataSize);
int DpExtRecv(int lpDP2A, int *lpidFrom, int *lpidTo, DWORD dwFlags, unsigned char *lplpData, int *lpdwDataSize);
int Net_SendSystemMessage(int messageType, int idFrom, int idTo, unsigned char *lpData, int dwDataSize);

extern int glpDP;
extern int AvPNetID;

/*
Version 0 - Original multiplayer + save patch
Version 100 - Added pistol,skeeter (and new levels)
*/
#define AVP_MULTIPLAYER_VERSION 100

static const int MESSAGEHEADERSIZE = sizeof(unsigned char) + (sizeof(int) * 2);

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
typedef struct {
    DWORD dwSize;
    DWORD dwFlags;
    GUID  guidInstance;
    GUID  guidApplication;
    DWORD dwMaxPlayers;
    DWORD dwCurrentPlayers;

	union
	{
		char lpszSessionName[40];
		char lpszSessionNameA[40];
	};

	DWORD dwUser1;
	DWORD dwUser2;
} NET_SESSIONDESC;

typedef struct playerDetails
{
	int playerId;
	int playerType;
	char playerName[40];
} playerDetails;

typedef struct 
{
	int dwSize;
	union
	{
		char *lpszShortName;
		char *lpszShortNameA;
	};
	union
	{
		char *lpszLongName;	
		char *lpszLongNameA;
	};
} DPNAME, FAR *LPDPNAME;

// easily sendable version of above
typedef struct playerName
{
	int dwSize;
	char LongNameA[40];
	char ShortNameA[40];
} playerName;

extern DPNAME AVPDPplayerName;

/*
 * DPMSG_ADDPLAYERTOGROUP
 * System message generated when a player is being added
 * to a group.
 */
typedef struct
{
    DWORD       dwType;         // Message type
    DPID        dpIdGroup;      // group ID being added to
    DPID        dpIdPlayer;     // player ID being added
} DPMSG_ADDPLAYERTOGROUP, FAR *LPDPMSG_ADDPLAYERTOGROUP;

/*
 * DPMSG_SETPLAYERORGROUPDATA
 * System message generated when remote data for a player or
 * group has changed.
 */
typedef struct
{
    DWORD       dwType;         // Message type
    DWORD       dwPlayerType;   // Is it a player or group
    DPID        dpId;           // ID of player or group
    LPVOID      lpData;         // pointer to remote data
    DWORD       dwDataSize;     // size of remote data
} DPMSG_SETPLAYERORGROUPDATA, FAR *LPDPMSG_SETPLAYERORGROUPDATA;

/*
 * DPMSG_SETPLAYERORGROUPNAME
 * System message generated when the name of a player or
 * group has changed.
 */
typedef struct
{
    DWORD       dwType;         // Message type
    DWORD       dwPlayerType;   // Is it a player or group
    DPID        dpId;           // ID of player or group
    DPNAME      dpnName;        // structure with new name info
} DPMSG_SETPLAYERORGROUPNAME, FAR *LPDPMSG_SETPLAYERORGROUPNAME;

/* The maximum length of a message that can be received by the system, in
 * bytes. DPEXT_MAX_MSG_SIZE bytes will be required to hold the message, so
 * you can't just set this to a huge number.
 */
#pragma pack()

#define DPEXT_MAX_MSG_SIZE	3072

/* Buffer used to store incoming messages. */
static unsigned char gbufDpExtRecv[ DPEXT_MAX_MSG_SIZE ];

#define DPEXT_NEXT_GRNTD_MSG_COUNT() \
	if( ++gnCurrGrntdMsgId < 1 ) gnCurrGrntdMsgId = 1

static BOOL gbDpExtDoGrntdMsgs = FALSE;
static BOOL gbDpExtDoErrChcks = FALSE;

static int gnCurrGrntdMsgId = 1;

#pragma pack(1)
typedef struct DPMSG_GENERIC
{
	int dwType;
} DPMSG_GENERIC;
typedef DPMSG_GENERIC * LPDPMSG_GENERIC;

typedef struct DPMSG_CREATEPLAYERORGROUP
{
	int dwType;
	
	DPID dpId;
	int dwPlayerType;
	
	DPNAME dpnName;
} DPMSG_CREATEPLAYERORGROUP;
typedef DPMSG_CREATEPLAYERORGROUP * LPDPMSG_CREATEPLAYERORGROUP;

typedef struct DPMSG_DESTROYPLAYERORGROUP
{
	int dwType;
	
	DPID dpId;
	int dwPlayerType;	
} DPMSG_DESTROYPLAYERORGROUP;
typedef DPMSG_DESTROYPLAYERORGROUP * LPDPMSG_DESTROYPLAYERORGROUP;
#pragma pack()

#endif // #ifndef _NETWORKING_