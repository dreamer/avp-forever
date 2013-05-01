// Copyright (C) 2011 Barry Duncan. All Rights Reserved.
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

#include "3dc.h"
#include "enet\enet.h"
#include "console.h"
#include "configFile.h"
#include "logString.h"
#include "menus.h"
#include "mp_config.h"
#include "networking.h"
#include <assert.h>
#include <queue>
#include "MemoryStream.h"

struct Message
{
	eMessageType type;
};

std::queue<Message> messageQueue;
/*
bool Net_QueueMessage()
{
	messageQueue.push();

	return true;
}

void Net_ProcessQueue()
{
	if (!messageQueue.size())
		return; // no messages to process

	for (size_t i = 0; i < messageQueue.size(); ++i)
	{
		// get first-in message
		Message currentMessage = messageQueue.front();

		switch (currentMessage.type)
		{

		}
	}
}
*/


bool running = false;

static ENetHost		*host = NULL;
static bool isHost = false;

static ENetAddress	ServerAddress;
static ENetAddress	ClientAddress;
static ENetAddress	BroadcastAddress;
static ENetPeer		*ServerPeer = NULL;
static bool net_IsInitialised = false;

extern void NewOnScreenMessage(char *messagePtr);

uint32_t AvPNetID = 0;
PlayerDetails thisClientPlayer;

// used to hold message data
static uint8_t packetBuffer[NET_MESSAGEBUFFERSIZE];

static uint32_t netPortNumber = 23513;
static uint32_t incomingBandwidth = 0;
static uint32_t outgoingBandwidth = 0;

SessionDescription netSession; // main game session

extern void MinimalNetCollectMessages(void);
extern void InitAVPNetGameForHost(int species, int gamestyle, int level);
extern void InitAVPNetGameForJoin(void);
extern int DetermineAvailableCharacterTypes(bool ConsiderUsedCharacters);
extern char* GetCustomMultiplayerLevelName(int index, int gameType);
extern unsigned char DebouncedKeyboardInput[];

static bool Net_CreatePlayer(char *playerName);
uint32_t Net_GetNextPlayerID();
void Net_FindAvPSessions();
void Net_ConnectToAddress();

static NetResult Net_CreateSession(const char* sessionName, int maxPlayers, int version, uint16_t gameStyle, uint16_t level);
static NetResult Net_UpdateSessionDescForLobbiedGame(uint16_t gameStyle, uint16_t level);

/* KJL 14:58:18 03/07/98 - AvP's Guid */
// {379CCA80-8BDD-11d0-A078-004095E16EA5}
static const GUID AvPGuid = { 0x379cca80, 0x8bdd, 0x11d0, { 0xa0, 0x78, 0x0, 0x40, 0x95, 0xe1, 0x6e, 0xa5 } };

const uint32_t kMessageHeaderSize = sizeof(MessageHeader);

#if 0
bool Net_SessionPoll()
{
	static bool isPolling = false;
	static ENetHost *searchHost = 0;

	ENetEvent eEvent;

	if (!isPolling) 
	{
		// set up broadcast address
		BroadcastAddress.host = ENET_HOST_BROADCAST;
		BroadcastAddress.port = netPortNumber; // can I reuse port?? :\

		// create Enet client
		searchHost = enet_host_create(NULL,    // create a client host
					1,                         // only allow 1 outgoing connection
					0,                         // channel limit
					incomingBandwidth,         // 57600 / 8 - 56K modem with 56 Kbps downstream bandwidth
					outgoingBandwidth);        // 14400 / 8 - 56K modem with 14 Kbps upstream bandwidth

		if (NULL == searchHost) {
			Con_PrintError("Net_SessionPoll() - Failed to create ENet client");
			return false;
		}

		ENetPeer *peer = enet_host_connect(searchHost, &BroadcastAddress, 2, 0);
		if (NULL == peer) {
			Con_PrintError("Net_SessionPoll() - Failed to connect to ENet Broadcast peer");
			return false;
		}

		if (enet_host_service(searchHost, &eEvent, 500) > 0) // this could probably be lower?
		{
			if (ENET_EVENT_TYPE_CONNECT == eEvent.type)
			{
				Con_PrintDebugMessage("Net_SessionPoll() - Connected for Broadcast");
	
	            // create broadcast header
				MessageHeader newHeader;
				newHeader.messageType = AVP_REQUEST_SERVER_INFO;
				newHeader.fromID      = kBroadcastID;
				newHeader.toID        = kBroadcastID;

				// should also pass game version number, guid etc?
	
				memcpy(packetBuffer, &newHeader, sizeof(newHeader));
	
				// create ENet packet
				ENetPacket *packet = enet_packet_create(packetBuffer, sizeof(newHeader), ENET_PACKET_FLAG_RELIABLE);
	
				enet_peer_send(peer, 0, packet);
				enet_host_flush(searchHost);

				isPolling = true;
			}
		}

		if (!isPolling) {
			return false;
		}
	}

	// perform a poll to check for session data
	if (isPolling) 
	{
		if (enet_host_check_events(searchHost, &eEvent) > 0) 
		{
			if (ENET_EVENT_TYPE_RECEIVE == eEvent.type) {
				if (sizeof(SessionDescription) == (eEvent.packet->dataLength - kMessageHeaderSize)) {
					// Handle session data
				}
			}
		}
	}
}
#endif

bool Net_CheckSessionDetails(const ENetEvent &eEvent)
{
	assert(sizeof(SessionDescription) == (eEvent.packet->dataLength - kMessageHeaderSize));

	// get header
	MessageHeader newHeader;
	memcpy(&newHeader, static_cast<uint8_t*> (eEvent.packet->data), sizeof(MessageHeader));

	if (AVP_SESSION_DATA != newHeader.messageType) {
		return false;
	}

	Con_PrintDebugMessage("Net_CheckSessionDetails() - server sent us session data");

	// get the session data from the packet
	SessionDescription session;
	memcpy(&session, static_cast<uint8_t*>(eEvent.packet->data + sizeof(MessageHeader)), sizeof(SessionDescription));

#if 0
	// check if we already have this session in our session list
	for (int i = 0; i < NumberOfSessionsFound; i++) {

		// already exists so update it
		if (SessionData[i].Guid == session.guidInstance) {

		}
		else {

		}
	}
#endif

	return true;
}

bool Net_UpdateSessionList(int *SelectedItem)
{
//	return false;

	char buf[100];
	sprintf(buf, "Net_UpdateSessionList at %d\n", timeGetTime());
	OutputDebugString(buf);

	GUID OldSessionGuids[MAX_NO_OF_SESSIONS];
	uint32_t OldNumberOfSessions = NumberOfSessionsFound;
	bool changed = false;

	const int32_t waitTime = 5000; // in ms, 5 ms

	static int32_t startTime = timeGetTime();

	// take a list of the old session guids
	for (uint32_t i = 0; i < NumberOfSessionsFound; i++)
	{
		OldSessionGuids[i] = SessionData[i].Guid;
	}

	// do the session enumeration thing
	Net_FindAvPSessions();

	// Have the available sessions changed? first check number of sessions
	if (NumberOfSessionsFound != OldNumberOfSessions)
	{
		changed = true;
	}
	else
	{
		// now check the guids of the new sessions against our previous list
		for (uint32_t i = 0; i < NumberOfSessionsFound; i++)
		{
			if (!IsEqualGUID(OldSessionGuids[i], SessionData[i].Guid)) {
				changed = true;
			}
		}
	}

	if (changed && OldNumberOfSessions > 0)
	{
		// See if our previously selected session is in the new session list
		int OldSelection = *SelectedItem;
		*SelectedItem=0;

		for (uint32_t i = 0; i < NumberOfSessionsFound; i++)
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

NetResult Net_Initialise()
{
	Con_PrintMessage("Initialising ENet networking");

	if (enet_initialize() != 0)
	{
		Con_PrintError("Failed to initialise ENet networking");
		return NET_FAIL;
	}

	// get user config data
	netPortNumber = Config_GetInt("[Networking]", "PortNumber", netPortNumber);
	incomingBandwidth = Config_GetInt("[Networking]", "IncomingBandwidth", 0);
	outgoingBandwidth = Config_GetInt("[Networking]", "OutgoingBandwidth", 0);

	// register console commands
	Con_AddCommand("connect", Net_ConnectToAddress);
	Con_AddCommand("disconnect", Net_Disconnect);

	net_IsInitialised = true;

	Con_PrintMessage("Initialised ENet networking succesfully");

	return NET_OK;
}

void Net_Deinitialise()
{
	Con_PrintMessage("Deinitialising ENet networking");

	// let everyone know we're disconnecting and destroy the host
	Net_Disconnect();

	enet_deinitialize();

	net_IsInitialised = false;
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
		return 0; // this shouldn't really be a failure as such
	}

	MinimalNetCollectMessages();
	if (!netGameData.needGameDescription)
	{
	  	// we now have the game description , so we can go to the configuration menu
	  	return AVPMENU_MULTIPLAYER_CONFIG_JOIN;
	}
	return 1;
}

int Net_ConnectingToLobbiedGame(char* playerName)
{
	OutputDebugString("Net_ConnectingToLobbiedGame\n");
	return 0;
}

NetResult Net_HostGame(char *playerName, char *sessionName, int species, uint16_t gameStyle, uint16_t level)
{
	if (!net_IsInitialised)
	{
		Con_PrintError("Networking is not initialisedr");
		return NET_FAIL;
	}

	// are we already connected?
	if (host)
	{
		// TODO
		Con_PrintError("already connected to something!");
	}

	int maxPlayers = DetermineAvailableCharacterTypes(false);
	if (maxPlayers < 1) maxPlayers = 1;
	if (maxPlayers > 8) maxPlayers = 8;

	if (!netGameData.skirmishMode)
	{
		if (!LobbiedGame)
		{
			ServerAddress.host = ENET_HOST_ANY;
			ServerAddress.port = netPortNumber;

			host = enet_host_create(&ServerAddress,    // the address to bind the server host to.
								32,                      // allow up to 32 clients and/or outgoing connections.
								0,                       // channel limit.
								incomingBandwidth,       // assume any amount of incoming bandwidth.
								outgoingBandwidth);      // assume any amount of outgoing bandwidth.
			if (NULL == host)
			{
				Con_PrintError("Failed to create ENet server");
				return NET_FAIL;
			}

			// create session
			char *customLevelName = GetCustomMultiplayerLevelName(level, gameStyle);
			if (customLevelName[0])
			{
				// add the level name to the beginning of the session name
				char name_buffer[100];
				sprintf(name_buffer, "%s:%s", customLevelName, sessionName);
				if ((Net_CreateSession(name_buffer, maxPlayers, kMultiplayerVersion, gameStyle, 100)) != NET_OK) {
					return NET_FAIL;
				}
			}
			else
			{
				if ((Net_CreateSession(sessionName, maxPlayers, kMultiplayerVersion, gameStyle, level)) != NET_OK) {
					return NET_FAIL;
				}
			}
		}
		else
		{
			// for lobbied games we need to fill in the level number into the existing session description
			if (Net_UpdateSessionDescForLobbiedGame(gameStyle, level) != NET_OK) {
				return NET_FAIL;
			}
		}

		// we're the host
		AvP.Network = I_Host;

		if (!Net_CreatePlayer(playerName)) {
			return NET_FAIL;
		}
	}
	else
	{
		// fake multiplayer (skirmish mode) - need to set the id to an non zero value
		AvPNetID = 100;

		memset(&thisClientPlayer, 0, sizeof(PlayerDetails));
		strncpy(thisClientPlayer.name, playerName, kPlayerNameSize-1);
		thisClientPlayer.name[kPlayerNameSize-1] = '\0';
		thisClientPlayer.playerID   = AvPNetID;
		thisClientPlayer.playerType = 0; // ??
	}

	InitAVPNetGameForHost(species, gameStyle, level);

	Con_PrintMessage("Server running...");

	return NET_OK;
}

void Net_Disconnect()
{
	OutputDebugString("Net_Disconnect\n");

	// need to let everyone know we're disconnecting
	DestroyPlayerOrGroup destroyPlayer;
	destroyPlayer.ID = AvPNetID;
	destroyPlayer.playerType = NET_PLAYERTYPE_PLAYER;
	destroyPlayer.type = 0; // ?

	if (AvP.Network != I_Host) // don't do this if this is the host
	{
		if (Net_SendSystemMessage(AVP_SYSTEM_MESSAGE, NET_SYSTEM_MESSAGE, NET_DESTROYPLAYERORGROUP, (uint8_t*)(&destroyPlayer), sizeof(DestroyPlayerOrGroup)) != NET_OK)
		{
			Con_PrintError("Net_Disconnect - Problem sending destroy player system message!");
			return; // what do we do in this case?
		}
	}
	else
	{
		// DPSYS_HOST or DPSYS_SESSIONLOST ?
	}

	if (host)
	{
		enet_host_destroy(host);
		host = NULL;
	}

	return;
}

uint32_t Net_JoinGame()
{	
	char buf[100];
	sprintf(buf, "Net_JoinGame at %d\n", timeGetTime());
	OutputDebugString(buf);

	if (!net_IsInitialised)
	{
		Con_PrintError("Networking is not initialisedr");
		return 0;
	}

	// enumerate sessions
//	Net_FindAvPSessions();
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

	if (host)
	{
		// TODO
		Con_PrintError("already connected to something!");
	}

	// parse the argument list
	std::string addressString = Con_GetArgument(0);
	std::string tempString;
	static ENetAddress connectionAddress;
	size_t colonPos = 0;
	ENetEvent eEvent;
	uint8_t receiveBuffer[3072];

	colonPos = addressString.find(":");
	tempString = addressString.substr(0, colonPos);

	Con_PrintMessage(tempString);

	enet_address_set_host(&connectionAddress, tempString.c_str());
	connectionAddress.port = (uint16_t)Util::StringToInt(addressString.substr(colonPos + 1));

	// try connect
	host = enet_host_create(NULL,         // create a client host
					1,                      // only allow 1 outgoing connection
					0,                      // channel limit
					incomingBandwidth,      // 57600 / 8 - 56K modem with 56 Kbps downstream bandwidth
					outgoingBandwidth);     // 14400 / 8 - 56K modem with 14 Kbps upstream bandwidth

	if (NULL == host)
	{
		Con_PrintError("Net_ConnectToAddress - Failed to create Enet client");
		return;
	}
	
	ServerPeer = enet_host_connect(host, &connectionAddress, 2, 0);
	if (NULL == ServerPeer)
	{
		Con_PrintError("Net_ConnectToAddress - Failed to init connection to server host");
		return;
	}

	// Wait up to 3 seconds for the connection attempt to succeed.
	if ((enet_host_service(host, &eEvent, 3000) > 0) && (eEvent.type == ENET_EVENT_TYPE_CONNECT))
	{	
		Con_PrintDebugMessage("Net_ConnectToAddress - we connected to server!");
	}
	else
	{
		Con_PrintError("Net_ConnectToAddress - failed to connect to server!");
		return;
	}

	// ask for session info
	Net_SendSystemMessage(AVP_REQUEST_SESSION_INFO, 0, 0, NULL, 0);

	if ((enet_host_service (host, &eEvent, 3000) > 0) && (eEvent.type == ENET_EVENT_TYPE_RECEIVE))
	{
		Con_PrintDebugMessage("Net_ConnectToAddress - we got something from the server!");
		memcpy(&receiveBuffer[0], static_cast<uint8_t*> (eEvent.packet->data), eEvent.packet->dataLength);
		size_t size = eEvent.packet->dataLength;
		enet_packet_destroy(eEvent.packet);

		MessageHeader newHeader;
		memcpy(&newHeader, &receiveBuffer[0], sizeof(MessageHeader));

		if (newHeader.messageType == AVP_SESSION_DATA)
		{
			// get the hosts ip address for later use
			enet_address_get_host_ip(&eEvent.peer->address, SessionData[NumberOfSessionsFound].hostAddress, 16);

			SessionDescription tempSession;

			// grab the session description struct
			assert(sizeof(SessionDescription) == (size - kMessageHeaderSize));
			memcpy(&tempSession, &receiveBuffer[kMessageHeaderSize], sizeof(SessionDescription));

			uint16_t gameStyle = tempSession.gameStyle;
			uint16_t level     = tempSession.level;

			char sessionName[100];
			char levelName[100];

			// split the session name up into its parts
			if (level >= 100)
			{
				// custom level name may be at the start
				strcpy(levelName, tempSession.sessionName);

				char *colon_pos = strchr(levelName,':');
				if (colon_pos)
				{
					*colon_pos = 0;
					strcpy(sessionName, colon_pos+1);
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

			if (tempSession.currentPlayers < tempSession.maxPlayers) {
				SessionData[NumberOfSessionsFound].AllowedToJoin = true;
			}
			else {
				SessionData[NumberOfSessionsFound].AllowedToJoin = false;
			}

			// multiplayer version number (possibly)
			if (tempSession.version != kMultiplayerVersion)
			{
				float version = 1.0f + tempSession.version / 100.0f;
				SessionData[NumberOfSessionsFound].AllowedToJoin = false;
				sprintf(SessionData[NumberOfSessionsFound].Name, "%s (V %.2f)", sessionName, version);
			}
			else
			{
				// get the level number in our list of levels (assuming we have the level)
				int local_index = GetLocalMultiplayerLevelIndex(level, levelName, gameStyle);

				if (local_index < 0)
				{
					// we don't have the level, so ignore this session
					//return;
				}
					
				SessionData[NumberOfSessionsFound].levelIndex = local_index;
			}
/*
			if (!Net_CreatePlayer("TestName"))
				return;
*/
			InitAVPNetGameForJoin();

			netGameData.levelNumber = SessionData[NumberOfSessionsFound].levelIndex;
			netGameData.joiningGameStatus = JOINNETGAME_WAITFORSTART;
		}
	}
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
	if (host) {
		enet_host_service(host, NULL, 0);
	}
}

NetResult Net_Receive(NetID &fromID, NetID &toID, uint8_t *messageData, size_t &dataSize)
{
	// this function is called from a loop. only return something other than NET_OK when we've no more messages
	bool isInternalOnly  = false;
	bool isSystemMessage = false;

	do
	{
		isInternalOnly = false;	// Default.

		ENetEvent eEvent;

		// check server events
		if (host)
		{
			if (enet_host_check_events(host, &eEvent) > 0)
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
						continue; // this is still fine, we might have more messages
					}
					case ENET_EVENT_TYPE_RECEIVE:
					{
						OutputDebugString("Enet received a packet!\n");

						// copy it in, header and all
						memcpy(messageData, static_cast<uint8_t*> (eEvent.packet->data), eEvent.packet->dataLength);
						dataSize = eEvent.packet->dataLength;
						enet_packet_destroy(eEvent.packet);
						continue;
					}
					case ENET_EVENT_TYPE_DISCONNECT:
					{
						OutputDebugString("ENet got a disconnection\n");
						eEvent.peer->data = NULL;
						continue;
					}
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

#if 0
		// we broke from recieve above and got here as we have some data to process (move this to a separate funtion?)
		MessageHeader newHeader;

		MemoryReadStream rs(messageData, dataSize);
		rs.GetBytes((uint8_t*)&newHeader, sizeof(MessageHeader));

		fromID = newHeader.fromID;
		toID   = newHeader.toID;

		switch (newHeader.messageType)
		{
			case AVP_REQUEST_SERVER_INFO:
			{
				// double check..
				if ((kBroadcastID == newHeader.fromID) && (kBroadcastID == newHeader.toID))
				{
					OutputDebugString("Someone sent us a broadcast packet!\n");

					// reply to it? handle this properly..dont send something from a receive function?
					MemoryWriteStream ws(packetBuffer, NET_MESSAGEBUFFERSIZE);

					MessageHeader newReplyHeader;
					newReplyHeader.messageType = AVP_SESSIONDATA;
					newReplyHeader.fromID = AvPNetID;
					newReplyHeader.toID = 0;

					ws.PutBytes((uint8_t*)&newReplyHeader, sizeof(MessageHeader));

					assert(sizeof(netSession) == sizeof(SessionDescription));
					ws.PutBytes((uint8_t*)&netSession, sizeof(SessionDescription));

					// create a packet with session struct inside
					ENetPacket *packet = enet_packet_create(&packetBuffer[0], ws.GetBytesWritten(), ENET_PACKET_FLAG_RELIABLE);

					// send the packet
					if ((enet_peer_send(eEvent.peer, eEvent.channelID, packet) < 0)) {
						Con_PrintError("problem sending session packet!");
					}
					else {
						Con_PrintMessage("sent session details to client");
					}

					return NET_OK;
				}
			}
			case AVP_REQUEST_SESSION_INFO:
			{
				NewOnScreenMessage("SENDING SESSION INFO");

				// a client requested a copy ofthe session data
				MessageHeader newReplyHeader;
				newReplyHeader.messageType = AVP_SESSIONDATA;
				newReplyHeader.fromID = AvPNetID;
				newReplyHeader.toID = 0;

				MemoryWriteStream ws(packetBuffer, NET_MESSAGEBUFFERSIZE);

				ws.PutBytes((uint8_t*)&newReplyHeader, sizeof(MessageHeader));

				assert(sizeof(netSession) == sizeof(SessionDescription));
				ws.PutBytes((uint8_t*)&netSession, sizeof(SessionDescription));

				// create a packet with session struct inside
				ENetPacket *packet = enet_packet_create(&packetBuffer[0], ws.GetBytesWritten(), ENET_PACKET_FLAG_RELIABLE);

				// send the packet
				if ((enet_peer_send(eEvent.peer, eEvent.channelID, packet) < 0))
				{
					Con_PrintError("problem sending session packet!");
					NewOnScreenMessage("problem sending session packet!");
				}
				else Con_PrintMessage("sent session details to client");

				return NET_OK;
			}
			case AVP_SYSTEM_MESSAGE:
			{
				OutputDebugString("We received a system message!\n");
				isSystemMessage = true;
				fromID = NET_SYSTEM_MESSAGE;
				toID = 0; // check!
				return NET_OK;
			}
			case AVP_GETPLAYERNAME:
			{
				OutputDebugString("message check - AVP_GETPLAYERNAME\n");

				int playerFrom = fromID;
				int idOfPlayerToGet = toID;

				PlayerDetails newPlayerDetails;

				int id = PlayerIdInPlayerList(idOfPlayerToGet);

				newPlayerDetails.playerID = id;
				strncpy(newPlayerDetails.name, netGameData.playerData[id].name, kPlayerNameSize);
	//TODO		strncpy(newPlayerDetails.clanTag, netGameData.playerData[id].name, PLAYER_NAME_SIZE);
				newPlayerDetails.playerType = 0; // do i even need this?

				if (NET_IDNOTINPLAYERLIST == id) {
					OutputDebugString("player not in the list!\n");
				}

				// pass it back..
				MessageHeader newReplyHeader;
				newReplyHeader.messageType = AVP_SENTPLAYERNAME;
				newReplyHeader.fromID = playerFrom;
				newReplyHeader.toID   = idOfPlayerToGet;

				MemoryWriteStream ws(packetBuffer, NET_MESSAGEBUFFERSIZE);
				ws.PutBytes((uint8_t*)&newReplyHeader, sizeof(MessageHeader));

				// copy the struct in after the header
				ws.PutBytes((uint8_t*)(&newPlayerDetails), sizeof(PlayerDetails));

				// create a packet with player name struct inside
				ENetPacket *packet = enet_packet_create(&packetBuffer, ws.GetBytesWritten(), ENET_PACKET_FLAG_RELIABLE);

				OutputDebugString("we got player name request, going to send it back\n");

				// send the packet
				if ((enet_peer_send(eEvent.peer, eEvent.channelID, packet) < 0)) {
					Con_PrintError("AVP_GETPLAYERNAME - problem sending player name packet!");
				}

				return NET_OK;
			}
			case AVP_SENTPLAYERNAME:
			{
				int playerFrom = fromID;
				int idOfPlayerWeGot = toID;

				PlayerDetails newPlayerDetails;
				rs.GetBytes((uint8_t*)&newPlayerDetails, sizeof(PlayerDetails));

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
				sprintf(buf, "ERROR: UNKNOWN MESSAGE TYPE: %d\n", newHeader.messageType); 
				OutputDebugString(buf);
				return NET_FAIL;
			}
		}
#endif
	}
	while (isInternalOnly);

	return NET_OK;
}

// used to send a message to the server only
NetResult Net_SendSystemMessage(int messageType, int fromID, int toID, uint8_t *messageData, size_t dataSize)
{
	// create a new header and copy it into the packet buffer
	MessageHeader newMessageHeader;
	newMessageHeader.messageType = messageType;
	newMessageHeader.fromID = fromID;
	newMessageHeader.toID   = toID;

	MemoryWriteStream ws(packetBuffer, sizeof(newMessageHeader));
	ws.PutBytes((uint8_t*)&newMessageHeader, sizeof(newMessageHeader));

	// put data after header
	if (dataSize && messageData) {
		ws.PutBytes((uint8_t*)&messageData, dataSize);
	}

	// create Enet packet
	ENetPacket *packet = enet_packet_create(packetBuffer, ws.GetBytesWritten(), ENET_PACKET_FLAG_RELIABLE);
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

	enet_host_flush(host);

	return NET_OK;
}

NetResult Net_Send(NetID fromID, NetID toID, uint8_t *messageData, size_t dataSize)
{
	if (messageData == NULL) 
	{
		Con_PrintError("Net_Send - messageData was NULL");
		return NET_FAIL;
	}

	MessageHeader newMessageHeader;
	newMessageHeader.messageType = AVP_GAMEDATA;
	newMessageHeader.fromID = fromID;
	newMessageHeader.toID   = toID;

	MemoryWriteStream ws(packetBuffer, NET_MESSAGEBUFFERSIZE);
	
	ws.PutBytes((uint8_t*)&newMessageHeader, sizeof(newMessageHeader));
	ws.PutBytes(messageData, dataSize);

	// create ENet packet
	ENetPacket *packet = enet_packet_create(packetBuffer, ws.GetBytesWritten(), ENET_PACKET_FLAG_RELIABLE);
	if (packet == NULL)
	{
		Con_PrintError("Net_Send - couldn't create packet");
		return NET_FAIL;
	}

	// send to all peers
	if (toID == NET_ID_ALLPLAYERS)
	{
		enet_host_broadcast(host, 0, packet);
	}
	else if (toID == NET_ID_SERVERPLAYER)
	{
		//enet_peer_send(
	}

 	enet_host_flush(host);

	return NET_OK;
}

NetResult Net_ConnectToSession(int sessionNumber, char *playerName)
{
	OutputDebugString("Step 1: Connect To Session\n");

	if (Net_OpenSession(SessionData[sessionNumber].hostAddress) != NET_OK) {
		return NET_FAIL;
	}

	OutputDebugString("Step 2: Connecting to host: ");
	OutputDebugString(SessionData[sessionNumber].hostAddress);

	if (!Net_CreatePlayer(playerName)) {
		return NET_FAIL;
	}

	InitAVPNetGameForJoin();

	netGameData.levelNumber = SessionData[sessionNumber].levelIndex;
	netGameData.joiningGameStatus = JOINNETGAME_WAITFORDESC;
	
	return NET_OK;
}

// this function should be called for the host only?
static bool Net_CreatePlayer(char *playerName)
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
	memset(&thisClientPlayer, 0, sizeof(PlayerDetails));
	strncpy(thisClientPlayer.name, playerName, kPlayerNameSize);
	thisClientPlayer.playerType = NET_PLAYERTYPE_PLAYER;
	thisClientPlayer.playerID	= AvPNetID;

	// do we need to do this as host? (call AddPlayer()..)
	if (AvP.Network != I_Host)
	{
		OutputDebugString("Net_CreatePlayer - Sending player struct\n");

		// send struct as system message
		if (Net_SendSystemMessage(AVP_SYSTEM_MESSAGE, NET_SYSTEM_MESSAGE, NET_CREATEPLAYERORGROUP, reinterpret_cast<uint8_t*>(&thisClientPlayer), sizeof(PlayerDetails)) != NET_OK)
		{
			Con_PrintError("Net_CreatePlayer - Problem sending create player system message!\n");
			return false;
		}
		else
		{
			Con_PrintMessage("Net_CreatePlayer - sent create player system message!\n");
		}
	}
	return true;
}

/*	
	just grab the value from timeGetTime as the player id
	it's probably fairly safe to assume two players will never 
	get the same values for this... 
*/
static uint32_t Net_GetNextPlayerID()
{
	return timeGetTime();
}

NetResult Net_OpenSession(const char *hostName)
{
	OutputDebugString("Net_OpenSession()\n");
	ENetEvent eEvent;

	enet_address_set_host(&ServerAddress, hostName);
	ServerAddress.port = netPortNumber;

	// create Enet client
	host = enet_host_create(NULL,     // create a client host
                1,                      // only allow 1 outgoing connection
				0,                      // channel limit
                incomingBandwidth,      // 57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth
                outgoingBandwidth);     // 14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth

	if (host == NULL)
    {
		Con_PrintError("Net_OpenSession - Failed to create Enet client");
		return NET_FAIL;
    }
	
	ServerPeer = enet_host_connect(host, &ServerAddress, 2, 0);
	if (ServerPeer == NULL)
	{
		Con_PrintError("Net_OpenSession - Failed to init connection to server host");
		return NET_FAIL;
	}

	/* see if we actually connected */
	/* Wait up to 3 seconds for the connection attempt to succeed. */
	if ((enet_host_service (host, &eEvent, 3000) > 0) && (eEvent.type == ENET_EVENT_TYPE_CONNECT))
	{	
		Con_PrintDebugMessage("Net_OpenSession - we connected to server!");
	}
	else
	{
		Con_PrintError("Net_OpenSession - failed to connect to server!\n");
		return NET_FAIL;
	}

	return NET_OK;
}

// try to make this re-entrant
void Net_FindAvPSessions()
{
//	OutputDebugString("Net_FindAvPSessions called\n");
	NumberOfSessionsFound = 0;

	const int kWaitTime = 3000; // in ms
	static bool searchStarted = false;

	/*static*/ ENetHost *searchHost = 0;

	ENetPeer *Peer = NULL;
	ENetEvent eEvent;
//	uint8_t receiveBuffer[NET_MESSAGEBUFFERSIZE]; // can reduce this in size?
	SessionDescription tempSession;
	char sessionName[100] = "";
	char levelName[100]   = "";
	char buf[100];

//	if (!searchStarted)
	{
		// set up broadcast address
		BroadcastAddress.host = ENET_HOST_BROADCAST;
		BroadcastAddress.port = netPortNumber; // can I reuse port?? :\

		// create Enet client
		searchHost = enet_host_create(NULL,    // create a client host
					1,                         // only allow 1 outgoing connection
					0,                         // channel limit
					incomingBandwidth,         // 57600 / 8 - 56K modem with 56 Kbps downstream bandwidth
					outgoingBandwidth);        // 14400 / 8 - 56K modem with 14 Kbps upstream bandwidth

		if (NULL == searchHost)
		{
			Con_PrintError("Failed to create ENet client");
			return;
		}

		Peer = enet_host_connect(searchHost, &BroadcastAddress, 2, 0);
		if (NULL == Peer)
		{
			Con_PrintError("Failed to connect to ENet Broadcast peer");
			return;
		}
	}

	// lets send a message out saying we're looking for available game sessions
	if (enet_host_service(searchHost, &eEvent, kWaitTime) > 0) // this could probably be lower?
	{
		if (ENET_EVENT_TYPE_CONNECT == eEvent.type)
		{
			Con_PrintDebugMessage("Net - Connected for Broadcast");

            // create broadcast header
			MessageHeader newHeader;
			newHeader.messageType = AVP_REQUEST_SERVER_INFO;
			newHeader.fromID      = kBroadcastID;
			newHeader.toID        = kBroadcastID;

			MemoryWriteStream ws(packetBuffer, NET_MESSAGEBUFFERSIZE);
			ws.PutBytes((uint8_t*)&newHeader, sizeof(newHeader));

			// create ENet packet
			ENetPacket *packet = enet_packet_create(packetBuffer, ws.GetBytesWritten(), ENET_PACKET_FLAG_RELIABLE);

			enet_peer_send(Peer, 0, packet);
			enet_host_flush(searchHost);
		}
	}
	
	// need to wait here for servers to respond with session info
	while (enet_host_service (searchHost, &eEvent, kWaitTime) > 0)
	{
		if (ENET_EVENT_TYPE_RECEIVE == eEvent.type)
		{
			MemoryReadStream ws(eEvent.packet->data, eEvent.packet->dataLength);

            // grab the header from the packet
			MessageHeader newHeader;
			ws.GetBytes((uint8_t*)&newHeader, sizeof(newHeader));

			if (AVP_SESSION_DATA == newHeader.messageType)
			{
				Con_PrintDebugMessage("Net - server sent us session data");

				// get the hosts ip address for later use
				enet_address_get_host_ip(&eEvent.peer->address, SessionData[NumberOfSessionsFound].hostAddress, 16);

				// grab the session description struct
				assert(sizeof(tempSession) == (eEvent.packet->dataLength - kMessageHeaderSize));
				ws.GetBytes((uint8_t*)&tempSession, sizeof(tempSession));

				uint16_t gameStyle = tempSession.gameStyle;
				uint16_t level     = tempSession.level;

				// split the session name up into its parts
				if (level >= 100)
				{
					// custom level name may be at the start
					strcpy(levelName, tempSession.sessionName);

					char *colon_pos = strchr(levelName,':');
					if (colon_pos)
					{
						*colon_pos = 0;
						strcpy(sessionName, colon_pos+1);
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

				if (tempSession.currentPlayers < tempSession.maxPlayers) {
					SessionData[NumberOfSessionsFound].AllowedToJoin = true;
				}
				else {
					SessionData[NumberOfSessionsFound].AllowedToJoin = false;
				}

				// multiplayer version number (possibly)
				if (tempSession.version != kMultiplayerVersion)
				{
					float version = 1.0f + tempSession.version / 100.0f;
					SessionData[NumberOfSessionsFound].AllowedToJoin = false;
 					sprintf(SessionData[NumberOfSessionsFound].Name, "%s (V %.2f)", sessionName, version);
				}
				else
				{
					// get the level number in our list of levels (assuming we have the level)
					int local_index = GetLocalMultiplayerLevelIndex(level, levelName, gameStyle);

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
	enet_host_destroy(searchHost);
}

static NetResult Net_CreateSession(const char* sessionName, int maxPlayers, int version, uint16_t gameStyle, uint16_t level)
{
	memset(&netSession, 0, sizeof(netSession));

	strncpy(netSession.sessionName, sessionName, sizeof(netSession.sessionName) - 1);

	netSession.maxPlayers = maxPlayers;

	netSession.version = version;
	netSession.gameStyle = gameStyle;
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

NetResult Net_UpdateSessionDescForLobbiedGame(uint16_t gameStyle, uint16_t level)
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
