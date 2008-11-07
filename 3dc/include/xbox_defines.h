#ifndef _XBOX_DEFINES_H_
#define _XBOX_DEFINES_H_

#ifdef __cplusplus
extern "C" { 
#endif

typedef int	DPID;

#if 0
// from dplay.h - required for pldnet.c
#define DPSYS_CREATEPLAYERORGROUP   0x0003
#define DPSYS_DESTROYPLAYERORGROUP  0x0005
#define DPSYS_ADDPLAYERTOGROUP      0x0007 
#define DPSYS_DELETEPLAYERFROMGROUP 0x0021
#define DPSYS_SESSIONLOST           0x0031
#define DPSYS_HOST                  0x0101
#define DPSYS_SETPLAYERORGROUPDATA  0x0102
#define DPSYS_SETPLAYERORGROUPNAME  0x0103
#define DPSYS_SETSESSIONDESC        0x0104
#define DPSYS_ADDGROUPTOGROUP      	0x0105  
#define DPSYS_DELETEGROUPFROMGROUP 	0x0106
#define DPSYS_SECUREMESSAGE         0x0107
#define DPSYS_STARTSESSION          0x0108
#define DPSYS_CHAT                  0x0109
#define DPSYS_SETGROUPOWNER         0x010A
#define DPSYS_SENDCOMPLETE          0x010d
#define DPPLAYERTYPE_GROUP          0x00000000
#define DPPLAYERTYPE_PLAYER         0x00000001

//errors
#define DP_OK						0
#define DPERR_BUSY					1
#define DPERR_CONNECTIONLOST		2
#define DPERR_INVALIDPARAMS			3
#define DPERR_INVALIDPLAYER			4
#define DPERR_NOTLOGGEDIN			5
#define DPERR_SENDTOOBIG			6
#define DPERR_BUFFERTOOSMALL		7

#define DPID_ALLPLAYERS				0
#define DPID_SYSMSG					0

#define DPRECEIVE_ALL               0x00000001

typedef struct DPNAME
{
	int dwSize;
	
	char *lpszShortNameA;
	char *lpszLongNameA;
} DPNAME, FAR *LPDPNAME;

/*
 * DPMSG_GENERIC
 * Generic message structure used to identify the message type.
 */
typedef struct
{
    DWORD       dwType;         // Message type
} DPMSG_GENERIC, FAR *LPDPMSG_GENERIC;

typedef struct
{
    DWORD       dwType;         // Message type
    DWORD       dwPlayerType;   // Is it a player or group
    DPID        dpId;           // ID of the player or group
    DWORD       dwCurrentPlayers;   // current # players & groups in session
    LPVOID      lpData;         // pointer to remote data
    DWORD       dwDataSize;     // size of remote data
    DPNAME      dpnName;        // structure with name info
	// the following fields are only available when using
	// the IDirectPlay3 interface or greater
    DPID	    dpIdParent;     // id of parent group
	DWORD		dwFlags;		// player or group flags
} DPMSG_CREATEPLAYERORGROUP, FAR *LPDPMSG_CREATEPLAYERORGROUP;

/*
 * DPMSG_DESTROYPLAYERORGROUP
 * System message generated when a player or group is being
 * destroyed in the session with information about it.
 */
typedef struct
{
    DWORD       dwType;         // Message type
    DWORD       dwPlayerType;   // Is it a player or group
    DPID        dpId;           // player ID being deleted
    LPVOID      lpLocalData;    // copy of players local data
    DWORD       dwLocalDataSize; // sizeof local data
    LPVOID      lpRemoteData;   // copy of players remote data
    DWORD       dwRemoteDataSize; // sizeof remote data
	// the following fields are only available when using
	// the IDirectPlay3 interface or greater
    DPNAME      dpnName;        // structure with name info
    DPID	    dpIdParent;     // id of parent group	
	DWORD		dwFlags;		// player or group flags
} DPMSG_DESTROYPLAYERORGROUP, FAR *LPDPMSG_DESTROYPLAYERORGROUP;

struct DpExtHeader
{
	DWORD dwChecksum;	/* Error checking information. 	 */
	DWORD dwMsgStamp;	/* Contains guaranteed msg info. */
};

#define DPEXT_HEADER_SIZE	( sizeof( struct DpExtHeader) )

typedef int LPDIRECTPLAY4;
typedef void* LPDPI;
#endif

#ifdef __cplusplus
};
#endif

#endif // #define _XBOX_DEFINES_H_