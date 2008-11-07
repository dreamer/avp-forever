extern "C" {

#include "3dc.h"
#include "networking.h"
#include "AvP_Menus.h"
#include "AvP_MP_Config.h"

#include "enet\enet.h"

#ifdef _DEBUG
	#pragma comment(lib, "enet_debug.lib")
#else
	#pragma comment(lib, "enet.lib")
#endif

#ifdef WIN32
	#pragma comment(lib, "ws2_32.lib")
#endif
#ifdef _XBOX
	//#pragma comment(lib, "xnetd.lib")
#endif

ENetHost * Client;
ENetAddress ServerAddress;
ENetAddress BroadcastAddress;

ENetPeer *ServerPeer;

/* used to hold message data */
static unsigned char packetBuffer[NET_MESSAGEBUFFERSIZE]; 

int AvPDefaultPort = 1234;

DPSESSIONDESC2 dpSession; // main game session

extern void MinimalNetCollectMessages(void);
extern void InitAVPNetGameForHost(int species, int gamestyle, int level);
extern void InitAVPNetGameForJoin(void);
extern int DetermineAvailableCharacterTypes(BOOL ConsiderUsedCharacters);
extern char* GetCustomMultiplayerLevelName(int index,int gameType);

static BOOL DirectPlay_CreatePlayer(char* FormalName,char* FriendlyName);
int GetNextPlayerID();
void FindAvPSessions();
static BOOL DpExtProcessRecvdMsg(BOOL bIsSystemMsg, LPVOID lpData, DWORD dwDataSize);
HRESULT DPlayCreateSession(LPTSTR lptszSessionName,int maxPlayers,int dwUser1,int dwUser2);
BOOL DirectPlay_UpdateSessionDescForLobbiedGame(int gamestyle,int level);
int DPlayOpenSession(char *hostName);
int sendSystemMessage(int messageType, int idFrom, int idTo, unsigned char *lpData, int dwDataSize);
void findPlayerName(int playerId, char *playerName, int size);


BOOL DirectPlay_UpdateSessionList(int *SelectedItem)
{
	int i;
	GUID OldSessionGuids[MAX_NO_OF_SESSIONS];
	int OldNumberOfSessions=NumberOfSessionsFound;
	BOOL changed=FALSE;

	//take a list of the old session guids
	for(i=0;i<NumberOfSessionsFound;i++)
	{
		OldSessionGuids[i]=SessionData[i].Guid;
	}

	//do the session enumeration thing
	FindAvPSessions();

	//Have the available sessions changed?
	//first check number of sessions
	if(NumberOfSessionsFound!=OldNumberOfSessions)
	{
		changed = TRUE;
	}
	else
	{
		//now check the guids of the new sessions against our previous list
		for(i=0;i<NumberOfSessionsFound;i++)
		{
			if(!IsEqualGUID(OldSessionGuids[i],SessionData[i].Guid))
			{
				changed = TRUE;
			}
		}
	}

	if(changed && OldNumberOfSessions>0)
	{
		//See if our previously selected session is in the new session list
		int OldSelection = *SelectedItem;
		*SelectedItem=0;

		for(i=0;i<NumberOfSessionsFound;i++)
		{
			if(IsEqualGUID(OldSessionGuids[OldSelection],SessionData[i].Guid))
			{
				//found the session
				*SelectedItem = i;
				break;
			}
		}
	}

	return changed;
}

int DirectPlay_ConnectingToSession()
{
	extern unsigned char DebouncedKeyboardInput[];
	//see if the player has got bored of waiting
	if(DebouncedKeyboardInput[KEY_ESCAPE])
	{
		//abort attempt to join game
		if(AVPDPNetID)
		{
//			IDirectPlayX_DestroyPlayer(glpDP, AVPDPNetID);
			AVPDPNetID = 0;
		}
		//DPlayClose();
		AvP.Network = I_No_Network;	
		return 0;
	}

	MinimalNetCollectMessages();
	if(!netGameData.needGameDescription)
	{
	  	//we now have the game description , so we can go to the configuration menu
	  	return AVPMENU_MULTIPLAYER_CONFIG_JOIN;
	}
	return 1;
}

int DirectPlay_ConnectingToLobbiedGame(char* playerName)
{
	OutputDebugString("\n DirectPlay_ConnectingToLobbiedGame");
	return 0;
}

int DirectPlay_HostGame(char *playerName, char *sessionName, int species, int gamestyle, int level)
{
	/* has to return 1 on success so can't use DP defines */
	int maxPlayers=DetermineAvailableCharacterTypes(FALSE);
	if(maxPlayers<1) maxPlayers=1;
	if(maxPlayers>8) maxPlayers=8;

	if(!netGameData.skirmishMode)
	{
		if(!LobbiedGame)
		{
			/* initialise eNet */
			if (enet_initialize () != 0)
			{
				//std::cout << "eNet couldn't init" << std::endl;
				OutputDebugString("Failed to init Enet\n");
				return 0;
			}
			else OutputDebugString("Initialised Enet\n");

			ServerAddress.host = ENET_HOST_ANY;
			ServerAddress.port = AvPDefaultPort;

			Client = enet_host_create(&ServerAddress /* the address to bind the server host to */, 
                                 32      /* allow up to 32 clients and/or outgoing connections */,
                                  0      /* assume any amount of incoming bandwidth */,
                                  0      /* assume any amount of outgoing bandwidth */);
			if (Client == NULL)
			{
				OutputDebugString("Failed to create Enet Server\n");
				return 0;
			}
			else OutputDebugString("Created Enet server\n");

			/* create session */
			{
				char* customLevelName = GetCustomMultiplayerLevelName(level,gamestyle);
				if(customLevelName[0])
				{
					//add the level name to the beginning of the session name
					char name_buffer[100];
					sprintf(name_buffer,"%s:%s",customLevelName,sessionName);
//					if ((DPlayCreateSession(name_buffer,maxPlayers,AVP_MULTIPLAYER_VERSION,(gamestyle<<8)|100)) != DP_OK) return 0;
				}
				else
				{
 //					static TCHAR sessionName[] = "AvP test session";
					if ((DPlayCreateSession(sessionName,maxPlayers,AVP_MULTIPLAYER_VERSION,(gamestyle<<8)|level)) != DP_OK) return 0;
				}
			}
			/* Try to create a DP player */
		}
		else
		{
			//for lobbied games we need to fill in the level number into the existing session description
			if(!DirectPlay_UpdateSessionDescForLobbiedGame(gamestyle,level)) return 0;
		}

		AvP.Network = I_Host;

		if(!DirectPlay_CreatePlayer(playerName,playerName)) return 0;

		/* dplay initialised.. */
		glpDP = 1;
	}
	else
	{
		//fake multiplayer
		//need to set the id to an non zero value
		AVPDPNetID = 100;

		ZeroMemory(&AVPDPplayerName,sizeof(DPNAME));
		AVPDPplayerName.dwSize = sizeof(DPNAME);
		AVPDPplayerName.lpszShortNameA	= playerName;
		AVPDPplayerName.lpszLongNameA = playerName;
	}

	InitAVPNetGameForHost(species,gamestyle,level);
	return 1;
}

int DirectPlay_Disconnect()
{
	OutputDebugString("Deinitialising Enet\n");

	/* need to let everyone know we're disconnecting */
	DPMSG_DESTROYPLAYERORGROUP destroyPlayer;
	destroyPlayer.dpId = AVPDPNetID;
	destroyPlayer.dwPlayerType = DPPLAYERTYPE_PLAYER;
	destroyPlayer.dwType = 0; // ?

	if(AvP.Network != I_Host) // don't do this if this is the host
	{
		int result = sendSystemMessage(AVP_SYSTEMMESSAGE, DPID_SYSMSG, DPSYS_DESTROYPLAYERORGROUP, (unsigned char*)&destroyPlayer, sizeof(DPMSG_DESTROYPLAYERORGROUP));
		if(result != DP_OK)
		{
			OutputDebugString("DirectPlay_Disconnect - Problem sending destroy player system message!\n");
			return 0;
		}
	}
	else
	{
		// DPSYS_HOST or DPSYS_SESSIONLOST ?
	}

	/* destroy client, check if exists first? */
	enet_host_destroy(Client);

	/* de-init Enet */
	enet_deinitialize();
	return 1;
}

void DirectPlay_EnumConnections()
{
	/* just set this here for now */
	netGameData.tcpip_available = 1;
}

int DirectPlay_JoinGame()
{	
//	OutputDebugString("DirectPlay_JoinGame\n");

	/* initialise eNet */
	if (enet_initialize () != 0)
	{
		OutputDebugString("Failed to init Enet\n");
		return 0;
	}
//	else OutputDebugString("Initialised Enet\n");

	/* enum sessions */
	FindAvPSessions();
	return NumberOfSessionsFound;
}

int DirectPlay_InitLobbiedGame()
{
	OutputDebugString("\n DirectPlay_InitLobbiedGame");
	extern char MP_PlayerName[];
	return 0;
}

BOOL DpExtInit(DWORD cGrntdBufs, DWORD cBytesPerBuf, BOOL bErrChcks)
{
	return TRUE;
}

int DpExtRecv(int lpDP2A, int *lpidFrom, int *lpidTo, DWORD dwFlags, unsigned char *lplpData, int *lpdwDataSize)
{
	BOOL	bInternalOnly;
	BOOL	bIsSysMsg;
	int		messageType = 0;

	if(Client == NULL) return DPERR_NOMESSAGES;

	do
	{
		bInternalOnly = FALSE;	/* Default. */

		ENetEvent event;

		if(enet_host_service(Client, &event, 1) > 0)
		{
			switch (event.type)
			{
/* not interested yet */
				case ENET_EVENT_TYPE_CONNECT:
					//OutputDebugString("Enet got a connection!\n");
					return DPERR_NOMESSAGES;
					break;

				case ENET_EVENT_TYPE_RECEIVE:
					//OutputDebugString("Enet received a packet!\n");
					memcpy(lplpData, (unsigned char*)event.packet->data, event.packet->dataLength);
					*lpdwDataSize = event.packet->dataLength;
					enet_packet_destroy(event.packet);
					break;
/* not interested for now */
				case ENET_EVENT_TYPE_DISCONNECT:
					OutputDebugString("Enet got a disconnection\n");
					event.peer->data = NULL;
					return DPERR_NOMESSAGES;
					break;

				case ENET_EVENT_TYPE_NONE:
					//OutputDebugString("Enet No Event\n");
					return DPERR_NOMESSAGES;
					break;
			}
		}
#if 1
		else /* no message */
		{
			return DPERR_NOMESSAGES;
		}
#endif
		messageType = (int)lplpData[0];
		*lpidFrom = *(int*)&lplpData[1];
		*lpidTo   = *(int*)&lplpData[5];

		// check for broadcast message
		if(messageType == AVP_BROADCAST)
		{
			// double check..
			if((*lpidFrom == 255) && (*lpidTo == 255))
			{
				OutputDebugString("\n Someone sent us a broadcast packet!");

				// reply to it? handle this properly..dont send something from a receieve function??
				packetBuffer[0] = AVP_SESSIONDATA;
				*(int*)&packetBuffer[1] = 0;
				*(int*)&packetBuffer[5] = 0;

				memcpy(&packetBuffer[MESSAGEHEADERSIZE], (unsigned char*)&dpSession, sizeof(dpSession));

				int length = MESSAGEHEADERSIZE + sizeof(dpSession);

				// create a packet with session struct inside
				ENetPacket * packet = enet_packet_create (packetBuffer, 
					length, 
					ENET_PACKET_FLAG_RELIABLE);

				// send the packet
				if((enet_peer_send(event.peer, event.channelID, packet) < 0))
				{
					OutputDebugString("problem sending session packet!\n");
				}

				return DPERR_NOMESSAGES;
			}
		}

		else if(messageType == AVP_SYSTEMMESSAGE)
		{
			OutputDebugString("We received a system message!\n");
			bIsSysMsg = TRUE;
		}

		else if(messageType == AVP_GETPLAYERNAME)
		{
			OutputDebugString("message check - AVP_GETPLAYERNAME\n");

			int playerFrom = *lpidFrom;
			int idOfPlayerToGet = *lpidTo;

			playerName tempName;

			int id = PlayerIdInPlayerList(idOfPlayerToGet);

			strcpy(tempName.LongNameA, netGameData.playerData[id].name);
			strcpy(tempName.ShortNameA, netGameData.playerData[id].name);
			tempName.dwSize = 1; // dont need this

			if(id == NET_IDNOTINPLAYERLIST)
			{
				OutputDebugString("player not in the list!\n");
			}

			/* pass it back.. */
			packetBuffer[0] = AVP_SENTPLAYERNAME;
			*(int*)&packetBuffer[1] = playerFrom;
			*(int*)&packetBuffer[5] = idOfPlayerToGet; 

			int length = sizeof(playerName) + MESSAGEHEADERSIZE;

			/* copy the struct in after the header */
			memcpy(&packetBuffer[MESSAGEHEADERSIZE], (unsigned char*)&tempName, length);

			// create a packet with player name struct inside
			ENetPacket * packet = enet_packet_create(packetBuffer, 
				length, 
				ENET_PACKET_FLAG_RELIABLE);

			OutputDebugString("we got player name request, going to send it back\n");

			// send the packet
			if((enet_peer_send(event.peer, event.channelID, packet) < 0))
			{
				OutputDebugString("AVP_GETPLAYERNAME - problem sending player name packet!\n");
			}

			return DPERR_NOMESSAGES;
		}
		else if(messageType == AVP_SENTPLAYERNAME)
		{
			int playerFrom = *lpidFrom;
			int idOfPlayerWeGot = *lpidTo;

			playerName tempName;
			memcpy(&tempName, &lplpData[MESSAGEHEADERSIZE], sizeof(playerName));//msgSize - MESSAGEHEADERSIZE);

			char buf[100];
			sprintf(buf, "sent player id: %d and player name: %s from: %d", idOfPlayerWeGot, tempName.LongNameA, playerFrom);
			OutputDebugString(buf);

			int id = PlayerIdInPlayerList(idOfPlayerWeGot);
			strcpy(netGameData.playerData[id].name, tempName.LongNameA);

			return DPERR_NOMESSAGES;
		}
/*
		else
		{
			return DPERR_NOMESSAGES;
		}
*/
	}
	while( bInternalOnly );

	return DP_OK;
}

int sendSystemMessage(int messageType, int idFrom, int idTo, unsigned char *lpData, int dwDataSize)
{
	packetBuffer[0] = messageType;
	*(int*)&packetBuffer[1] = idFrom;
	*(int*)&packetBuffer[5] = idTo;

	/* put data after header */
	memcpy(&packetBuffer[MESSAGEHEADERSIZE], lpData, dwDataSize);

	int length = MESSAGEHEADERSIZE + dwDataSize;

	/* create Enet packet */
	ENetPacket * packet = enet_packet_create (packetBuffer, 
					length, 
					ENET_PACKET_FLAG_RELIABLE);

	if(packet == NULL)
	{
		OutputDebugString("sendSystemMessage - couldn't create packet\n");
		return DP_FAIL;
	}

	/* let us know if we're not connected */
	if(ServerPeer->state != ENET_PEER_STATE_CONNECTED)
	{
		OutputDebugString("sendSystemMessage - not connected to server peer!\n");
	}

	/* send the packet */
	if(enet_peer_send(ServerPeer, 0, packet) != 0)
	{
		OutputDebugString("sendSystemMessage - can't send peer packet\n");
	}

	enet_host_flush(Client);

	return DP_OK;
}

int DpExtSend(int lpDP2A, DPID idFrom, DPID idTo, DWORD dwFlags, unsigned char *lpData, int dwDataSize)
{
	if(lpData == NULL) 
	{
		OutputDebugString("DpExtSend - No data\n");
		return DP_FAIL;
	}
		
	if(idFrom == DPID_SYSMSG)
	{
		OutputDebugString("DpExtSend - sending system message\n");
		packetBuffer[0] = AVP_SYSTEMMESSAGE;
	}
	else 
	{
		packetBuffer[0] = AVP_GAMEDATA;
	}
	*(int*)&packetBuffer[1] = idFrom;
	*(int*)&packetBuffer[5] = idTo;

	memcpy(&packetBuffer[MESSAGEHEADERSIZE], lpData, dwDataSize);

	int length = MESSAGEHEADERSIZE + dwDataSize;
	/* create Enet packet */
	ENetPacket * packet = enet_packet_create (packetBuffer, 
					length, 
					ENET_PACKET_FLAG_RELIABLE);

	if(packet == NULL)
	{
		OutputDebugString("DpExtSend - couldn't create packet\n");
		return DP_FAIL;
	}

	/* send to all peers */
	enet_host_broadcast(Client, 0, packet);

 	enet_host_flush(Client);

	return DP_OK;
}

int DirectPlay_ConnectToSession(int sessionNumber, char *playerName)
{
	OutputDebugString("Connect To Session\n");
	if(DPlayOpenSession(SessionData[sessionNumber].hostAddress) != 0) return 0;
	
	if(!DirectPlay_CreatePlayer(playerName,playerName)) return 0;

//	OutputDebugString("Creating player struct\n");

	InitAVPNetGameForJoin();

	netGameData.levelNumber = SessionData[sessionNumber].levelIndex;

	netGameData.joiningGameStatus = JOINNETGAME_WAITFORSTART;
	
	return 1;
}

int LaunchMplayer()
{
	return 0;
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
	sendSystemMessage(AVP_GETPLAYERNAME, id, DPSYS_GETPLAYERNAME, (unsigned char*)&tempPlayerDetails, sizeof(playerDetails));

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
		return DP_FAIL;
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
		return DP_OK;
	}

	strcpy((char*)data, "DeadMeat");
	*size = 9;
#endif
	return DP_OK;
}
#endif

static BOOL DirectPlay_CreatePlayer(char* FormalName,char* FriendlyName)
{
	// Initialise static DP name structure to refer to the names:
	ZeroMemory(&AVPDPplayerName,sizeof(DPNAME));
	AVPDPplayerName.dwSize = sizeof(DPNAME);
	AVPDPplayerName.lpszShortNameA	= FriendlyName;
	AVPDPplayerName.lpszLongNameA = FormalName;

	/* get next available player ID */
	AVPDPNetID = GetNextPlayerID(); 

	/* create struct to send player data */
	playerDetails newPlayer;
	newPlayer.playerId = AVPDPNetID;
	newPlayer.playerType = DPPLAYERTYPE_PLAYER;
	strcpy(&newPlayer.playerName[0], AVPDPplayerName.lpszShortNameA);

	OutputDebugString("CreatePlayer - Sending player struct\n");

	/* dont think we do this if we're the server :) */
	if(AvP.Network != I_Host)
	{
		/* send struct as system message */
		int result = sendSystemMessage(AVP_SYSTEMMESSAGE, DPID_SYSMSG, DPSYS_CREATEPLAYERORGROUP, (unsigned char*)&newPlayer, sizeof(playerDetails));
		if(result != DP_OK)
		{
			OutputDebugString("CreatePlayer - Problem sending create player system message!\n");
			return 0;
		}
		else
		{
			OutputDebugString("CreatePlayer - sent create player system message!\n");
		}
	}
	return 1;
}

/* just grab the value from timeGetTime as the player id
** it's probably fairly safe to assume two players will never 
** get the same values for this... */
int GetNextPlayerID()
{
	int val = timeGetTime();
//	char buf[100];
//	sprintf(buf, "giving player id: %d\n", val);
//	OutputDebugString(buf);
	return val;
}

int DPlayOpenSession(char *hostName)
{
	ENetEvent event;

	enet_address_set_host(&ServerAddress, hostName);
	ServerAddress.port = AvPDefaultPort;

	// create Enet client
	Client = enet_host_create(NULL /* create a client host */,
                1 /* only allow 1 outgoing connection */,
                0,//57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
                0);//14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);

	if(Client == NULL)
    {
		OutputDebugString("DPlayOpenSession - Failed to create Enet client\n");
		return DP_FAIL;
    }
	
	ServerPeer = enet_host_connect(Client, &ServerAddress, 2);
	if(ServerPeer == NULL)
	{
		OutputDebugString("DPlayOpenSession - Failed to init connection to server host\n");
		return DP_FAIL;
	}

	/* see if we actually connected */
	/* Wait up to 3 seconds for the connection attempt to succeed. */
	if(enet_host_service (Client, &event, 3000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
	{	
		OutputDebugString("DPlayOpenSession - we connected to server!\n");
	}

	else
	{
		OutputDebugString("DPlayOpenSession - failed to connect to server!\n");
		return DP_FAIL;
	}
	
	glpDP = 1;

	return DP_OK;
}

void FindAvPSessions()
{
	NumberOfSessionsFound = 0;

	ENetPeer *Peer;
	ENetEvent event;
	unsigned char receiveBuffer[NET_MESSAGEBUFFERSIZE];
	int messageType;
	DPSESSIONDESC2 tempSession;
	char sessionName[100]="";
	char levelName[100]="";
	int gamestyle;
	int level;

	// set up broadcast address
	BroadcastAddress.host = ENET_HOST_BROADCAST;
	BroadcastAddress.port = AvPDefaultPort; /* can I reuse port?? :\ */

	// create Enet client
	Client = enet_host_create (NULL /* create a client host */,
                1 /* only allow 1 outgoing connection */,
                0,//57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
                0);//14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);

    if(Client == NULL)
    {
		OutputDebugString("Failed to create Enet client\n");
		return;
    }
	else OutputDebugString("Created Enet client\n");

	Peer = enet_host_connect(Client, &BroadcastAddress, 2);
	if (Peer == NULL)
	{
		OutputDebugString("Failed to connect to Enet Broadcast peer\n");
		return;
	}
	else OutputDebugString("Connected to Enet broadcast peer\n");

	if (enet_host_service(Client, &event, 3000) > 0)
	{
		if (event.type == ENET_EVENT_TYPE_CONNECT)
		{
			OutputDebugString("Connected for broadcast!!\n");

			packetBuffer[0] = AVP_BROADCAST;
			*(int*)&packetBuffer[1] = 255;
			*(int*)&packetBuffer[5] = 255;

			// create Enet packet
			ENetPacket * packet = enet_packet_create(packetBuffer, 
					MESSAGEHEADERSIZE, // size of packet 
					ENET_PACKET_FLAG_RELIABLE);

			enet_peer_send(Peer, 0, packet);
			enet_host_flush(Client);
		}
	}

	char buf[100];
	
	// need to wait here for servers to respond with session info

	/* each server SHOULD only reply once? */
	while (enet_host_service (Client, &event, 2000) > 0)
	{
		if(event.type == ENET_EVENT_TYPE_RECEIVE)
		{
			OutputDebugString("received session info?\n");
			memcpy(receiveBuffer, (unsigned char*)event.packet->data, event.packet->dataLength);
			messageType = (int)receiveBuffer[0];

			if(messageType == AVP_SESSIONDATA)
			{
				OutputDebugString("server sent us session data\n");
				// get the hosts ip address for later use
				enet_address_get_host_ip(&event.peer->address, SessionData[NumberOfSessionsFound].hostAddress, 16);

				// else
				memcpy((DPSESSIONDESC2*)&tempSession, &receiveBuffer[MESSAGEHEADERSIZE], (event.packet->dataLength - MESSAGEHEADERSIZE));

				gamestyle = (tempSession.dwUser2 >> 8) & 0xff;
				level = tempSession.dwUser2  & 0xff;

				//split the session name up into its parts
				if(level>=100)
				{
					char* colon_pos;
					//custom level name may be at the start
					strcpy(levelName,tempSession.lpszSessionNameA);

					colon_pos = strchr(levelName,':');
					if(colon_pos)
					{
						*colon_pos = 0;
						strcpy(sessionName,colon_pos+1);
					}
					else
					{
						strcpy(sessionName,tempSession.lpszSessionNameA);
						levelName[0] = 0;
					}
				}
				else
				{
					strcpy(sessionName,tempSession.lpszSessionNameA);
				}

				sprintf(SessionData[NumberOfSessionsFound].Name,"%s (%d/%d)",sessionName,tempSession.dwCurrentPlayers,tempSession.dwMaxPlayers);

				SessionData[NumberOfSessionsFound].Guid	= tempSession.guidInstance;

				if(tempSession.dwCurrentPlayers < tempSession.dwMaxPlayers)
					SessionData[NumberOfSessionsFound].AllowedToJoin =TRUE;
				else
					SessionData[NumberOfSessionsFound].AllowedToJoin =FALSE;

				//multiplayer version number (possibly)
				if(tempSession.dwUser1 != AVP_MULTIPLAYER_VERSION)
				{
					float version = 1.0f + tempSession.dwUser1/100.0f;
					SessionData[NumberOfSessionsFound].AllowedToJoin =FALSE;
 					sprintf(SessionData[NumberOfSessionsFound].Name,"%s (V %.2f)",sessionName,version);
				}
				else
				{
					//get the level number in our list of levels (assuming we have the level)
					int local_index = GetLocalMultiplayerLevelIndex(level,levelName,gamestyle);

					if(local_index<0)
					{
						//we don't have the level , so ignore this session
						return;
					}
						
					SessionData[NumberOfSessionsFound].levelIndex = local_index;
				}

				NumberOfSessionsFound++;
				break;
				sprintf(buf,"num sessions found: %d", NumberOfSessionsFound);
				OutputDebugString(buf);
			} // if AVP_SESSIONDATA
		}
	}
	
	// close connection
	enet_host_destroy(Client);
}

HRESULT DPlayCreateSession(LPTSTR lptszSessionName,int maxPlayers,int dwUser1,int dwUser2)
{
	ZeroMemory(&dpSession, sizeof(dpSession));
	dpSession.dwSize = sizeof(dpSession);

	#ifdef UNICODE
	strcpy(dpDesc.lpszSessionName, lptszSessionName);
#else
	strcpy(dpSession.lpszSessionNameA, lptszSessionName);
#endif
	dpSession.dwMaxPlayers = maxPlayers;

	dpSession.dwUser1 = dwUser1;
	dpSession.dwUser2 = dwUser2;

	/* should this be done here? */
	dpSession.dwCurrentPlayers = 1;

/*
	// set the application guid
	if (glpGuid)
		dpDesc.guidApplication = *glpGuid;
*/

#if 0
	HRESULT hr = E_FAIL;
	DPSESSIONDESC2 dpDesc;

	if (!glpDP)
		return DPERR_NOINTERFACE;

	ZeroMemory(&dpDesc, sizeof(dpDesc));
	dpDesc.dwSize = sizeof(dpDesc);
	dpDesc.dwFlags = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE;
	if (gbUseProtocol)
		dpDesc.dwFlags |= DPSESSION_DIRECTPLAYPROTOCOL;

#ifdef UNICODE
	dpDesc.lpszSessionName = lptszSessionName;
#else
	dpDesc.lpszSessionNameA = lptszSessionName;
#endif
	dpDesc.dwMaxPlayers=maxPlayers;

	dpDesc.dwUser1 = dwUser1;
	dpDesc.dwUser2 = dwUser2;

	// set the application guid
	if (glpGuid)
		dpDesc.guidApplication = *glpGuid;

	hr = IDirectPlayX_Open(glpDP, &dpDesc, DPOPEN_CREATE);

	// Check for Async message support
//	if (SUCCEEDED(hr))
//		CheckCaps();
#endif
	/* return 0 on success */
	return DP_OK;
}

BOOL DirectPlay_UpdateSessionDescForLobbiedGame(int gamestyle,int level)
{
#if 0
	DPSESSIONDESC2 sessionDesc;
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
		if(hr!=DP_OK)
		{
			return FALSE;
		}

	}
#endif
	return TRUE;
}

} // extern C