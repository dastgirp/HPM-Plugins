//===== Hercules Plugin ======================================
//= Chat Timestamp
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.1
//===== Description: =========================================
//= Will add Timestamp to Party/Guild Chats
//===== Changelog: ===========================================
//= v1.0 - Initial Release.
//= v1.1 - Timestamp can be added to whisper. Fixed PM to NPC.
//===== Repo Link: ===========================================
//= https://github.com/dastgir/HPM-Plugins
//============================================================

#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/HPMi.h"
#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/nullpo.h"
#include "common/strlib.h"
#include "common/socket.h"

#include "map/clif.h"
#include "map/guild.h"
#include "map/map.h"
#include "map/party.h"
#include "map/pc.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

/**
 * Enables TimeStamp in Player Messages.
 * This Options Enables Timestamp everywhere a player can send message to.
 **/
//#define ALL_MESSAGE_TIMESTAMP
// Timestamp for Whispers
// Message Appears as "Name: [Time] Message"
//#define WHISPER_TIMESTAMP
// Timestamp for Party Chats
#define PARTY_MESSAGE_TIMESTAMP
// Timestamp for Guild Chats
#define GUILD_MESSAGE_TIMESTAMP

HPExport struct hplugin_info pinfo =
{
	"Chat TimeStamp",
	SERVER_TYPE_MAP,
	"1.1",
	HPM_VERSION,
};

// Don't touch below this
#ifdef ALL_MESSAGE_TIMESTAMP
	#undef PARTY_MESSAGE_TIMESTAMP
	#undef GUILD_MESSAGE_TIMESTAMP
	// Whisper is handled in different function
	#ifndef WHISPER_TIMESTAMP
		#define WHISPER_TIMESTAMP
	#endif
#endif

void get_timestamp(char* string)
{
	time_t t;
	t = time(NULL);
	strftime(string, sizeof(string), "[%H:%M] ", localtime(&t));
	return;
}

const char *clif_process_chat_message_post(const char *retVal, struct map_session_data *sd, const struct packet_chat_message *packet, char *out_buf, int out_buflen)
{
	char time_prefix[CHAT_SIZE_MAX + NAME_LENGTH + 3 + 1 + 10];
	
	nullpo_ret(sd);

	if (retVal == NULL)
		return NULL;
	
	get_timestamp(time_prefix);

	strcat(time_prefix, out_buf);
	safestrncpy(out_buf, time_prefix, strlen(time_prefix)+1);

	retVal = out_buf;
	return retVal;
}

int guild_send_message_pre(struct map_session_data **sd, const char **mes)
{
	char prefix[CHAT_SIZE_MAX + NAME_LENGTH + 3 + 1];

	nullpo_ret(*sd);

	if ((*sd)->status.guild_id == 0)
		return 0;
	
	get_timestamp(prefix);
	
	strcat(prefix, *mes);
	
	*mes = (const char*)prefix;

	return 1;
}

int party_send_message_pre(struct map_session_data **sd, const char **mes)
{
	char prefix[CHAT_SIZE_MAX + NAME_LENGTH + 3 + 1];

	nullpo_ret(*sd);

	if ((*sd)->status.party_id == 0)
		return 0;
	
	get_timestamp(prefix);
	
	strcat(prefix, *mes);
	
	*mes = (const char*)prefix;
	return 1;
}

/// Whisper is transmitted to the destination player (ZC_WHISPER).
/// 0097 <packet len>.W <nick>.24B <message>.?B
/// 0097 <packet len>.W <nick>.24B <isAdmin>.L <message>.?B (PACKETVER >= 20091104)
void clif_wis_message_pre(int *fd_, const char **nick_, const char **mes_, int *mes_len_)
{
	char prefix[CHAT_SIZE_MAX + NAME_LENGTH + 3 + 1];
	int fd = *fd_, mes_len = *mes_len_;
	const char *nick = *nick_, *mes = *mes_;
#if PACKETVER >= 20091104
	struct map_session_data *ssd = NULL;
#endif // PACKETVER >= 20091104
	nullpo_retv(nick);
	nullpo_retv(mes);

	get_timestamp(prefix);

	mes_len += strlen(prefix);

	strcat(prefix, mes);

	mes = (const char*)prefix;

#if PACKETVER < 20091104
	WFIFOHEAD(fd, mes_len + NAME_LENGTH + 5);
	WFIFOW(fd,0) = 0x97;
	WFIFOW(fd,2) = mes_len + NAME_LENGTH + 5;
	safestrncpy(WFIFOP(fd,4), nick, NAME_LENGTH);
	safestrncpy(WFIFOP(fd,28), mes, mes_len + 1);
	WFIFOSET(fd,WFIFOW(fd,2));
#else
	ssd = map->nick2sd(nick, false);

	WFIFOHEAD(fd, mes_len + NAME_LENGTH + 9);
	WFIFOW(fd,0) = 0x97;
	WFIFOW(fd,2) = mes_len + NAME_LENGTH + 9;
	safestrncpy(WFIFOP(fd,4), nick, NAME_LENGTH);
	WFIFOL(fd,28) = (ssd && pc_get_group_level(ssd) == 99) ? 1 : 0; // isAdmin; if nonzero, also displays text above char
	safestrncpy(WFIFOP(fd,32), mes, mes_len + 1);
	WFIFOSET(fd,WFIFOW(fd,2));
#endif
	hookStop();
}

HPExport void plugin_init(void)
{
#ifdef PARTY_MESSAGE_TIMESTAMP
	addHookPre(party, send_message, party_send_message_pre);
#endif
#ifdef GUILD_MESSAGE_TIMESTAMP
	addHookPre(guild, send_message, guild_send_message_pre);
#endif
#ifdef ALL_MESSAGE_TIMESTAMP
	addHookPost(clif, process_chat_message, clif_process_chat_message_post);
#endif
#ifdef WHISPER_TIMESTAMP
	addHookPre(clif, wis_message, clif_wis_message_pre);
#endif
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
