
#include "enet\enet.h"
#include "console.h"
#include "configFile.h"
#include "logString.h"

extern "C" {

#include "3dc.h"
#include "AvP_Menus.h"
#include "AvP_MP_Config.h"
#include "assert.h"
#include "networking.h"

static ENetHost		*Client = NULL; // if we're a client
static ENetHost		*Server = NULL; // if we're a server
static ENetAddress	ServerAddress;
static ENetAddress	ClientAddress;
static ENetAddress	BroadcastAddress;
static ENetPeer		*ServerPeer = NULL;
static bool net_IsInitialised = false;

extern void NewOnScreenMessage(char *messagePtr);

int glpDP = 0;
int AvPNetID = 0;
PlayerDetails thisClientPlayer;

// used to hold message data
static uint8_t packetBuffer[NET_MESSAGEBUFFERSIZE];

static int netPortNumber = 23513;
static int incomingBandwidth = 0;
static int outgoingBandwidth = 0;

NET_SESSIONDESC netSession; // main game session

extern void MinimalNetCollectMessages(void);
extern void InitAVPNetGameForHost(int species, int gamestyle, int level);
extern void InitAVPNetGameForJoin(void);
extern int DetermineAvailableCharacterTypes(BOOL ConsiderUsedCharacters);
extern char* GetCustomMultiplayerLevelName(int index, int gameType);
extern unsigned char DebouncedKeyboardInput[];

static bool Net_CreatePlayer(char *playerName, char *clanTag);
int Net_GetNextPlayerID();
void Net_FindAvPSessions();
static BOOL DpExtProcessRecvdMsg(BOOL bIsSystemMsg, LPVOID lpData, DWORD dwDataSize);
void Net_Disconnect();
void Net_ConnectToAddress();
int Net_CreateSession(const char* sessionName, int maxPlayers, int version, int level);
int Net_UpdateSessionDescForLobbiedGame(int gamestyle, int level);

/* KJL 14:58:18 03/07/98 - AvP's Guid */
// {379CCA80-8BDD-11d0-A078-004095E16EA5}
static const GUID AvPGuid = { 0x379cca80, 0x8bdd, 0x11d0, { 0xa0, 0x78, 0x0, 0x40, 0x95, 0xe1, 0x6e, 0xa5 } };

const int MESSAGEHEADERSIZE = sizeof(messageHeader);

BOOL Net_UpdateSessionList(int *SelectedItem)
{
	GUID OldSessionGuids[MAX_NO_OF_SESSIONS];
	int OldNumberOfSessions = NumberOfSessionsFound;
	BOOL changed = FALSE;

	// test, remove
//	if (NumberOfSessionsFound == 1) 
//		return FALSE;

	// take a list of the old session guids
	for (int i = 0; i < NumberOfSessionsFound; i++)
	{
		OldSessionGuids[i] = SessionData[i].Guid;
	}

	// do the session enumeration thing
// bjd - CHECK	Net_FindAvPSessions();

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

	// get user config data
	netPortNumber = Config_GetInt("[Networking]", "PortNumber", 1234);
	incomingBandwidth = Config_GetInt("[Networking]", "IncomingBandwidth", 0);
	outgoingBandwidth = Config_GetInt("[Networking]", "OutgoingBandwidth", 0);

	// register console commands
	Con_AddCommand("connect", Net_ConnectToAddress);
	Con_AddCommand("disconnect", Net_Disconnect);

	net_IsInitialised = true;

	return NET_OK;
}

int Net_Deinitialise()
{
	Con_PrintMessage("Deinitialising ENet networking");

	Net_Disconnect();

	// de-initialise ENet
	enet_deinitialize();

	net_IsInitialised = false;
	glpDP = 0;

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

			Server = enet_host_create(&ServerAddress,	// the address to bind the server host to, 
                                 32,					// allow up to 32 clients and/or outgoing connections,
                                 incomingBandwidth,		// assume any amount of incoming bandwidth,
                                 outgoingBandwidth);	// assume any amount of outgoing bandwidth;
			if (Server == NULL)
			{
				Con_PrintError("Failed to create ENet server");
				return NET_FAIL;
			}

			// create session
			char* customLevelName = GetCustomMultiplayerLevelName(level, gamestyle);
			if (customLevelName[0])
			{
				// add the level name to the beginning of the session name
				char name_buffer[100];
				sprintf(name_buffer, "%s:%s", customLevelName, sessionName);
				if ((Net_CreateSession(name_buffer, maxPlayers, AVP_MULTIPLAYER_VERSION, (gamestyle<<8)|100)) != NET_OK) 
                    return NET_FAIL;
			}
			else
			{
				if ((Net_CreateSession(sessionName, maxPlayers, AVP_MULTIPLAYER_VERSION, (gamestyle<<8)|level)) != NET_OK) 
					return NET_FAIL;
			}
		}
		else
		{
			// for lobbied games we need to fill in the level number into the existing session description
			if (Net_UpdateSessionDescForLobbiedGame(gamestyle, level) != NET_OK) 
				return NET_FAIL;
		}

		// we're the host
		AvP.Network = I_Host;

		// need to remove this and use the above..?
		glpDP = 1;

		if (!Net_CreatePlayer(playerName, /*clanTag - ADDME!*/ NULL)) 
			return NET_FAIL;
	}
	else
	{
		// fake multiplayer - need to set the id to an non zero value
		AvPNetID = 100;

		ZeroMemory(&thisClientPlayer, sizeof(PlayerDetails));
		strncpy(thisClientPlayer.name, playerName, PLAYER_NAME_SIZE-1);
		thisClientPlayer.name[PLAYER_NAME_SIZE-1] = '\0';
		thisClientPlayer.clanTag[0] = '\0';
		thisClientPlayer.playerID = AvPNetID;
		thisClientPlayer.playerType = 0; // ??
	}

	InitAVPNetGameForHost(species, gamestyle, level);
	return NET_OK;
}

void Net_Disconnect()
{
	OutputDebugString("Net_Disconnect\n");

	// need to let everyone know we're disconnecting
	DPMSG_DESTROYPLAYERORGROUP destroyPlayer;
	destroyPlayer.ID = AvPNetID;
	destroyPlayer.playerType = NET_PLAYERTYPE_PLAYER;
	destroyPlayer.type = 0; // ?

	if (AvP.Network != I_Host) // don't do this if this is the host
	{
		if (Net_SendSystemMessage(AVP_SYSTEMMESSAGE, NET_SYSTEM_MESSAGE, NET_DESTROYPLAYERORGROUP, (uint8_t*)(&destroyPlayer), sizeof(DPMSG_DESTROYPLAYERORGROUP)) != NET_OK)
		{
			Con_PrintError("Net_Disconnect - Problem sending destroy player system message!");
			return; // what do we do in this case?
		}
	}
	else
	{
		// DPSYS_HOST or DPSYS_SESSIONLOST ?
	}

	// destroy client, check if exists first?
	if (Client)
	{
		enet_host_destroy(Client);
		Client = NULL;
	}
	else if (Server)
	{
		enet_host_destroy(Server);
		Server = NULL;
	}

	return;
}

int Net_JoinGame()
{	
	assert (net_IsInitialised);

	// enumerate sessions
	Net_FindAvPSessions();
	return NumberOfSessionsFound;
}

// called from the console
void Net_ConnectToAddress()
{
	if (Con_GetNumArguments() == 0)
	{
		Con_PrintMessage("usage: connect <ip>:<port>, eg 192.168.0.1:1234");
		return;
	}

	// make sure we're not already connected
	//Net_Disconnect();

	// parse the argument list
	std::string addressString = Con_GetArgument(0);
	std::string tempString;
	static ENetAddress connectionAddress;
	size_t colonPos = 0;
	ENetEvent eEvent;
	uint8_t receiveBuffer[3072];

	colonPos = addressString.find(":");
	tempString = addressString.substr(0, colonPos);

	enet_address_set_host(&connectionAddress, tempString.c_str());
	connectionAddress.port = (uint16_t)StringToInt(addressString.substr(colonPos + 1));

	// try connect
	Client = enet_host_create(NULL,     // create a client host
                1,                      // only allow 1 outgoing connection
                incomingBandwidth,      // 57600 / 8 - 56K modem with 56 Kbps downstream bandwidth
                outgoingBandwidth);     // 14400 / 8 - 56K modem with 14 Kbps upstream bandwidth

	if (Client == NULL)
    {
		Con_PrintError("Net_ConnectToAddress - Failed to create Enet client");
		return;
    }
	
	ServerPeer = enet_host_connect(Client, &connectionAddress, 2);
	if (ServerPeer == NULL)
	{
		Con_PrintError("Net_ConnectToAddress - Failed to init connection to server host");
		return;
	}

	// Wait up to 3 seconds for the connection attempt to succeed.
	if ((enet_host_service (Client, &eEvent, 3000) > 0) && (eEvent.type == ENET_EVENT_TYPE_CONNECT))
	{	
		Con_PrintDebugMessage("Net_ConnectToAddress - we connected to server!");
	}
	else
	{
		Con_PrintError("Net_ConnectToAddress - failed to connect to server!");
		return;
	}
	
	glpDP = 1;

	// ask for session info
	Net_SendSystemMessage(AVP_REQUEST_SESSION_DATA, 0, 0, NULL, 0);

	if ((enet_host_service (Client, &eEvent, 3000) > 0) && (eEvent.type == ENET_EVENT_TYPE_RECEIVE))
	{	
		Con_PrintDebugMessage("Net_ConnectToAddress - we got something from the server!");
		memcpy(&receiveBuffer[0], static_cast<uint8_t*> (eEvent.packet->data), eEvent.packet->dataLength);
		size_t size = eEvent.packet->dataLength;
		enet_packet_destroy(eEvent.packet);

		messageHeader newHeader;
		memcpy(&newHeader, &receiveBuffer[0], sizeof(messageHeader));

		if (newHeader.messageType == AVP_SESSIONDATA)
		{
			// get the hosts ip address for later use
			enet_address_get_host_ip(&eEvent.peer->address, SessionData[NumberOfSessionsFound].hostAddress, 16);

			NET_SESSIONDESC tempSession;

			// grab the session description struct
			assert(sizeof(NET_SESSIONDESC) == (size - MESSAGEHEADERSIZE));
			memcpy(&tempSession, &receiveBuffer[MESSAGEHEADERSIZE], sizeof(NET_SESSIONDESC));

			int gamestyle = (tempSession.level >> 8) & 0xff;
			int level = tempSession.level  & 0xff;

			char sessionName[100];
			char levelName[100];

			// split the session name up into its parts
			if (level >= 100)
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

			sprintf(SessionData[NumberOfSessionsFound].Name,"%s (%d/%d)", sessionName, tempSession.currentPlayers, tempSession.maxPlayers);

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
				sprintf(SessionData[NumberOfSessionsFound].Name, "%s (V %.2f)", sessionName, version);
			}
			else
			{
				// get the level number in our list of levels (assuming we have the level)
				int local_index = GetLocalMultiplayerLevelIndex(level, levelName, gamestyle);

				if (local_index < 0)
				{
					// we don't have the level, so ignore this session
					//return;
				}
					
				SessionData[NumberOfSessionsFound].levelIndex = local_index;
			}

			if (!Net_CreatePlayer("TestName", NULL))
				return;

			InitAVPNetGameForJoin();

			netGameData.levelNumber = SessionData[NumberOfSessionsFound].levelIndex;
			netGameData.joiningGameStatus = JOINNETGAME_WAITFORSTART;
		}
	}

/*
	if (!Net_CreatePlayer("BlahMan", NULL))
		return;// NET_FAIL;

	InitAVPNetGameForJoin();

	netGameData.levelNumber = SessionData[sessionNumber].levelIndex;
	netGameData.joiningGameStatus = JOINNETGAME_WAITFORSTART;
*/
}

int Net_InitLobbiedGame()
{
	OutputDebugString("Net_InitwhoLobbiedGame\n");
	extern char MP_PlayerName[];
	return 0;
}

// call once per frame
void Net_ServiceNetwork()
{
	if (Server)
	{
		enet_host_service (Server, NULL, 0);
	}
	else if (Client)
	{
		enet_host_service (Client, NULL, 0);
	}
}

int Net_Receive(int *fromID, int *toID, int flags, uint8_t *messageData, size_t *dataSize)
{
	// this function is called from a loop. only return something other than NET_OK when we've no more messages
	bool	isInternalOnly = false;
	bool	isSystemMessage = false;
	int		messageType = 0;

	do
	{
		isInternalOnly = false;	// Default.

		ENetEvent eEvent;

		// check server events
		if (Server)
		{
			if (enet_host_check_events (Server, &eEvent) > 0)
			{
				switch (eEvent.type)
				{
					case ENET_EVENT_TYPE_CONNECT:
					{
						char ipaddr[32];
						enet_address_get_host_ip(&eEvent.peer->address, ipaddr, sizeof(ipaddr));

						std::stringstream sstream;
						sstream << "Enet got a connection from " << /*std::string(ipaddr)*/ipaddr << ":" << eEvent.peer->address.port;
						OutputDebugString(sstream.str().c_str());
						//sprintf(buf, "Enet got a connection from %s:%u\n", ipaddr, eEvent.peer->address.port);
						//OutputDebugString(buf);

						return NET_OK; // this is still fine, we might have more messages
					}
					case ENET_EVENT_TYPE_RECEIVE:
					{
						OutputDebugString("Enet received a packet!\n");

						// copy it in, header and all
						memcpy(messageData, static_cast<uint8_t*> (eEvent.packet->data), eEvent.packet->dataLength);
						*dataSize = eEvent.packet->dataLength;
						enet_packet_destroy(eEvent.packet);
						break; // break so we fall down to "switch (messageType)"
					}
					case ENET_EVENT_TYPE_DISCONNECT:
					{
						OutputDebugString("ENet got a disconnection\n");
						eEvent.peer->data = NULL;
						return NET_OK;
					}
/*
					case ENET_EVENT_TYPE_NONE:
					{
						//OutputDebugString("Enet FALSE Event\n");
						eEvent.peer->data = NULL;
						return NET_NO_MESSAGES;
					}
*/
				}
			}
			else
			{
				return NET_NO_MESSAGES;
			}
		}
		else if (Client)
		{
			if (enet_host_check_events (Client, &eEvent) > 0)
			{
				switch (eEvent.type)
				{
					case ENET_EVENT_TYPE_RECEIVE:
					{
						OutputDebugString("Enet Client received a packet!\n");

						// copy it in, header and all
						memcpy(messageData, static_cast<uint8_t*> (eEvent.packet->data), eEvent.packet->dataLength);
						*dataSize = eEvent.packet->dataLength;
						enet_packet_destroy(eEvent.packet);
						break; // break so we fall down to "switch (messageType)"
					}
/*
					case ENET_EVENT_TYPE_NONE:
					{
						//OutputDebugString("Enet FALSE Event\n");
						eEvent.peer->data = NULL;
						return NET_NO_MESSAGES;
					}
*/
				}
			}
			else
			{
				return NET_NO_MESSAGES;
			}
		}
		else // no client or server!
		{
			return NET_NO_MESSAGES;
		}

		// we broke from recieve above and got here as we have some data to process (move this to a separate funtion?
		messageHeader newHeader;
		memcpy(&newHeader, &messageData[0], sizeof(messageHeader));

		messageType = newHeader.messageType;

		*fromID = newHeader.fromID;
		*toID = newHeader.toID;

		switch (messageType)
		{
			case AVP_BROADCAST:
			{
				// double check..
				if ((NET_BROADCAST_ID == newHeader.fromID) && (NET_BROADCAST_ID == newHeader.toID))
				{
					OutputDebugString("Someone sent us a broadcast packet!\n");

					// reply to it? handle this properly..dont send something from a receive function??
					messageHeader newReplyHeader;
					newReplyHeader.messageType = AVP_SESSIONDATA;
					newReplyHeader.fromID = AvPNetID;
					newReplyHeader.toID = 0;
					memcpy(&packetBuffer[0], &newReplyHeader, sizeof(messageHeader));

					assert(sizeof(netSession) == sizeof(NET_SESSIONDESC));
					memcpy(&packetBuffer[MESSAGEHEADERSIZE], &netSession, sizeof(NET_SESSIONDESC));

					int length = MESSAGEHEADERSIZE + sizeof(NET_SESSIONDESC);

					// create a packet with session struct inside
					ENetPacket * packet = enet_packet_create(&packetBuffer[0], length, ENET_PACKET_FLAG_RELIABLE);

					// send the packet
					if ((enet_peer_send(eEvent.peer, eEvent.channelID, packet) < 0))
					{
						Con_PrintError("problem sending session packet!");
					}
					else Con_PrintMessage("sent session details to client");

					return NET_OK;
				}
			}
			case AVP_REQUEST_SESSION_DATA:
			{
				NewOnScreenMessage("SENDING SESSION DATA");

				// a client requested a copy ofthe session data
				messageHeader newReplyHeader;
				newReplyHeader.messageType = AVP_SESSIONDATA;
				newReplyHeader.fromID = AvPNetID;
				newReplyHeader.toID = 0;
				memcpy(&packetBuffer[0], &newReplyHeader, sizeof(messageHeader));

				assert(sizeof(netSession) == sizeof(NET_SESSIONDESC));
				memcpy(&packetBuffer[MESSAGEHEADERSIZE], &netSession, sizeof(NET_SESSIONDESC));

				int length = MESSAGEHEADERSIZE + sizeof(NET_SESSIONDESC);

				// create a packet with session struct inside
				ENetPacket * packet = enet_packet_create(&packetBuffer[0], length, ENET_PACKET_FLAG_RELIABLE);

				// send the packet

				if ((enet_peer_send(eEvent.peer, eEvent.channelID, packet) < 0))
				{
					Con_PrintError("problem sending session packet!");
					NewOnScreenMessage("problem sending session packet!");
				}
				else Con_PrintMessage("sent session details to client");

				return NET_OK;
			}
			case AVP_SYSTEMMESSAGE:
			{
				OutputDebugString("We received a system message!\n");
				isSystemMessage = true;
				*fromID = NET_SYSTEM_MESSAGE;
				*toID = 0; // check!
				return NET_OK;
			}
			case AVP_GETPLAYERNAME:
			{
				OutputDebugString("message check - AVP_GETPLAYERNAME\n");

				int playerFrom = *fromID;
				int idOfPlayerToGet = *toID;

				PlayerDetails newPlayerDetails;

				int id = PlayerIdInPlayerList(idOfPlayerToGet);

				newPlayerDetails.playerID = id;
				strncpy(newPlayerDetails.name, netGameData.playerData[id].name, PLAYER_NAME_SIZE);
	//TODO		strncpy(newPlayerDetails.clanTag, netGameData.playerData[id].name, PLAYER_NAME_SIZE);
				newPlayerDetails.clanTag[0] = '\0';
				newPlayerDetails.playerType = 0; // do i even need this?

				if (NET_IDNOTINPLAYERLIST == id)
				{
					OutputDebugString("player not in the list!\n");
				}

				// pass it back..
				messageHeader newReplyHeader;
				newReplyHeader.messageType = AVP_SENTPLAYERNAME;
				newReplyHeader.fromID = playerFrom;
				newReplyHeader.toID = idOfPlayerToGet;

				int length = MESSAGEHEADERSIZE + sizeof(PlayerDetails);

				// copy the struct in after the header
				memcpy(&packetBuffer[MESSAGEHEADERSIZE], reinterpret_cast<uint8_t*>(&newPlayerDetails), sizeof(PlayerDetails));

				// create a packet with player name struct inside
				ENetPacket * packet = enet_packet_create(&packetBuffer, length, ENET_PACKET_FLAG_RELIABLE);

				OutputDebugString("we got player name request, going to send it back\n");

				// send the packet
				if ((enet_peer_send(eEvent.peer, eEvent.channelID, packet) < 0))
				{
					Con_PrintError("AVP_GETPLAYERNAME - problem sending player name packet!");
				}

				return NET_OK;
			}
			case AVP_SENTPLAYERNAME:
			{
				int playerFrom = *fromID;
				int idOfPlayerWeGot = *toID;

				PlayerDetails newPlayerDetails;
				memcpy(&newPlayerDetails, &messageData[MESSAGEHEADERSIZE], sizeof(PlayerDetails));

				char buf[100];
				sprintf(buf, "sent player id: %d and player name: %s from: %d", idOfPlayerWeGot, newPlayerDetails.name, playerFrom);
				OutputDebugString(buf);

				int id = PlayerIdInPlayerList(idOfPlayerWeGot);
				strcpy(netGameData.playerData[id].name, newPlayerDetails.name);

				return NET_OK;
			}
			case AVP_GAMEDATA:
			{
				// do nothing here
				return NET_OK;
			}
			default:
			{
				char buf[100];
				sprintf(buf, "ERROR: UNKNOWN MESSAGE TYPE: %d\n", messageType); 
				OutputDebugString(buf);
				return NET_FAIL;
			}
		}
	}
	while (isInternalOnly);

	return NET_OK;
}

// used to send a message to the server only
int Net_SendSystemMessage(int messageType, int fromID, int toID, uint8_t *messageData, size_t dataSize)
{
	// create a new header and copy it into the packet buffer
	messageHeader newMessageHeader;
	newMessageHeader.messageType = messageType;
	newMessageHeader.fromID = fromID;
	newMessageHeader.toID = toID;
	memcpy(&packetBuffer[0], &newMessageHeader, sizeof(newMessageHeader));

	// put data after header
	if (dataSize && messageData)
	{
		memcpy(&packetBuffer[MESSAGEHEADERSIZE], messageData, dataSize);
	}

	size_t length = MESSAGEHEADERSIZE + dataSize;

	// create Enet packet
	ENetPacket * packet = enet_packet_create (packetBuffer, length, ENET_PACKET_FLAG_RELIABLE);

	if (packet == NULL)
	{
		Con_PrintError("Net_SendSystemMessage - couldn't create packet");
		return NET_FAIL;
	}

	// let us know if we're not connected
	if (ENET_PEER_STATE_CONNECTED != ServerPeer->state)
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

int Net_Send(int fromID, int toID, int flags, uint8_t *messageData, size_t dataSize)
{
	if (messageData == NULL) 
	{
		Con_PrintError("Net_Send - lpData was NULL");
		return NET_FAIL;
	}

	return NET_OK;

	messageHeader newMessageHeader;
/*
	if (NET_SYSTEM_MESSAGE == fromID)
	{
		Con_PrintDebugMessage("Net_Send - sending system message\n");
		newMessageHeader.messageType = AVP_SYSTEMMESSAGE;
	}
	else 
	{
		newMessageHeader.messageType = AVP_GAMEDATA;
	}
*/
	newMessageHeader.messageType = AVP_GAMEDATA;
	newMessageHeader.fromID = fromID;
	newMessageHeader.toID = toID;

	memcpy(&packetBuffer[0], &newMessageHeader, sizeof(messageHeader));

	memcpy(&packetBuffer[MESSAGEHEADERSIZE], messageData, dataSize);

	size_t length = MESSAGEHEADERSIZE + dataSize;

	// create ENet packet
	ENetPacket * packet = enet_packet_create(packetBuffer, length, ENET_PACKET_FLAG_RELIABLE);

	if (packet == NULL)
	{
		Con_PrintError("Net_Send - couldn't create packet");
		return NET_FAIL;
	}

	// send to all peers
	if (toID == NET_ID_ALLPLAYERS)
	{
		enet_host_broadcast(Client, 0, packet);
	}
	else if (toID == NET_ID_SERVERPLAYER)
	{
		//enet_peer_send(
	}


 	enet_host_flush(Client);

	return NET_OK;
}

int Net_ConnectToSession(int sessionNumber, char *playerName)
{
	OutputDebugString("Step 1: Connect To Session\n");

	if (Net_OpenSession(SessionData[sessionNumber].hostAddress) != NET_OK) 
		return NET_FAIL;

	OutputDebugString("Step 2: Connecting to host: ");
	OutputDebugString(SessionData[sessionNumber].hostAddress);

	if (!Net_CreatePlayer(playerName, NULL))
		return NET_FAIL;

	InitAVPNetGameForJoin();

	netGameData.levelNumber = SessionData[sessionNumber].levelIndex;
	netGameData.joiningGameStatus = JOINNETGAME_WAITFORDESC;
	
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

// this function should be called for the host only?
static bool Net_CreatePlayer(char *playerName, char *clanTag)
{
	// get next available player ID
	if (AvP.Network != I_Host)
	{
		AvPNetID = Net_GetNextPlayerID();
	}
	else
	{
		AvPNetID = 1; // host is always 1?
	}

	// Initialise static name structure to refer to the names:
	ZeroMemory(&thisClientPlayer, sizeof(PlayerDetails));
	strncpy(thisClientPlayer.name, playerName, PLAYER_NAME_SIZE);
//ADDME	thisClientPlayer.clanTag	= clanTag;
	thisClientPlayer.clanTag[0] = '\0';
	thisClientPlayer.playerType = NET_PLAYERTYPE_PLAYER;
	thisClientPlayer.playerID	= AvPNetID;

	// do we need to do this as host? (call AddPlayer()..)
	if (AvP.Network != I_Host)
	{
		OutputDebugString("CreatePlayer - Sending player struct\n");

		// send struct as system message
		if (Net_SendSystemMessage(AVP_SYSTEMMESSAGE, NET_SYSTEM_MESSAGE, NET_CREATEPLAYERORGROUP, reinterpret_cast<uint8_t*>(&thisClientPlayer), sizeof(PlayerDetails)) != NET_OK)
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
	OutputDebugString("Net_OpenSession()\n");
	ENetEvent eEvent;

	enet_address_set_host(&ServerAddress, hostName);
	ServerAddress.port = netPortNumber;

	// create Enet client
	Client = enet_host_create(NULL,     // create a client host
                1,                      // only allow 1 outgoing connection
                incomingBandwidth,      // 57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth
                outgoingBandwidth);     // 14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth

	if (Client == NULL)
    {
		Con_PrintError("Net_OpenSession - Failed to create Enet client");
		return NET_FAIL;
    }
	
	ServerPeer = enet_host_connect(Client, &ServerAddress, 2);
	if (ServerPeer == NULL)
	{
		Con_PrintError("Net_OpenSession - Failed to init connection to server host");
		return NET_FAIL;
	}

	/* see if we actually connected */
	/* Wait up to 3 seconds for the connection attempt to succeed. */
	if ((enet_host_service (Client, &eEvent, 3000) > 0) && (eEvent.type == ENET_EVENT_TYPE_CONNECT))
	{	
		Con_PrintDebugMessage("Net_OpenSession - we connected to server!");
	}
	else
	{
		Con_PrintError("Net_OpenSession - failed to connect to server!\n");
		return NET_FAIL;
	}
	
	glpDP = 1;

	return NET_OK;
}

int running = 0;
#include <process.h>

unsigned int __stdcall SessionSearch(void *args)
{
	ENetPeer *Peer = NULL;
	ENetEvent eEvent;
	uint8_t receiveBuffer[NET_MESSAGEBUFFERSIZE]; // can reduce this in size?
	int messageType;
	NET_SESSIONDESC tempSession;
	char sessionName[100];
	char levelName[100];
	int gamestyle;
	int level;
	char buf[100];

	running = 1;

	OutputDebugString("in thread\n");

	// set up broadcast address
	BroadcastAddress.host = ENET_HOST_BROADCAST;
	BroadcastAddress.port = netPortNumber; // can I reuse port?? :\

	// create Enet client
	Client = enet_host_create (NULL,	// create a client host
				1,						// only allow 1 outgoing connection
				incomingBandwidth,		// 57600 / 8 - 56K modem with 56 Kbps downstream bandwidth
				outgoingBandwidth);		// 14400 / 8 - 56K modem with 14 Kbps upstream bandwidth

	if (Client == NULL)
    {
		Con_PrintError("Failed to create ENet client");
		return -1;
    }

	Peer = enet_host_connect(Client, &BroadcastAddress, 2);
	if (Peer == NULL)
	{
		Con_PrintError("Failed to connect to ENet Broadcast peer");
		return -1;
	}

	// lets send a message out saying we're looking for available game sessions
	if (enet_host_service(Client, &eEvent, 1000) > 0) // this could probably be lower?
	{
		if (eEvent.type == ENET_EVENT_TYPE_CONNECT)
		{
			Con_PrintDebugMessage("Net - Connected for Broadcast");

            // create broadcast header
			messageHeader newHeader;
			newHeader.messageType = AVP_BROADCAST;
			newHeader.fromID = NET_BROADCAST_ID;
			newHeader.toID = NET_BROADCAST_ID;

			memcpy(packetBuffer, &newHeader, sizeof(messageHeader));

			// create ENet packet
			ENetPacket * packet = enet_packet_create(packetBuffer, sizeof(messageHeader), ENET_PACKET_FLAG_RELIABLE);

			enet_peer_send(Peer, 0, packet);
			enet_host_flush(Client);
		}
	}

	// need to wait here for servers to respond with session info
	while (enet_host_service (Client, &eEvent, 10000) > 0) // 10 secs
	{
		if (eEvent.type == ENET_EVENT_TYPE_RECEIVE)
		{
			memcpy(&receiveBuffer[0], static_cast<uint8_t*> (eEvent.packet->data), eEvent.packet->dataLength);

            // grab the header from the packet
            messageHeader newHeader;
            memcpy(&newHeader, &receiveBuffer[0], sizeof(messageHeader));
            messageType = newHeader.messageType;

			if (AVP_SESSIONDATA == messageType)
			{
				Con_PrintDebugMessage("Net - server sent us session data");

				// get the hosts ip address for later use
				enet_address_get_host_ip(&eEvent.peer->address, SessionData[NumberOfSessionsFound].hostAddress, 16);

				// grab the session description struct
				assert(sizeof(NET_SESSIONDESC) == (eEvent.packet->dataLength - MESSAGEHEADERSIZE));
				memcpy(&tempSession, &receiveBuffer[MESSAGEHEADERSIZE], sizeof(NET_SESSIONDESC));

				gamestyle = (tempSession.level >> 8) & 0xff;
				level = tempSession.level  & 0xff;

				// split the session name up into its parts
				if (level >= 100)
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

				sprintf(SessionData[NumberOfSessionsFound].Name,"%s (%d/%d)", sessionName, tempSession.currentPlayers, tempSession.maxPlayers);

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
 					sprintf(SessionData[NumberOfSessionsFound].Name, "%s (V %.2f)", sessionName, version);
				}
				else
				{
					// get the level number in our list of levels (assuming we have the level)
					int local_index = GetLocalMultiplayerLevelIndex(level, levelName, gamestyle);

					if (local_index < 0)
					{
						// we don't have the level, so ignore this session
						//return;
					}
						
					SessionData[NumberOfSessionsFound].levelIndex = local_index;
				}

				NumberOfSessionsFound++;
				sprintf(buf,"num sessions found: %d", NumberOfSessionsFound);
				OutputDebugString(buf);
				break;
			} // if AVP_SESSIONDATA
			else
			{
				char message[100];
				sprintf(buf, "Net - server sent us unknown data type: %d\n", messageType);
				std::string strMessage = message;
				//Con_PrintError("Net - server sent us unknown data");
				Con_PrintError(strMessage);
				break;
			}
			
			// destroy the packet
			enet_packet_destroy(eEvent.packet);
		}
	}

	// ok done

	// close connection
	enet_host_destroy(Client);

	OutputDebugString("leaving thread\n");

	_endthreadex(0);

	return 0;
}

void Net_FindAvPSessions()
{
//	OutputDebugString("Net_FindAvPSessions called\n");
	NumberOfSessionsFound = 0;

//	return;
#if 0
	static bool busy = false;
	static int timer = 0;
	static HANDLE threadHandle;

	if (!busy)
	{
		// grab the current time
		timer = timeGetTime();

		// start the thread
		OutputDebugString("creating thread\n");
		threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, SessionSearch, /*static_cast<void*>(VorbisStream)*/NULL, 0, NULL));
		busy = true;
	}

	if (DebouncedKeyboardInput[KEY_ESCAPE])
	{
		busy = false;
	}

	if (WaitForSingleObject(threadHandle, 30) == WAIT_OBJECT_0)
	{
		OutputDebugString("we know the thread is dead\n");
		// thread is finished
		busy = false;
	}
#endif
#if 1

	ENetPeer *Peer = NULL;
	ENetEvent eEvent;
	uint8_t receiveBuffer[NET_MESSAGEBUFFERSIZE]; // can reduce this in size?
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
                incomingBandwidth,		// 57600 / 8 - 56K modem with 56 Kbps downstream bandwidth
                outgoingBandwidth);		// 14400 / 8 - 56K modem with 14 Kbps upstream bandwidth

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

    // lets send a message out saying we're looking for available game sessions
	if (enet_host_service(Client, &eEvent, 1000) > 0) // this could probably be lower?
	{
		if (ENET_EVENT_TYPE_CONNECT == eEvent.type)
		{
			Con_PrintDebugMessage("Net - Connected for Broadcast");

            // create broadcast header
			messageHeader newHeader;
			newHeader.messageType = AVP_BROADCAST;
			newHeader.fromID = NET_BROADCAST_ID;
			newHeader.toID = NET_BROADCAST_ID;

			memcpy(packetBuffer, &newHeader, sizeof(messageHeader));

			// create ENet packet
			ENetPacket *packet = enet_packet_create(packetBuffer, sizeof(messageHeader), ENET_PACKET_FLAG_RELIABLE);

			enet_peer_send(Peer, 0, packet);
			enet_host_flush(Client);
		}
	}
	
	// need to wait here for servers to respond with session info
	while (enet_host_service (Client, &eEvent, 3000) > 0)
	{
		if (ENET_EVENT_TYPE_RECEIVE == eEvent.type)
		{
			memcpy(&receiveBuffer[0], static_cast<uint8_t*> (eEvent.packet->data), eEvent.packet->dataLength);

            // grab the header from the packet
            messageHeader newHeader;
            memcpy(&newHeader, &receiveBuffer[0], sizeof(messageHeader));

			if (AVP_SESSIONDATA == newHeader.messageType)
			{
				Con_PrintDebugMessage("Net - server sent us session data");

				// get the hosts ip address for later use
				enet_address_get_host_ip(&eEvent.peer->address, SessionData[NumberOfSessionsFound].hostAddress, 16);

				// grab the session description struct
				assert(sizeof(NET_SESSIONDESC) == (eEvent.packet->dataLength - MESSAGEHEADERSIZE));
				memcpy(&tempSession, &receiveBuffer[MESSAGEHEADERSIZE], sizeof(NET_SESSIONDESC));

				gamestyle = (tempSession.level >> 8) & 0xff;
				level = tempSession.level  & 0xff;

				// split the session name up into its parts
				if (level >= 100)
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

				sprintf(SessionData[NumberOfSessionsFound].Name,"%s (%d/%d)", sessionName, tempSession.currentPlayers, tempSession.maxPlayers);

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
 					sprintf(SessionData[NumberOfSessionsFound].Name, "%s (V %.2f)", sessionName, version);
				}
				else
				{
					// get the level number in our list of levels (assuming we have the level)
					int local_index = GetLocalMultiplayerLevelIndex(level, levelName, gamestyle);

					if (local_index < 0)
					{
						// we don't have the level, so ignore this session
						return;
					}
						
					SessionData[NumberOfSessionsFound].levelIndex = local_index;
				}

				NumberOfSessionsFound++;
				sprintf(buf,"num sessions found: %d", NumberOfSessionsFound);
				OutputDebugString(buf);
				break;
			} // if AVP_SESSIONDATA
			else
			{
				char message[100];
				sprintf(buf, "Net - server sent us unknown data type: %d\n", newHeader.messageType);
				std::string strMessage = message;
				//Con_PrintError("Net - server sent us unknown data");
				Con_PrintError(strMessage);
				break;
			}
		}
	}
	
	// close connection
	enet_host_destroy(Client);
#endif
}

int Net_CreateSession(const char* sessionName, int maxPlayers, int version, int level)
{
	ZeroMemory(&netSession, sizeof(NET_SESSIONDESC));
	netSession.size = sizeof(NET_SESSIONDESC);

	strcpy(netSession.sessionName, sessionName);

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

int Net_UpdateSessionDescForLobbiedGame(int gamestyle, int level)
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
	return NET_OK;
}

} // extern C