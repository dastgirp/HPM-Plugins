//===== Hercules Plugin ======================================
//= @afk
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.4
//===== Description: =========================================
//= Will change your look to contain afk bubble without
//= needing a player to be online
//===== Changelog: ===========================================
//= v1.0 - Initial Conversion
//= v1.1 - Dead Person cannot @afk.
//= v1.2 - Added afk_timeout option and battle_config for it
//= v1.3 - Added noafk mapflag.
//= v1.4 - Compatible with new Hercules.
//===== Additional Comments: =================================
//= AFK Timeout Setting(BattleConf):
//= 	afk_timeout: TimeInSeconds
//= noafk Mapflag:
//= 	prontera	mapflag	noafk
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
#include "map/battle.h"
#include "map/channel.h"
#include "map/clif.h"
#include "map/map.h"
#include "map/npc.h"
#include "map/pc.h"
#include "map/script.h"
#include "map/skill.h"
#include "map/status.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo =
{
	"@afk",
	SERVER_TYPE_MAP,
	"1.4",
	HPM_VERSION,
};

int afk_timeout = 0;  // @afk Timeout

struct plugin_mapflag {
	unsigned noafk : 1; // No Detect NoMapFlag
};

ACMD(afk)
{
	// MapFlag Restriction
	struct plugin_mapflag *mf_data = getFromMAPD(&map->list[sd->bl.m], 0);
	if (mf_data && mf_data->noafk) {
		clif->message(fd, "@afk is forbidden in this map.");
		return true;
	}
	if (pc_isdead(sd)) {
		clif->message(fd, "Cannot use @afk While dead.");
		return true;
	}
	if (DIFF_TICK(timer->gettick(),sd->canlog_tick) < battle->bc->prevent_logout) {
		clif->message(fd, "Failed to use @afk, please try again later.");
		return true;
	}
	sd->state.autotrade = 1;
	sd->block_action.immune = 1;
	pc_setsit(sd);
	skill->sit(sd, 1);
	clif->sitting(&sd->bl);
	clif->changelook(&sd->bl, LOOK_HEAD_TOP, 471);
	clif->specialeffect(&sd->bl, 234, AREA);
	if (afk_timeout) {
		status->change_start(NULL, &sd->bl, SC_AUTOTRADE, 10000, 0, 0, 0, 0, afk_timeout * 1000,0);
	}
	channel->quit(sd); //Quit from Channels.
	clif->authfail_fd(fd, 15);
	return true;
}

void afk_timeout_adjust(const char *key, const char *val)
{
	int value = config_switch(val);
	if (strcmpi(key,"battle_configuration/afk_timeout") == 0) {
		if (value < 0) {
			ShowDebug("Received Invalid Setting for afk_timeout(%d), defaulting to 0\n", value);
			return;
		}
		afk_timeout = value;
	}
	return;
	
}

int afk_timeout_return(const char *key)
{
	if (strcmpi(key, "battle_configuration/afk_timeout") == 0)
		return afk_timeout;

	return 0;
}

void npc_parse_unknown_mapflag_pre(const char **name, const char **w3, const char **w4, const char **start, const char **buffer, const char **filepath, int **retval)
{
	int16 m = map->mapname2mapid(*name);
	if (strcmpi(*w3, "noafk") == 0) {
		struct plugin_mapflag *mf_data;
		if ((mf_data = getFromMAPD(&map->list[m], 0)) == NULL) {
			CREATE(mf_data, struct plugin_mapflag, 1);
			mf_data->noafk = 1;
			addToMAPD(&map->list[m], mf_data, 0, true);
		}
		mf_data->noafk = 1;
		hookStop();
	}
	
	return;
}

HPExport void plugin_init(void)
{
	addAtcommand("afk", afk);
	addHookPre(npc, parse_unknown_mapflag, npc_parse_unknown_mapflag_pre);
}

HPExport void server_preinit(void)
{
	addBattleConf("battle_configuration/afk_timeout", afk_timeout_adjust, afk_timeout_return, false);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
