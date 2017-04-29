//===== Hercules Plugin ======================================
//= @deadon
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Starts TrickDead to specified player.
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
#include "map/status.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo =
{
	"@deadon",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

ACMD(deadon)
{
	struct map_session_data *pl_sd;
	struct s_mapiterator *iter;
	
	if (*message == NULL) {
		clif->message(fd,"Usage: @deadon <CharacterName/all>");
		return false;
	}

	if (strcmpi(message, "all") != 0 (pl_sd = map->nick2sd(message)) == NULL && (pl_sd = map->charid2sd(atoi(message))) == NULL) {
		clif->message(fd, msg_txt(3));	// Character not found.
		return false;
	}

	if (pc_get_group_level(pl_sd) > pc_get_group_level(sd)) {
		clif->message(fd, "You cannot use this command on higher GM level.");
		return false;
	}
	
	if (strcmpi(message, "all") == 0) {
		iter = mapit_getallusers();
		for (pl_sd = BL_UCCAST(BL_PC, mapit->first(iter)); mapit->exists(iter); pl_sd = BL_UCCAST(BL_PC, mapit->next(iter))) {
			if (pl_sd->status.char_id == sd->status.char_id || sd->bl.m != pl_sd->bl.m)
				continue; 
			if (pc_get_group_level(pl_sd) > pc_get_group_level(sd)) {
				continue;
			}
			sc_start(&pl_sd->bl, SC_TRICKDEAD, 100, 0, 0);
			clif->message(pl_sd->fd, "You were forced to lie down by GM.");
		}
		mapit->free(iter);
		clif->message(fd, "All Players are acting dead now.");
	} else {
		sc_start(&pl_sd->bl, SC_TRICKDEAD, 100, 0, 0);
		clif->message(pl_sd->fd, "You were forced to lie down by GM.");
		clif->message(fd, "Player is acting dead now.");
	}
	
	return true;
}

ACMD_FUNC(deadoff)
{
	struct map_session_data *pl_sd;
	struct s_mapiterator *iter;
	
	if (*message == NULL) {
		clif->message(fd,"Usage: @deadoff <CharacterName/all>");
		return false;
	}

	if (strcmpi(message, "all") != 0 (pl_sd = map->nick2sd(message)) == NULL && (pl_sd = map->charid2sd(atoi(message))) == NULL) {
		clif->message(fd, msg_txt(3));	// Character not found.
		return false;
	}

	if (pc_get_group_level(pl_sd) > pc_get_group_level(sd)) {
		clif->message(fd, "You cannot use this command on higher GM level.");
		return false;
	}

	if (strcmpi(message, "all") == 0) {
		iter = mapit_getallusers();
		for (pl_sd = BL_UCCAST(BL_PC, mapit->first(iter)); mapit->exists(iter); pl_sd = BL_UCCAST(BL_PC, mapit->next(iter))) {
			if (pl_sd->status.char_id == sd->status.char_id || sd->bl.m != pl_sd->bl.m)
				continue; 
			if (pc_get_group_level(pl_sd) > pc_get_group_level(sd)) {
				continue;
			}
			status_change_end(&pl_sd->bl, SC_TRICKDEAD, INVALID_TIMER);
			clif->message(pl_sd->fd, "You were forced to get up by GM.");
		}
		mapit->free(iter);
		clif->message(fd, "Command Succesfully Executed.");
	} else {
		status_change_end(&pl_sd->bl, SC_TRICKDEAD, INVALID_TIMER);
		clif->message(pl_sd->fd, "You were forced to get up by GM.");
		clif->message(fd, "Command Succesfully Executed.");
	}
	
	return true;
}

HPExport void plugin_init(void)
{
	addAtcommand("deadon", deadon);
	addAtcommand("deadoff", deadoff);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
