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

HPExport struct hplugin_info pinfo =
{
	"Chat TimeStamp",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

int guild_send_message_pre(struct map_session_data **sd, const char **mes)
{
	char prefix[CHAT_SIZE_MAX + NAME_LENGTH + 3 + 1];

	nullpo_ret(*sd);

	if ((*sd)->status.guild_id == 0)
		return 0;
	
	time_t t = time(NULL);
	strftime(prefix, 10, "[%H:%M] ", localtime(&t));
	
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
	
	time_t t = time(NULL);
	strftime(prefix, 10, "[%H:%M] ", localtime(&t));
	
	strcat(prefix, *mes);
	
	*mes = (const char*)prefix;
	return 1;
}

HPExport void plugin_init (void)
{
	addHookPre(party, send_message, party_send_message_pre);
	addHookPre(guild, send_message, guild_send_message_pre);
}

HPExport void server_online (void)
{
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}
