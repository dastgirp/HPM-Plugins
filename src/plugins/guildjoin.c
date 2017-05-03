//===== Hercules Plugin ======================================
//= Guild Join ScriptCommand
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= 
//===== Changelog: ===========================================
//= v1.0 - Initial Conversion
//===== Additional Comments: =================================
//= 
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
#include "common/socket.h"
#include "common/strlib.h"
#include "common/timer.h"
#include "common/mapindex.h"

#include "map/atcommand.h"
#include "map/clif.h"
#include "map/map.h"
#include "map/pc.h"
#include "map/guild.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo =
{
	"guildjoin",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

BUILDIN(guildjoin)
{
	int guild_id = script_getnum(st,2), i, count = 0;
	struct map_session_data *sd;
	struct guild *g;
	if ((sd = map->charid2sd(script_getnum(st, 3))) == NULL) {
		script_pushint(st, 0);
		return false;
	}
	if (sd->status.guild_id) {
		script_pushint(st, 0);
		return false;
	}
	if ((g = guild->search(guild_id)) == NULL) {
		script_pushint(st, 0);
		return false;
	}
	for (i = 0; i < g->max_member; i++) {
		if (g->member[i].account_id > 0) {
			count++;
		}
	}
	if (count >= g->max_member) {
		script_pushint(st, 0);
		return false;
	}
	sd->guild_invite = guild_id;
	script_pushint(st, guild->reply_invite(sd, guild_id, 1));
	return true;
}

HPExport void plugin_init(void)
{
	addScriptCommand("guildjoin", "ii", guildjoin);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
