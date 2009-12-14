
#include "enet\enet.h"
#include "logString.h"
#include "console.h"
#include "configFile.h"

extern "C" {

#include "3dc.h"
#include "AvP_Menus.h"
#include "AvP_MP_Config.h"
#include "networking.h"
#include "assert.h"

static ENetHost		*Client;
static ENetAddress	ServerAddress;
static ENetAddress	BroadcastAddress;
static ENetPeer		*ServerPeer;
static bool net_IsInitialised = false;

int glpDP;
int AvPNetID;
DPNAME AVPDPplayerName;

// used to hold message data
static uint8_t packetBuffer[NET_MESSAGEBUFFERSIZE];

static int netPortNumber = 1234;
static int incomingBandwidth = 0;
static int outgoingBandwidth = 0;

NET_SESSIONDESC netSession; // main game session

extern void MinimalNetCollectMessages(void);
extern void InitAVPNetGameForHost(int species, int gamestyle, int level);
extern void InitAVPNetGameForJoin(void);
extern int DetermineAvailableCharacterTypes(BOOL ConsiderUsedCharacters);
extern char* GetCustomMultiplayerLevelName(int index, int gameType);
extern unsigned char DebouncedKeyboardInput[];

static bool Net_CreatePlayer(char* FormalName,char* FriendlyName);
int Net_GetNextPlayerID();
void Net_FindAvPSessions();
static BOOL DpExtProcessRecvdMsg(BOOL bIsSystemMsg, LPVOID lpData, DWORD dwDataSize);
HRESULT Net_CreateSession(LPTSTR lptszSessionName, int maxPlayers, int version, int level);
BOOL Net_UpdateSessionDescForLobbiedGame(int gamestyle, int level);
int Net_OpenSession(const char *hostName);
int Net_SendSystemMessage(int messageType, int idFrom, int idTo, uint8_t *lpData, int dwDataSize);

/* KJL 14:58:18 03/07/98 - AvP's Guid */
// {379CCA80-8BDD-11d0-A078-004095E16EA5}
static const GUID AvPGuid = { 0x379cca80, 0x8bdd, 0x11d0, { 0xa0, 0x78, 0x0, 0x40, 0x95, 0xe1, 0x6e, 0xa5 } };

struct messageHeader
{
	uint8_t		messageType;
	uint32_t	fromID;
	uint32_t	toID;
};

const int MESSAGEHEADERSIZE = sizeof(messageHeader);

BOOL Net_UpdateSessionList(int *SelectedItem)
{
	GUID OldSessionGuids[MAX_NO_OF_SESSIONS];
	int OldNumberOfSessions = NumberOfSessionsFound;
	BOOL changed = FALSE;

	/* bjd -test, remove */
	if (NumberOfSessionsFound == 1) 
		return FALSE;

	// take a list of the old session guids
	for (int i = 0; i < NumberOfSessionsFound; i++)
	{
		OldSessionGuids[i] = SessionData[i].Guid;
	}

	// do the session enumeration thing
	Net_FindAvPSessions();

	// Have the available sessions changed? first check number of sessions
	if (NumberOfSessionsFound != OldNumberOfSessions)
	{
		changed = TRUE;
	}
	else
	{
		// now check the guids of the new sessions against our previous list
		for (int i = 0; i < NumberOfSessionsFound; i++)
		{
			if (!IsEqualGUID(OldSessionGuids[i], SessionData[i].Guid))
			{
				changed = TRUE;
			}
		}
	}

	if (changed && OldNumberOfSessions > 0)
	{
		// See if our previously selected session is in the new session list
		int OldSelection = *SelectedItem;
		*SelectedItem=0;

		for (int i = 0; i < NumberOfSessionsFound; i++)
		{
			if (IsEqualGUID(OldSessionGuids[OldSelection], SessionData[i].Guid))
			{
				// found the session
				*SelectedItem = i;
				break;
			}
		}
	}

	return changed;
}

int Net_Initialise()
{
	Con_PrintMessage("Initialising ENet networking");

	// initialise ENet
	if (enet_initialize() != 0)
	{
		Con_PrintMessage("Failed to initialise ENet networking");
		return NET_FAIL;
	}

	netPortNumber = Config_GetInt("[Networking]", "PortNumber", 1234);
	incomingBandwidth = Config_GetInt("[Networking]", "IncomingBandwidth", 0);
	outgoingBandwidth = Config_GetInt("[Networking]", "OutgoingBandwidth", 0);

	net_IsInitialised = true;

	return NET_OK;
}

int Net_Deinitialise()
{
	Con_PrintMessage("Deinitialising ENet networking");

	// de-initialise ENet
	enet_deinitialize();

	net_IsInitialised = false;

	return NET_OK;
}

int Net_ConnectingToSession()
{
	OutputDebugString("Net_ConnectingToSession\n");

	// see if the player has got bored of waiting
	if (DebouncedKeyboardInput[KEY_ESCAPE])
	{
		// abort attempt to join game
		if (AvPNetID)
		{
//			IDirectPlayX_DestroyPlayer(glpDP, AvPNetID);
			AvPNetID = 0;
		}
		//DPlayClose();
		AvP.Network = I_No_Network;	
		return NET_FAIL;
	}

	MinimalNetCollectMessages();
	if (!netGameData.needGameDescription)
	{
	  	// we now have the game description , so we can go to the configuration menu
	  	return AVPMENU_MULTIPLAYER_CONFIG_JOIN;
	}
	return NET_OK;
}

int Net_ConnectingToLobbiedGame(char* playerName)
{
	OutputDebugString("Net_ConnectingToLobbiedGame\n");
	return 0;
}

int Net_HostGame(char *playerName, char *sessionName, int species, int gamestyle, int level)
{
	assert (net_IsInitialised);

	int maxPlayers = DetermineAvailableCharacterTypes(FALSE);
	if (maxPlayers < 1) maxPlayers = 1;
	if (maxPlayers > 8) maxPlayers = 8;

	if (!netGameData.skirmishMode)
	{
		if (!LobbiedGame)
		{
			ServerAddress.host = ENET_HOST_ANY;
			ServerAddress.port = netPortNumber;

			Client = enet_host_create(&ServerAddress,	// the address to bind the server host to, 
                                 32,					// allow up to 32 clients and/or outgoing connections,
                                 incomingBandwidth,		// assume any amount of incoming bandwidth,
                                 outgoingBandwidth);	// assume any amount of outgoing bandwidth;
			if (Client == NULL)
			{
				Con_PrintError("Failed to create ENet server");
				return NET_FAIL;
			}

			// create session
			{
				char* customLevelName = GetCustomMultiplayerLevelName(level, gamestyle);
				if (customLevelName[0])
				{
					// add the level name to the beginning of the session name
					char name_buffer[100];
					sprintf(name_buffer, "%s:%s", customLevelName, sessionName);
//					if ((Net_CreateSession(name_buffer,maxPlayers,AVP_MULTIPLAYER_VERSION,(gamestyle<<8)|100)) != NET_OK) return 0;
				}
				else
				{
					if ((Net_CreateSession(sessionName, maxPlayers, AVP_MULTIPLAYER_VERSION, (gamestyle<<8)|level)) != NET_OK) 
						return NET_FAIL;
				}
			}
		}
		else
		{
			// for lobbied games we need to fill in the level number into the existing session description
			if (!Net_UpdateSessionDescForLobbiedGame(gamestyle, level)) 
				return NET_FAIL;
		}

		AvP.Network = I_Host;

		if (!Net_CreatePlayer(playerName, playerName)) 
			return NET_FAIL;

		// need to replace this..
		glpDP = 1;
	}
	else
	{
		// fake multiplayer - need to set the id to an non zero value
		AvPNetID = 100;

		ZeroMemory(&AVPDPplayerName,sizeof(DPNAME));
		AVPDPplayerName.size = sizeof(DPNAME);
		AVPDPplayerName.shortName = playerName;
		AVPDPplayerName.longName = playerName;
	}

	InitAVPNetGameForHost(species, gamestyle, level);
	return NET_OK;
}

int Net_Disconnect()
{
	OutputDebugString("Deinitialising ENet\n");

	// need to let everyone know we're disconnecting
	DPMSG_DESTROYPLAYERORGROUP destroyPlayer;
	destroyPlayer.ID = AvPNetID;
	destroyPlayer.playerType = DPPLAYERTYPE_PLAYER;
	destroyPlayer.type = 0; // ?

	if (AvP.Network != I_Host) // don't do this if this is the host
	{
		if (Net_SendSystemMessage(AVP_SYSTEMMESSAGE, DPID_SYSMSG, NET_DESTROYPLAYERORGROUP, reinterpret_cast<uint8_t*>(&destroyPlayer), sizeof(DPMSG_DESTROYPLAYERORGROUP)) != NET_OK)
		{
			Con_PrintError("Net_Disconnect - Problem sending destroy player system message!");
			return NET_FAIL;
		}
	}
	else
	{
		// DPSYS_HOST or DPSYS_SESSIONLOST ?
	}

	// destroy client, check if exists first?
	enet_host_destroy(Client);

	return NET_OK;
}

void Net_EnumConnections()
{
	// just set this here for now
	netGameData.tcpip_available = 1;
}

int Net_JoinGame()
{	
	assert (net_IsInitialised);

	// enumerate sessions
	Net_FindAvPSessions();
	return NumberOfSessionsFound;
}

int Net_InitLobbiedGame()
{
	OutputDebugString("\n Net_InitLobbiedGame");
	extern char MP_PlayerName[];
	return 0;
}

BOOL DpExtInit(DWORD cGrntdBufs, DWORD cBytesPerBuf, BOOL bErrChcks)
{
	return TRUE;
}

int DpExtRecv(int lpDP2A, int *lpidFrom, int *lpidTo, DWORD dwFlags, uint8_t *lplpData, int *lpdwDataSize)
{
	BOOL	bInternalOnly;
	BOOL	bIsSysMsg;
	int		messageType = 0;

	if (Client == NULL) 
		return DPERR_NOMESSAGES;

	do
	{
		bInternalOnly = FALSE;	// Default.

		ENetEvent event;

		if (enet_host_service(Client, &event, 1) > 0)
		{
			switch (event.type)
			{
				// not interested yet
				case ENET_EVENT_TYPE_CONNECT:
					OutputDebugString("Enet got a connection!\n");
					return DPERR_NOMESSAGES;
					break;

				case ENET_EVENT_TYPE_RECEIVE:
					//OutputDebugString("Enet received a packet!\n");
					memcpy(lplpData, static_cast<uint8_t*> (event.packet->data), event.packet->dataLength);
					*lpdwDataSize = event.packet->dataLength;
					enet_packet_destroy(event.packet);
					break;
				case ENET_EVENT_TYPE_DISCONNECT:
					OutputDebugString("ENet got a disconnection\n");
					event.peer->data = NULL;
					return DPERR_NOMESSAGES;
					break;

				case ENET_EVENT_TYPE_NONE:
					//OutputDebugString("Enet FALSE Event\n");
					return DPERR_NOMESSAGES;
					break;
			}
		}

		else // no message
		{
			return DPERR_NOMESSAGES;
		}

		messageHeader newHeader;
		memcpy(&newHeader, lplpData, sizeof(messageHeader));

		messageType = newHeader.messageType;
		*lpidFrom = newHeader.fromID;
		*lpidTo = newHeader.toID;
/*
		messageType = (char)lplpData[0];
		*lpidFrom = *(int*)&lplpData[1];
		*lpidTo   = *(int*)&lplpData[5];
*/
		// check for broadcast message
		if (messageType == AVP_BROADCAST)
		{
			// double check..
			if ((newHeader.fromID == 255) && (newHeader.toID == 255))
			{
				OutputDebugString("Someone sent us a broadcast packet!\n");

				// reply to it? handle this properly..dont send something from a receive function??
				messageHeader newReplyHeader;
				newReplyHeader.messageType = AVP_SESSIONDATA;
				newReplyHeader.fromID = 0;
				newReplyHeader.toID = 0;
				memcpy(packetBuffer, &newReplyHeader, sizeof(messageHeader));
/*
				packetBuffer[0] = AVP_SESSIONDATA;
				*(int*)&packetBuffer[1] = 0;
				*(int*)&packetBuffer[5] = 0;
*/
				assert(sizeof(netSession) == sizeof(NET_SESSIONDESC));
				memcpy(&packetBuffer[MESSAGEHEADERSIZE], &netSession, sizeof(NET_SESSIONDESC));

				int length = MESSAGEHEADERSIZE + sizeof(NET_SESSIONDESC);

				// create a packet with session struct inside
				ENetPacket * packet = enet_packet_create(&packetBuffer, 
					length, 
					ENET_PACKET_FLAG_RELIABLE);

				// send the packet
				if ((enet_peer_send(event.peer, event.channelID, packet) < 0))
				{
					LogErrorString("problem sending session packet!\n");
				}

				return DPERR_NOMESSAGES;
			}
		}

		else if (messageType == AVP_SYSTEMMESSAGE)
		{
			OutputDebugString("We received a system message!\n");
			bIsSysMsg = TRUE;
		}

		else if (messageType == AVP_GETPLAYERNAME)
		{
			OutputDebugString("message check - AVP_GETPLAYERNAME\n");

			int playerFrom = *lpidFrom;
			int idOfPlayerToGet = *lpidTo;

			playerName tempName;

			int id = PlayerIdInPlayerList(idOfPlayerToGet);

			strcpy(tempName.longName, netGameData.playerData[id].name);
			strcpy(tempName.shortName, netGameData.playerData[id].name);
			tempName.size = sizeof(playerName);

			if (id == NET_IDNOTINPLAYERLIST)
			{
				OutputDebugString("player not in the list!\n");
			}

			// pass it back..
			messageHeader newReplyHeader;
			newReplyHeader.messageType = AVP_SENTPLAYERNAME;
			newReplyHeader.fromID = playerFrom;
			newReplyHeader.toID = idOfPlayerToGet;
/*
			packetBuffer[0] = AVP_SENTPLAYERNAME;
			*(int*)&packetBuffer[1] = playerFrom;
			*(int*)&packetBuffer[5] = idOfPlayerToGet; 
*/
			int length = MESSAGEHEADERSIZE + sizeof(playerName);

			// copy the struct in after the header
			memcpy(&packetBuffer[MESSAGEHEADERSIZE], reinterpret_cast<uint8_t*>(&tempName), sizeof(playerName));

			// create a packet with player name struct inside
			ENetPacket * packet = enet_packet_create(&packetBuffer, length, ENET_PACKET_FLAG_RELIABLE);

			OutputDebugString("we got player name request, going to send it back\n");

			// send the packet
			if ((enet_peer_send(event.peer, event.channelID, packet) < 0))
			{
				Con_PrintError("AVP_GETPLAYERNAME - problem sending player name packet!");
			}

			return DPERR_NOMESSAGES;
		}
		else if (messageType == AVP_SENTPLAYERNAME)
		{
			int playerFrom = *lpidFrom;
			int idOfPlayerWeGot = *lpidTo;

			playerName tempName;
			memcpy(&tempName, &lplpData[MESSAGEHEADERSIZE], sizeof(playerName));

			char buf[100];
			sprintf(buf, "sent player id: %d and player name: %s from: %d", idOfPlayerWeGot, tempName.longName, playerFrom);
			OutputDebugString(buf);

			int id = PlayerIdInPlayerList(idOfPlayerWeGot);
			strcpy(netGameData.playerData[id].name, tempName.longName);

			return DPERR_NOMESSAGES;
		}
		else if (messageType == AVP_GAMEDATA)
		{
			// do nothing here
		}
		else
		{
			char buf[100];
			sprintf(buf, "ERROR: UNKNOWN MESSAGE TYPE: %d\n", messageType); 
			//OutputDebugString("ERROR: UNKNOWN MESSAGE TYPE\n");
			OutputDebugString(buf);
			return DPERR_NOMESSAGES;
		}
	}
	while (bInternalOnly);

	return NET_OK;
}

int Net_SendSystemMessage(int messageType, int idFrom, int idTo, uint8_t *lpData, int dwDataSize)
{
	messageHeader newMessageHeader;
	newMessageHeader.messageType = messageType;
	newMessageHeader.fromID = idFrom;
	newMessageHeader.toID = idTo;
	memcpy(packetBuffer, &newMessageHeader, sizeof(newMessageHeader));
/*
	packetBuffer[0] = messageType;
	*(int*)&packetBuffer[1] = idFrom;
	*(int*)&packetBuffer[5] = idTo;
*/
	// put data after header
	memcpy(&packetBuffer[MESSAGEHEADERSIZE], lpData, dwDataSize);

	int length = MESSAGEHEADERSIZE + dwDataSize;

	// create Enet packet
	ENetPacket * packet = enet_packet_create (packetBuffer, length, ENET_PACKET_FLAG_RELIABLE);

	if (packet == NULL)
	{
		Con_PrintError("Net_SendSystemMessage - couldn't create packet");
		return NET_FAIL;
	}

	// let us know if we're not connected
	if (ServerPeer->state != ENET_PEER_STATE_CONNECTED)
	{
		Con_PrintError("Net_SendSystemMessage - not connected to server peer!");
	}

	// send the packet
	if (enet_peer_send(ServerPeer, 0, packet) != 0)
	{
		Con_PrintError("Net_SendSystemMessage - can't send peer packet");
	}

	enet_host_flush(Client);

	return NET_OK;
}

int DpExtSend(int lpDP2A, int idFrom, int idTo, DWORD dwFlags, uint8_t *lpData, int dwDataSize)
{
	if (lpData == NULL) 
	{
		Con_PrintError("DpExtSend - lpData was NULL");
		return NET_FAIL;
	}

	messageHeader newMessageHeader;
		
	if (idFrom == DPID_SYSMSG)
	{
		Com_PrintDebugMessage("DpExtSend - sending system message\n");
		//packetBuffer[0] = AVP_SYSTEMMESSAGE;
		newMessageHeader.messageType = AVP_SYSTEMMESSAGE;
	}
	else 
	{
		//packetBuffer[0] = AVP_GAMEDATA;
		newMessageHeader.messageType = AVP_GAMEDATA;
	}

//	*(int*)&packetBuffer[1] = idFrom;
//	*(int*)&packetBuffer[5] = idTo;
	newMessageHeader.fromID = idFrom;
	newMessageHeader.toID = idTo;
	memcpy(packetBuffer, &newMessageHeader, sizeof(messageHeader));

	memcpy(&packetBuffer[MESSAGEHEADERSIZE], lpData, dwDataSize);

	int length = MESSAGEHEADERSIZE + dwDataSize;

	// create ENet packet
	ENetPacket * packet = enet_packet_create (packetBuffer, length, ENET_PACKET_FLAG_RELIABLE);

	if (packet == NULL)
	{
		Con_PrintError("DpExtSend - couldn't create packet");
		return NET_FAIL;
	}

	// send to all peers
	enet_host_broadcast(Client, 0, packet);

 	enet_host_flush(Client);

	return NET_OK;
}

int Net_ConnectToSession(int sessionNumber, char *playerName)
{
	OutputDebugString("Step 1: Connect To Session\n");

	if (Net_OpenSession(SessionData[sessionNumber].hostAddress) != NET_OK) 
		return NET_FAIL;

	OutputDebugString("Step 2: connecting to host: ");
	OutputDebugString(SessionData[sessionNumber].hostAddress);
	
	if (!Net_CreatePlayer(playerName,playerName))
		return NET_FAIL;

	InitAVPNetGameForJoin();

	netGameData.levelNumber = SessionData[sessionNumber].levelIndex;
	netGameData.joiningGameStatus = JOINNETGAME_WAITFORSTART;
	
	return NET_OK;
}

#if 0
// IDirectPlayX_GetPlayerName(glpDP,messagePtr->players[i].playerId,data,&size);
int IDirectPlayX_GetPlayerName(int glpDP, DPID id, unsigned char *data, int *size)
{	
#if 1
	ENetEvent event;
	unsigned char receiveBuffer[NET_MESSAGEBUFFERSIZE];

	OutputDebugString("GetPlayerName - Should NOT be called by host!\n");

	playerDetails tempPlayerDetails;
	tempPlayerDetails.playerId = id;

	playerName tempName;
	
	/* need to request this from the server - just send back to sending eNet peer */
	Net_SendSystemMessage(AVP_GETPLAYERNAME, id, DPSYS_GETPLAYERNAME, (unsigned char*)&tempPlayerDetails, sizeof(playerDetails));

	// just wait here for reply? I think this is ok as we only get called from ProcessNetMsg_GameDescription 
	// which states it'll only get called for non hosts who are in game startup mode. Recheck this if players get
	// momentarily paused when new players join :)
	
	if(enet_host_service(Client, &event, 1000) > 0 && event.type == ENET_EVENT_TYPE_RECEIVE)
	{
		memcpy(receiveBuffer, (unsigned char*)event.packet->data, event.packet->dataLength);

		if(receiveBuffer[0] != AVP_GETPLAYERNAME) 
		{	
			OutputDebugString("wrong sodding message?!\n");
			return 0;
		}

		int type = *(int*)&receiveBuffer[5];

		if(type == DPSYS_GETPLAYERNAME)
		{
			OutputDebugString("we got the player name info\n");
			memcpy(&tempName, &receiveBuffer[MESSAGEHEADERSIZE], sizeof(playerName));
		}
	}
	else
	{
		return NET_FAIL;
	}

	/* just return size if pointer was null, as per original function */
	if(data == NULL)
	{
		*size = strlen(tempName.LongNameA);//dpPlayerName.dwSize;
	}
	else
	{
		strcpy((char*)data, tempName.LongNameA);
	}
#endif
#if 0
	/* just return size if pointer was null, as per original function */
	if(data == NULL)
	{
		*size = 9;
		return NET_OK;
	}

	strcpy((char*)data, "DeadMeat");
	*size = 9;
#endif
	return NET_OK;
}
#endif

static bool Net_CreatePlayer(char* FormalName, char* FriendlyName)
{
	// Initialise static name structure to refer to the names:
	ZeroMemory(&AVPDPplayerName,sizeof(DPNAME));
	AVPDPplayerName.size = sizeof(DPNAME);
	AVPDPplayerName.shortName = FriendlyName;
	AVPDPplayerName.longName = FormalName;

	// get next available player ID
	AvPNetID = Net_GetNextPlayerID(); 

	// create struct to send player data
	playerDetails newPlayer;
	newPlayer.playerID = AvPNetID;
	newPlayer.playerType = DPPLAYERTYPE_PLAYER;
	strcpy(&newPlayer.playerName[0], AVPDPplayerName.shortName);

	OutputDebugString("CreatePlayer - Sending player struct\n");

	// dont think we do this if we're the server :)
	if (AvP.Network != I_Host)
	{
		// send struct as system message
		if (Net_SendSystemMessage(AVP_SYSTEMMESSAGE, DPID_SYSMSG, NET_CREATEPLAYERORGROUP, reinterpret_cast<uint8_t*>(&newPlayer), sizeof(playerDetails)) != NET_OK)
		{
			Con_PrintError("CreatePlayer - Problem sending create player system message!\n");
			return false;
		}
		else
		{
			Con_PrintMessage("CreatePlayer - sent create player system message!\n");
		}
	}
	return true;
}

/*	
	just grab the value from timeGetTime as the player id
	it's probably fairly safe to assume two players will never 
	get the same values for this... 
*/
static int Net_GetNextPlayerID()
{
	int val = timeGetTime();
//	char buf[100];
//	sprintf(buf, "giving player id: %d\n", val);
//	OutputDebugString(buf);
	return val;
}

int Net_OpenSession(const char *hostName)
{
	ENetEvent event;

	enet_address_set_host(&ServerAddress, hostName);
	ServerAddress.port = netPortNumber;

	// create Enet client
	Client = enet_host_create(NULL /* create a client host */,
                1 /* only allow 1 outgoing connection */,
                incomingBandwidth,//57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
                outgoingBandwidth);//14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);

	if (Client == NULL)
    {
		LogErrorString("Net_OpenSession - Failed to create Enet client\n");
		return NET_FAIL;
    }
	
	ServerPeer = enet_host_connect(Client, &ServerAddress, 2);
	if (ServerPeer == NULL)
	{
		LogErrorString("Net_OpenSession - Failed to init connection to server host\n");
		return NET_FAIL;
	}

	/* see if we actually connected */
	/* Wait up to 3 seconds for the connection attempt to succeed. */
	if ((enet_host_service (Client, &event, 3000) > 0) && (event.type == ENET_EVENT_TYPE_CONNECT))
	{	
		OutputDebugString("Net_OpenSession - we connected to server!\n");
	}
	else
	{
		LogErrorString("Net_OpenSession - failed to connect to server!\n");
		return NET_FAIL;
	}
	
	glpDP = 1;

	return NET_OK;
}

void Net_FindAvPSessions()
{
	OutputDebugString("Net_FindAvPSessions called\n");
	NumberOfSessionsFound = 0;

	ENetPeer *Peer;
	ENetEvent event;
	uint8_t receiveBuffer[NET_MESSAGEBUFFERSIZE];
	int messageType;
	NET_SESSIONDESC tempSession;
	char sessionName[100] = "";
	char levelName[100] = "";
	int gamestyle;
	int level;
	char buf[100];

	// set up broadcast address
	BroadcastAddress.host = ENET_HOST_BROADCAST;
	BroadcastAddress.port = netPortNumber; // can I reuse port?? :\

	// create Enet client
	Client = enet_host_create (NULL,	// create a client host
                1,						// only allow 1 outgoing connection
                incomingBandwidth,		//57600 / 8 - 56K modem with 56 Kbps downstream bandwidth
                outgoingBandwidth);		//14400 / 8 - 56K modem with 14 Kbps upstream bandwidth

    if (Client == NULL)
    {
		Con_PrintError("Failed to create ENet client");
		return;
    }

	Peer = enet_host_connect(Client, &BroadcastAddress, 2);
	if (Peer == NULL)
	{
		Con_PrintError("Failed to connect to ENet Broadcast peer");
		return;
	}

	if (enet_host_service(Client, &event, 1000) > 0)
	{
		if (event.type == ENET_EVENT_TYPE_CONNECT)
		{
			Com_PrintDebugMessage("Connected for Broadcast");

			messageHeader newHeader;
			newHeader.messageType = AVP_BROADCAST;
			newHeader.fromID = 255;
			newHeader.toID = 255;

			memcpy(packetBuffer, &newHeader, sizeof(messageHeader));
/*
			packetBuffer[0] = AVP_BROADCAST;
			*(int*)&packetBuffer[1] = 255;
			*(int*)&packetBuffer[5] = 255;

			// create ENet packet
			ENetPacket * packet = enet_packet_create(packetBuffer, 
					MESSAGEHEADERSIZE, // size of packet 
					ENET_PACKET_FLAG_RELIABLE);
*/
			// create ENet packet
			ENetPacket * packet = enet_packet_create(packetBuffer, sizeof(messageHeader), ENET_PACKET_FLAG_RELIABLE);

			enet_peer_send(Peer, 0, packet);
			enet_host_flush(Client);
		}
	}
	
	// need to wait here for servers to respond with session info

	// each server SHOULD only reply once?
	while (enet_host_service (Client, &event, 2000) > 0)
	{
		if (event.type == ENET_EVENT_TYPE_RECEIVE)
		{
//			OutputDebugString("received session info?\n");

			memcpy(&receiveBuffer[0], static_cast<uint8_t*> (event.packet->data), event.packet->dataLength);
			messageType = static_cast<uint8_t> (receiveBuffer[0]);

			if (messageType == AVP_SESSIONDATA)
			{
				Com_PrintDebugMessage("server sent us session data");

				// get the hosts ip address for later use
				enet_address_get_host_ip(&event.peer->address, SessionData[NumberOfSessionsFound].hostAddress, 16);

				// grab the session description struct
				assert(sizeof(NET_SESSIONDESC) == (event.packet->dataLength - MESSAGEHEADERSIZE));
				memcpy((NET_SESSIONDESC*)&tempSession, &receiveBuffer[MESSAGEHEADERSIZE], (event.packet->dataLength - MESSAGEHEADERSIZE));

				gamestyle = (tempSession.level >> 8) & 0xff;
				level = tempSession.level  & 0xff;

				// split the session name up into its parts
				if (level>=100)
				{
					char* colon_pos;
					// custom level name may be at the start
					strcpy(levelName, tempSession.sessionName);

					colon_pos = strchr(levelName,':');
					if (colon_pos)
					{
						*colon_pos = 0;
						strcpy(sessionName,colon_pos+1);
					}
					else
					{
						strcpy(sessionName, tempSession.sessionName);
						levelName[0] = 0;
					}
				}
				else
				{
					strcpy(sessionName, tempSession.sessionName);
				}

				sprintf(SessionData[NumberOfSessionsFound].Name,"%s (%d/%d)",sessionName,tempSession.currentPlayers,tempSession.maxPlayers);

				SessionData[NumberOfSessionsFound].Guid	= tempSession.guidInstance;

				if (tempSession.currentPlayers < tempSession.maxPlayers)
					SessionData[NumberOfSessionsFound].AllowedToJoin = TRUE;
				else
					SessionData[NumberOfSessionsFound].AllowedToJoin = FALSE;

				// multiplayer version number (possibly)
				if (tempSession.version != AVP_MULTIPLAYER_VERSION)
				{
					float version = 1.0f + tempSession.version / 100.0f;
					SessionData[NumberOfSessionsFound].AllowedToJoin = FALSE;
 					sprintf(SessionData[NumberOfSessionsFound].Name,"%s (V %.2f)", sessionName, version);
				}
				else
				{
					// get the level number in our list of levels (assuming we have the level)
					int local_index = GetLocalMultiplayerLevelIndex(level, levelName, gamestyle);

					if (local_index < 0)
					{
						// we don't have the level , so ignore this session
						return;
					}
						
					SessionData[NumberOfSessionsFound].levelIndex = local_index;
				}

				NumberOfSessionsFound++;
				sprintf(buf,"num sessions found: %d", NumberOfSessionsFound);
				OutputDebugString(buf);
				break;
			} // if AVP_SESSIONDATA
		}
	}
	
	// close connection
	enet_host_destroy(Client);
}

HRESULT Net_CreateSession(LPTSTR lptszSessionName, int maxPlayers, int version, int level)
{
	ZeroMemory(&netSession, sizeof(NET_SESSIONDESC));
	netSession.size = sizeof(NET_SESSIONDESC);

	strcpy(netSession.sessionName, lptszSessionName);

	netSession.maxPlayers = maxPlayers;

	netSession.version = version;
	netSession.level = level;

	// should this be done here?
	netSession.currentPlayers = 1;

#ifdef WIN32
	CoCreateGuid(&netSession.guidInstance);
#endif

	// set the application guid
	netSession.guidApplication = AvPGuid;

	return NET_OK;
}

BOOL Net_UpdateSessionDescForLobbiedGame(int gamestyle,int level)
{
#if 0
	NET_SESSIONDESC sessionDesc;
	HRESULT hr;
	if(!DirectPlay_GetSessionDesc(&sessionDesc)) return 0;

	{
		char* customLevelName = GetCustomMultiplayerLevelName(level,gamestyle);
		if(customLevelName[0])
		{
			//store the gamestyle and a too big level number in dwUser2
			sessionDesc.dwUser2 = (gamestyle<<8)|100;
		}
		else
		{
			//store the gamestyle and level number in dwUser2
			sessionDesc.dwUser2 = (gamestyle<<8)|level;
		}
		
		//store the custom level name in the session name as is.
		//since we never see the session name for lobbied games anyway
		sessionDesc.lpszSessionNameA = customLevelName;
		
		//make sure that dwUser2 is nonzero , so that it can be checked for by the 
		//clients
		sessionDesc.dwUser2|=0x80000000;
		
		sessionDesc.dwUser1 = AVP_MULTIPLAYER_VERSION;
		
		hr = IDirectPlayX_SetSessionDesc(glpDP,&sessionDesc,0);
		if(hr!=NET_OK)
		{
			return FALSE;
		}

	}
#endif
	return TRUE;
}

} // extern C