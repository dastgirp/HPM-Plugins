//===== Hercules Plugin ======================================
//= Chat Timestamp
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Will add Timestamp to Party/Guild Chats
//===== Changelog: ===========================================
//= v1.0 - Initial Release.
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
// #define PLAYER_MESSAGE_TIMESTAMP

HPExport struct hplugin_info pinfo =
{
	"Chat TimeStamp",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

char prefix_msg[CHAT_SIZE_MAX + NAME_LENGTH + 3 + 1];

const char *clif_process_chat_message_post(const char *retVal, struct map_session_data *sd, const struct packet_chat_message *packet, char *out_buf, int out_buflen)
{
	time_t t;
	int textlen = 0;
	
	nullpo_ret(sd);

	if (retVal == NULL)
		return NULL;
#if PACKETVER >= 20151001
	// Packet doesn't include a NUL terminator
	textlen = packet->packet_len - 4;
#else // PACKETVER < 20151001
	// Packet includes a NUL terminator
	textlen = packet->packet_len - 4 - 1;
#endif // PACKETVER > 20151001
	safestrncpy(out_buf, retVal, textlen+1); // [!] packet->message is not necessarily NUL terminated
	
	t = time(NULL);
	strftime(prefix_msg, 10, "[%H:%M] ", localtime(&t));
	
	strcat(prefix_msg, out_buf);
	retVal = prefix_msg;
	return retVal;
}

int guild_send_message_pre(struct map_session_data **sd, const char **mes)
{
	char prefix[CHAT_SIZE_MAX + NAME_LENGTH + 3 + 1];
	time_t t;

	nullpo_ret(*sd);

	if ((*sd)->status.guild_id == 0)
		return 0;
	
	t = time(NULL);
	strftime(prefix, 10, "[%H:%M] ", localtime(&t));
	
	strcat(prefix, *mes);
	
	*mes = (const char*)prefix;

	return 1;
}

int party_send_message_pre(struct map_session_data **sd, const char **mes)
{
	char prefix[CHAT_SIZE_MAX + NAME_LENGTH + 3 + 1];
	time_t t;

	nullpo_ret(*sd);

	if ((*sd)->status.party_id == 0)
		return 0;
	
	t = time(NULL);
	strftime(prefix, 10, "[%H:%M] ", localtime(&t));
	
	strcat(prefix, *mes);
	
	*mes = (const char*)prefix;
	return 1;
}

HPExport void plugin_init(void)
{
#ifndef PLAYER_MESSAGE_TIMESTAMP
	addHookPre(party, send_message, party_send_message_pre);
	addHookPre(guild, send_message, guild_send_message_pre);
#else
	addHookPost(clif, process_chat_message, clif_process_chat_message_post);
#endif
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
