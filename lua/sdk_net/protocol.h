#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#define	net_NOP 		0			// nop command used for padding
#define net_Disconnect	1			// disconnect, last message in connection
#define net_File		2			// file transmission message request/deny
#define net_Tick		3			// send last world tick
#define net_StringCmd	4			// a string command
#define net_SetConVar	5			// sends one/multiple convar settings
#define	net_SignonState	6			// signals current signon state
#define	svc_Print			7		// print text to console
#define	svc_ServerInfo		8		// first message from server about game, map etc
#define svc_SendTable		9		// sends a sendtable description for a game class
#define svc_ClassInfo		10		// Info about classes (first byte is a CLASSINFO_ define).							
#define	svc_SetPause		11		// tells client if server paused or unpaused
#define	svc_CreateStringTable	12	// inits shared string tables
#define	svc_UpdateStringTable	13	// updates a string table
#define svc_VoiceInit		14		// inits used voice codecs & quality
#define svc_VoiceData		15		// Voicestream data from the server
#define	svc_Sounds			17		// starts playing sound
#define	svc_SetView			18		// sets entity as point of view
#define	svc_FixAngle		19		// sets/corrects players viewangle
#define	svc_CrosshairAngle	20		// adjusts crosshair in auto aim mode to lock on traget
#define	svc_BSPDecal		21		// add a static decal to the worl BSP
#define svc_UserMessage		23	// a game specific message 
#define svc_EntityMessage	24	// a message for an entity
#define	svc_GameEvent		25	// global game event fired
#define	svc_PacketEntities	26  // non-delta compressed entities
#define	svc_TempEntities	27	// non-reliable event object
#define svc_Prefetch		28	// only sound indices for now
#define svc_Menu			29	// display a menu from a plugin
#define svc_GameEventList	30	// list of known games events and fields
#define svc_GetCvarValue	31	// Server wants to know the value of a cvar on the client.
#define SVC_LASTMSG			31	// last known server messages

#endif