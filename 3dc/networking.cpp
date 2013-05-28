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
#include "MemoryStream.h"

static ENetHost *host = NULL;
static bool isHost = false;

static ENetAddress  ServerAddress;
static ENetAddress  ClientAddress;
static ENetPeer     *ServerPeer = NULL;
static bool net_IsInitialised = false;

static ENetSocket broadcastSocket = ENET_SOCKET_NULL;

extern void NewOnScreenMessage(char *messagePtr);

NetID AvPNetID = 0; // rename this
PlayerDetails thisClientPlayer;

// used to hold message data
const int kPacketBufferSize = NET_MESSAGEBUFFERSIZE;
static uint8_t packetBuffer[kPacketBufferSize];

static uint32_t netPortNumber = 23513;
static const uint32_t broadcastPort = 23514; // checkme

const int kBroadcastID = 255;
static uint32_t sessionCheckTimer = 0;
const uint32_t  sessionCheckTimeout = 5000; // 5 second timeout

static uint32_t incomingBandwidth = 0;
static uint32_t outgoingBandwidth = 0;

SessionDescription netSession; // main game session

extern void MinimalNetCollectMessages(void);
extern void InitAVPNetGameForHost(int species, int gamestyle, int level);
extern void InitAVPNetGameForJoin(void);
extern int DetermineAvailableCharacterTypes(bool ConsiderUsedCharacters);
extern char *GetCustomMultiplayerLevelName(int index, int gameType);
extern unsigned char DebouncedKeyboardInput[];

static bool Net_CreatePlayer(char *playerName);
static NetResult Net_CreateSession(const char *sessionName, const char *levelName, int maxPlayers, int version, uint16_t gameStyle, uint16_t level);
static NetResult Net_UpdateSessionDescForLobbiedGame(uint16_t gameStyle, uint16_t level);

/* KJL 14:58:18 03/07/98 - AvP's Guid */
// {379CCA80-8BDD-11d0-A078-004095E16EA5}
static const GUID AvPGuid = { 0x379cca80, 0x8bdd, 0x11d0, { 0xa0, 0x78, 0x0, 0x40, 0x95, 0xe1, 0x6e, 0xa5 } };

const uint32_t kMessageHeaderSize = sizeof(MessageHeader);

/* Some notes on how DirectPlay worked (and what AvP might be expecting)

	System messages are sent to ALL players


*/

static void Net_AddSession(SessionDescription &session)
{
	uint16_t gameStyle = session.gameStyle;
	uint16_t level     = session.level;

	SessionData[NumberOfSessionsFound].host = session.host;
	SessionData[NumberOfSessionsFound].port = session.port;

	sprintf(SessionData[NumberOfSessionsFound].Name,"%s (%d/%d)", session.sessionName, session.nPlayers, session.maxPlayers);

	SessionData[NumberOfSessionsFound].Guid	= session.guidInstance;

	if (session.nPlayers < session.maxPlayers) {
		SessionData[NumberOfSessionsFound].AllowedToJoin = true;
	}
	else {
		SessionData[NumberOfSessionsFound].AllowedToJoin = false;
	}

	// multiplayer version number (possibly)
	if (session.version != kMultiplayerVersion)
	{
		float version = 1.0f + session.version / 100.0f;
		SessionData[NumberOfSessionsFound].AllowedToJoin = false;
 		sprintf(SessionData[NumberOfSessionsFound].Name, "%s (V %.2f)", session.sessionName, version);
	}
	else
	{
		// get the level number in our list of levels (assuming we have the level)
		int local_index = GetLocalMultiplayerLevelIndex(level, session.levelName, gameStyle); // bjd - CHECKME
		if (local_index < 0)
		{
			// we don't have the level, so ignore this session
//			return;
		}
						
		SessionData[NumberOfSessionsFound].levelIndex = local_index;
	}

	NumberOfSessionsFound++;
}

bool Net_UpdateSessionList(int *SelectedItem)
{
	if (timeGetTime() - sessionCheckTimer >= sessionCheckTimeout) {
		sessionCheckTimer = 0;
		return false;
	}

	GUID OldSessionGuids[MAX_NO_OF_SESSIONS];
	uint32_t OldNumberOfSessions = NumberOfSessionsFound;
	bool changed = false;

	// take a list of the old session guids
	for (uint32_t i = 0; i < NumberOfSessionsFound; i++) {
		OldSessionGuids[i] = SessionData[i].Guid;
	}

	// poll network for session data
	{
		MessageHeader header;
		
		ENetBuffer buf;
		buf.data = &packetBuffer;
		buf.dataLength = kPacketBufferSize;

		enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;

		MemoryReadStream rs(packetBuffer, kPacketBufferSize);

		while (enet_socket_wait(broadcastSocket, &events, 0) >= 0 && events)
		{
			ENetAddress address;
			int len = enet_socket_receive(broadcastSocket, &address, &buf, 1);

			if (len <= 0) {
				break;
			}

			// hack - I think we'll receive our own broadcast?
			if (len == 12) {
				OutputDebugString("We received our request back\n");
			}

			else if (len == sizeof(header) + sizeof(SessionDescription)) 
			{
				OutputDebugString("Got some session info\n");

				rs.GetBytes(&header, sizeof(header));

				if (header.messageType == NETSYS_SESSION_INFO)
				{
					OutputDebugString("Net_AddSession called\n");
					SessionDescription newSession;
					rs.GetBytes(&newSession, sizeof(newSession));

					// FIXME? - server should set this from their side correctly?
					newSession.host = address.host;

					Net_AddSession(newSession);
				}
			}
			else {
				char tprint[100];
				sprintf(tprint, "Got data of size %d\n", len);
				OutputDebugString(tprint);
			}
		}
	}

	// Have the available sessions changed? first check number of sessions
	if (NumberOfSessionsFound != OldNumberOfSessions) {
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
//	broadcastPort = Config_GetInt("[Networking]", "BroadcastPort", broadcastPort);
	incomingBandwidth = Config_GetInt("[Networking]", "IncomingBandwidth", 0);
	outgoingBandwidth = Config_GetInt("[Networking]", "OutgoingBandwidth", 0);

	broadcastSocket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
	if (ENET_SOCKET_NULL == broadcastSocket) {
		Con_PrintError("Net_Initialise - Couldn't create broadcast socket");
	}
	else {
		enet_socket_set_option(broadcastSocket, ENET_SOCKOPT_NONBLOCK,  1);
		enet_socket_set_option(broadcastSocket, ENET_SOCKOPT_BROADCAST, 1);

		ENetAddress thisHost;
		thisHost.host = ENET_HOST_ANY;
		thisHost.port = broadcastPort;
		if (enet_socket_bind(broadcastSocket, &thisHost) < 0) {
			Con_PrintError("Net_Initialise - Couldn't bind broadcast socket");
			enet_socket_destroy(broadcastSocket);
			broadcastSocket = ENET_SOCKET_NULL;
		}
	}

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
		return 0;
	}

	MinimalNetCollectMessages();
	if (!netGameData.needGameDescription)
	{
		// we now have the game description , so we can go to the configuration menu
		return AVPMENU_MULTIPLAYER_CONFIG_JOIN;
	}
	return 1;
}

int Net_ConnectingToLobbiedGame(char *playerName)
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

			host = enet_host_create(&ServerAddress,      // the address to bind the server host to.
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
			char *levelName = GetCustomMultiplayerLevelName(level, gameStyle);
			if (IsCustomLevel(level, gameStyle))
			{
				if ((Net_CreateSession(sessionName, levelName, maxPlayers, kMultiplayerVersion, gameStyle, 100)) != NET_OK) {
					return NET_FAIL;
				}
			}
			else 
			{
				if ((Net_CreateSession(sessionName, levelName, maxPlayers, kMultiplayerVersion, gameStyle, level)) != NET_OK) {
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
		thisClientPlayer.ID   = AvPNetID;
		thisClientPlayer.type = 0; // ??
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
	destroyPlayer.ID   = AvPNetID;
	destroyPlayer.type = NET_PLAYERTYPE_PLAYER;

	if (AvP.Network != I_Host) // don't do this if this is the host
	{
		if (Net_SendSystemMessage(NET_SYSTEM_MESSAGE, NET_DESTROYPLAYERORGROUP, &destroyPlayer, sizeof(DestroyPlayerOrGroup)) != NET_OK)
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

	enet_socket_destroy(broadcastSocket);

	return;
}

uint32_t Net_JoinGame()
{	
//	char buf[100];
//	sprintf(buf, "Net_JoinGame at %d\n", timeGetTime());
//	OutputDebugString(buf);

	if (!net_IsInitialised)
	{
		Con_PrintError("Networking is not initialisedr");
		return 0;
	}

	// enumerate sessions
	return NumberOfSessionsFound;
}

// called from the console
void Net_ConnectToAddress()
{
#if 0
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

	colonPos = addressString.find(":");
	tempString = addressString.substr(0, colonPos);

	Con_PrintMessage(tempString);

	enet_address_set_host(&connectionAddress, tempString.c_str());
	connectionAddress.port = (uint16_t)Util::StringToInt(addressString.substr(colonPos + 1));

	// try connect
	host = enet_host_create(NULL,           // create a client host
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
		MemoryReadStream rs(eEvent.packet->data, eEvent.packet->dataLength);

		MessageHeader newHeader;
		rs.GetBytes(&newHeader, kMessageHeaderSize);

		if (newHeader.messageType == AVP_SESSION_DATA)
		{
			// get the hosts ip address for later use
//			enet_address_get_host_ip(&eEvent.peer->address, SessionData[NumberOfSessionsFound].hostAddress, 16);

			SessionDescription tempSession;

			// grab the session description struct
			rs.GetBytes(&tempSession, sizeof(SessionDescription));

			uint16_t gameStyle = tempSession.gameStyle;
			uint16_t level     = tempSession.level;

			sprintf(SessionData[NumberOfSessionsFound].Name,"%s (%d/%d)", tempSession.sessionName, tempSession.nPlayers, tempSession.maxPlayers);

			SessionData[NumberOfSessionsFound].Guid	= tempSession.guidInstance;

			if (tempSession.nPlayers < tempSession.maxPlayers) {
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
				sprintf(SessionData[NumberOfSessionsFound].Name, "%s (V %.2f)", tempSession.sessionName, version);
			}
			else
			{
				// get the level number in our list of levels (assuming we have the level)
				int local_index = GetLocalMultiplayerLevelIndex(level, tempSession.levelName, gameStyle); // bjd - CHECKME

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

		enet_packet_destroy(eEvent.packet);
	}
#endif
}

int Net_InitLobbiedGame()
{
	OutputDebugString("Net_InitwhoLobbiedGame\n");
	extern char MP_PlayerName[];
	return 0;
}

static void Net_CheckForSessionRequest()
{
	enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;

	while (enet_socket_wait(broadcastSocket, &events, 0) >= 0 && events)
	{
		Con_PrintDebugMessage("Net_CheckForSessionRequest got something");
		ENetAddress addr;
		ENetBuffer buf;
		buf.data = packetBuffer;
		buf.dataLength = kPacketBufferSize;

		int len = enet_socket_receive(broadcastSocket, &addr, &buf, 1);
		if (0 == len) {
			return; // blocking - check later
		}
		if (len > 0) {
			Con_PrintDebugMessage("Got a packet of some sort..");
		}
		if (len == sizeof(MessageHeader))
		{
			MessageHeader header;
			memcpy(&header, packetBuffer, sizeof(MessageHeader));

			if (NETSYS_REQUEST_SESSION_INFO == header.messageType)
			{
				NewOnScreenMessage("Got asked for session information. sending...");
				MemoryWriteStream ws(packetBuffer, kPacketBufferSize);

				header.messageType = NETSYS_SESSION_INFO;
				ws.PutBytes(&header, sizeof(header));
				ws.PutBytes(&netSession, sizeof(SessionDescription));
				
				buf.dataLength = ws.GetBytesWritten();
				buf.data = packetBuffer;

					int ret = enet_socket_send(broadcastSocket, &addr, &buf, 1);
					if (0 == ret) {
						// blocking - wait 1 second then try again (for a maximum of 5 seconds)
//						Sleep(1000);
						return;
					}
					else if (0 > ret) {
						Con_PrintError("Couldn't send session details");
						return;
					}
					else {
						Con_PrintDebugMessage("Sent session details");
						return;
					}
//				}
			}
		}
	}
}

// call once per frame
void Net_ServiceNetwork()
{
	if (host)
	{
		enet_host_service(host, NULL, 0);

		// also check for session data requests
		Net_CheckForSessionRequest();
	}
}

NetResult Net_Receive(uint8_t *messageData, size_t &dataSize)
{
	// this function is called from a loop. only return something other than NET_OK when we've no more messages
	if (host)
	{
		dataSize = 0;

		ENetEvent eEvent;
		if (enet_host_check_events(host, &eEvent) > 0) // check server events
		{
			NewOnScreenMessage("got an enet message from enet_host_check_events in Net_Receive");
			switch (eEvent.type)
			{
				case ENET_EVENT_TYPE_CONNECT:
				{
					char ipaddr[32];
					enet_address_get_host_ip(&eEvent.peer->address, ipaddr, sizeof(ipaddr));

					std::stringstream sstream;
					sstream << "Enet got a connection from " << ipaddr << ":" << eEvent.peer->address.port;
					NewOnScreenMessage((char*)sstream.str().c_str());
					break;
				}
				case ENET_EVENT_TYPE_RECEIVE:
				{
					NewOnScreenMessage("Enet received a packet!\n");

					// copy it in, header and all
					memcpy(messageData, eEvent.packet->data, eEvent.packet->dataLength);
					dataSize = eEvent.packet->dataLength;
					enet_packet_destroy(eEvent.packet);
					break;
				}
				case ENET_EVENT_TYPE_DISCONNECT:
				{
					NewOnScreenMessage("ENet got a disconnection\n");
					eEvent.peer->data = NULL;
					break;
				}
				case ENET_EVENT_TYPE_NONE:
						return NET_NO_MESSAGES;
					break;
				default: break;
			}
		}
		else
		{
			return NET_NO_MESSAGES;
		}
	}

	return NET_OK;
}

// used to send a message to the server only
NetResult Net_SendSystemMessage(NetID fromID, NetID toID, const void *messageData, size_t dataSize)
{
	// create a new header and copy it into the packet buffer
	MessageHeader newMessageHeader;
	newMessageHeader.messageType = NET_SYSTEM_MESSAGE;
	newMessageHeader.fromID = fromID;
	newMessageHeader.toID   = toID;

	MemoryWriteStream ws(packetBuffer, kPacketBufferSize);
	ws.PutBytes(&newMessageHeader, sizeof(newMessageHeader));

	// put data after header
	if (dataSize && messageData) {
		ws.PutBytes(messageData, dataSize);
	}

	// create ENet packet
	ENetPacket *packet = enet_packet_create(packetBuffer, ws.GetBytesWritten(), ENET_PACKET_FLAG_RELIABLE);
	if (packet == NULL)
	{
		Con_PrintError("Net_SendSystemMessage - couldn't create packet");
		return NET_FAIL;
	}

	// let us know if we're not connected
	if (ENET_PEER_STATE_CONNECTED != ServerPeer->state) {
		Con_PrintError("Net_SendSystemMessage - not connected to server peer!");
	}

	// send the packet
	if (enet_peer_send(ServerPeer, 0, packet) != 0) {
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

	MemoryWriteStream ws(packetBuffer, kPacketBufferSize);
	
	ws.PutBytes(&newMessageHeader, sizeof(newMessageHeader));
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
		enet_host_broadcast(host, 0, packet); // FIXME
	}

	enet_host_flush(host);

	return NET_OK;
}

NetResult Net_ConnectToSession(int sessionNumber, char *playerName)
{
	OutputDebugString("Step 1: Connect To Session\n");

	if (host) {
		enet_host_destroy(host);
		host = NULL;
	}

	ServerAddress.host = SessionData[sessionNumber].host;
	ServerAddress.port = SessionData[sessionNumber].port;

	ENetAddress ourAddress;
	ourAddress.host = ENET_HOST_ANY;
	ourAddress.port = netPortNumber;

	// create ENet client
	host = enet_host_create(&ourAddress,       // create a client host
				1,                      // only allow 1 outgoing connection
				0,                      // channel limit
				incomingBandwidth,      // 57600 / 8 : 56K modem with 56 Kbps downstream bandwidth
				outgoingBandwidth);     // 14400 / 8 : 56K modem with 14 Kbps upstream bandwidth

	if (host == NULL)
	{
		Con_PrintError("Net_ConnectToSession - Failed to create Enet client");
		return NET_FAIL;
	}
	
	ServerPeer = enet_host_connect(host, &ServerAddress, 2, 0);
	if (ServerPeer == NULL)
	{
		Con_PrintError("Net_ConnectToSession - Failed to init connection to server host");
		return NET_FAIL;
	}

	ENetEvent eEvent;

	/* see if we actually connected */
	/* Wait up to 3 seconds for the connection attempt to succeed. */
	if ((enet_host_service(host, &eEvent, 3000) > 0) && (eEvent.type == ENET_EVENT_TYPE_CONNECT))
	{
		Con_PrintDebugMessage("Net_ConnectToSession - we connected to server!");
	}
	else
	{
		Con_PrintError("Net_ConnectToSession - failed to connect to server!\n");
		return NET_FAIL;
	}

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
	memset(&thisClientPlayer, 0, sizeof(PlayerDetails));

	if (AvP.Network == I_Host) {
		thisClientPlayer.ID	  = Net_GetNextPlayerID();
		AvPNetID = thisClientPlayer.ID;
	}	
	
	thisClientPlayer.type = NET_PLAYERTYPE_PLAYER;
	strncpy(thisClientPlayer.name, playerName, kPlayerNameSize - 1);

	// do we need to do this as host? (call AddPlayer()..)
	if (AvP.Network != I_Host)
	{
		OutputDebugString("Net_CreatePlayer - Sending player struct\n");

		// send struct as system message
		if (Net_SendSystemMessage(NET_SYSTEM_MESSAGE, NET_CREATEPLAYERORGROUP, &thisClientPlayer, sizeof(PlayerDetails)) != NET_OK)
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
NetID Net_GetNextPlayerID()
{
	return (NetID)timeGetTime();
}

void Net_FindAvPSessions()
{
	// stop this function being called if we're already searching.
	// this is a bit hacky but needs to be here for the moment due to
	// the horrible menu system
	if (sessionCheckTimer != 0) {
		return;
	}

	if (ENET_SOCKET_NULL == broadcastSocket) {
		sessionCheckTimer = 0;
		return;
	}

	ENetAddress destAddress;
	destAddress.host = ENET_HOST_BROADCAST;
	destAddress.port = broadcastPort;

	// create broadcast header
	MessageHeader newHeader;
	newHeader.messageType = NETSYS_REQUEST_SESSION_INFO;
	newHeader.fromID      = kBroadcastID;
	newHeader.toID        = kBroadcastID;

	ENetBuffer buf;
	buf.data = &newHeader;
	buf.dataLength = sizeof(MessageHeader);

	for (int i = 0; i < 5; i++) 
	{
		int ret = enet_socket_send(broadcastSocket, &destAddress, &buf, 1);
		if (0 == ret) {
			// blocking - wait 1 second then try again (for a maximum of 5 seconds)
			Sleep(1000);
		}
		else if (0 > ret) {
			Con_PrintError("Couldn't send broadcast packet");
			return;
		}
		else {
			Con_PrintDebugMessage("Sent broadcast packet");
			break;
		}
	}

	sessionCheckTimer = timeGetTime();
}

static NetResult Net_CreateSession(const char *sessionName, const char *levelName, int maxPlayers, int version, uint16_t gameStyle, uint16_t level)
{
	memset(&netSession, 0, sizeof(netSession));

#ifdef WIN32
	CoCreateGuid(&netSession.guidInstance);
#endif

	// set the application guid
	netSession.guidApplication = AvPGuid;

	netSession.host = ServerAddress.host;
	netSession.port = ServerAddress.port;
	netSession.version = version;
	netSession.gameStyle = gameStyle;
	netSession.level     = level;
	// should this be done here? we might want a dedicated server. perhaps a host should be 'connecting' to themselves
	netSession.nPlayers  = 1;
	netSession.maxPlayers = maxPlayers;

	strncpy(netSession.levelName  , levelName,   sizeof(netSession.levelName)   - 1);
	strncpy(netSession.sessionName, sessionName, sizeof(netSession.sessionName) - 1);

	return NET_OK;
}

NetResult Net_UpdateSessionDescForLobbiedGame(uint16_t gameStyle, uint16_t level)
{
	return NET_OK;
}
