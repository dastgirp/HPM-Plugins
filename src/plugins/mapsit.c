//===== Hercules Plugin ======================================
//= @mapsit
//===== By: ==================================================
//= Cretino
//= Cnverted to Plugin by Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Various commands to make player force sit.
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
#include "common/utils.h"

#include "map/atcommand.h"
#include "map/clif.h"
#include "map/map.h"
#include "map/pc.h"
#include "map/status.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo =
{
	"@mapsit",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

struct player_data {
	bool sitting;
};

struct player_data* pd_search(struct map_session_data *sd)
{
	struct player_data *data;
	if ((data = getFromMSD(sd, 0)) == NULL) {
		CREATE(data, struct player_data, 1);
		data->sitting = false;
		addToMSD(sd, data, 0, true);
	}
	return data;
}

void set_sitting(struct map_session_data *sd, bool value)
{
	struct player_data *data = NULL;
	if (sd == NULL) {
		return;
	}
	data = pd_search(sd);
	data->sitting = value;
	return;
}

bool get_sitting(struct map_session_data *sd)
{
	struct player_data *data = NULL;
	if (sd == NULL) {
		return false;
	}
	data = pd_search(sd);
	return data->sitting;
}

void pc_sit(struct map_session_data *sd, int type) {
	switch(type) {
	case 0:
		set_sitting(sd, 0);
		pc->setstand(sd);
		skill->sit(sd, 0);
		clif->standing(&sd->bl);
		break;
	
	case 1:
		if (!pc_issit(sd)) {
			set_sitting(sd, 1);
			pc_setsit(sd);
			skill->sit(sd, 1);
			clif->sitting(&sd->bl);
		} else if (pc_issit(sd) && !get_sitting(sd)) {
			set_sitting(sd, 1);
		}
		break;
	
	default:
		ShowError("pc_sit: invalid type(%d).\n", type);
		break;
	}
}

ACMD(mapsit)
{
	struct s_mapiterator* iter;
	int level = 0;
	struct map_session_data *pl_sd = NULL;
	
	iter = mapit_getallusers();
	for (pl_sd = BL_UCAST(BL_PC, mapit->first(iter)); mapit->exists(iter); pl_sd = BL_UCAST(BL_PC, mapit->next(iter))) {
		if (pl_sd->bl.m != sd->bl.m) {
			continue;
		}
		
		if (pl_sd->status.char_id == sd->status.char_id) {
			continue;
		}
		
		if (pl_sd != NULL && pc_get_group_level(pl_sd) > pc_get_group_level(sd)) {
			clif->message(fd, msg_txt(81));
			continue;
		}
		
		if (pc_issit(pl_sd) && get_sitting(pl_sd)) {
			continue;
		}
		
		if (sscanf(message, "%d", &level) < 1) {
			pc_sit(pl_sd, 1);
		} else {
			if (pc_get_group_level(pl_sd) < level) {
				pc_sit(pl_sd, 1);
			}
		}
	}
	
	mapit->free(iter);
	clif->message(fd, "All Players in the map sat.");
	
	return true;
}

ACMD(mapstand)
{
	struct s_mapiterator* iter;
	int level = 0;
	struct map_session_data *pl_sd = NULL;
	
	iter = mapit_getallusers();
	for (pl_sd = BL_UCAST(BL_PC, mapit->first(iter)); mapit->exists(iter); pl_sd = BL_UCAST(BL_PC, mapit->next(iter))) {
		if (pl_sd->bl.m != sd->bl.m) {
			continue;
		}
		
		if (pl_sd->status.char_id == sd->status.char_id) {
			continue;
		}
		
		if (pl_sd != NULL && pc_get_group_level(pl_sd) > pc_get_group_level(sd)) {
			clif->message(fd, msg_txt(81));
			continue;
		}
		
		if (!pc_issit(pl_sd)) {
			continue;
		}
		
		if (sscanf(message, "%d", &level) < 1) {
			pc_sit(pl_sd, 0);
		} else {
			level = level > 99 ? 99 : level;
			if (pc_get_group_level(pl_sd) < level) {
				pc_sit(pl_sd, 0);
			} else {
				continue;
			}
		}
	}
	
	mapit->free(iter);
	clif->message(fd, "All Players are standing now.");
	
	return true;
}

ACMD(sit)
{
	struct map_session_data *pl_sd = NULL;
	char output[128];	

	if (!*message) {
		clif->message(fd, "Usage: @sit <Character Name/ID>.");
		return false;
	}

	if ((pl_sd = map->nick2sd(message, true)) == NULL && (pl_sd = map->charid2sd(atoi(message))) == NULL) {
		clif->message(fd, msg_txt(3));
		return false;
	}
		
	if (pl_sd == sd) {
		clif->message(fd, "You cannot use command on yourself.");
		return false;
	}

	if (pl_sd != NULL && pc_get_group_level(pl_sd) > pc_get_group_level(sd)) {
		clif->message(fd, msg_txt(81));
		return false;
	}
	
	if (pc_issit(pl_sd) && get_sitting(pl_sd)) {
		sprintf(output, "Player '%s' is already sitting.", pl_sd->status.name);
		clif->message(fd, output);
		return false;
	}
	
	pc_sit(pl_sd, 1);
	sprintf(output, "Player '%s' sat.", pl_sd->status.name);
	clif->message(fd, output);
	
	return true;
}

ACMD(stand)
{
	struct map_session_data *pl_sd = NULL;
	char output[128];

	if (!*message) {
		clif->message(fd, "Usage: @stand <Character Name/ID>");
		return false;
	}

	if ((pl_sd = map->nick2sd(message, true)) == NULL && (pl_sd = map->charid2sd(atoi(message))) == NULL) {
		clif->message(fd, msg_txt(3));
		return false;
	}
		
	if (pl_sd == sd) {
		clif->message(fd, "You cannot use command on yourself.");
		return false;
	}

	if (pl_sd != NULL && pc_get_group_level(pl_sd) > pc_get_group_level(sd)) {
		clif->message(fd, msg_txt(81));
		return false;
	}
	
	if (!pc_issit(pl_sd)) {
		sprintf(output, "Player '%s' is already standing.", pl_sd->status.name);
		clif->message(fd, output);
		return false;
	}
	
	pc_sit(pl_sd, 0);
	sprintf(output, "Player '%s' was forced to stand.", pl_sd->status.name);
	clif->message(fd, output);
	
	return true;
}

/**
 * mapsit "<Map>",{<Type>{,<Level>}};
 */
BUILDIN(mapsit)
{
	struct map_session_data *pl_sd = NULL;
	struct s_mapiterator *iter;
	int mapid;
	const char *mapname;
	int type = script_hasdata(st, 3) ? script_getnum(st, 3) : 0;
	int level = script_hasdata(st, 4) ? script_getnum(st, 4) : 0;
	
	mapname = script_getstr(st, 2);
	
	type = cap_value(type, 0, 1);
	
	if ((mapid = map->mapname2mapid(mapname)) < 0){
		ShowError("buildin_mapsit: invalid map name(%s).\n", mapname);
		return false;
	}
	
	iter = mapit_getallusers();
	
	for (pl_sd = BL_UCAST(BL_PC, mapit->first(iter)); mapit->exists(iter); pl_sd = BL_UCAST(BL_PC, mapit->next(iter))) {
		if (pl_sd->bl.m != mapid) {
			continue;
		}
		
		if (type && pc_issit(pl_sd) && get_sitting(pl_sd)) {
			continue;
		}
		
		if (!type && !pc_issit(pl_sd)) {
			continue;
		}
		
		if (script_hasdata(st, 4)) {
			level = level > 99 ? 99 : level;
			if (pc_get_group_level(pl_sd) < level) {
				pc_sit(pl_sd, type);
			} else {
				continue;
			}
		} else {
			pc_sit(pl_sd, type);
		}
	}
	
	mapit->free(iter);
	
	return true;
}

/*==========================================
 * sit "Player Name",<type>;
 *------------------------------------------*/
BUILDIN(sit)
{
	struct map_session_data *pl_sd = map->nick2sd(script_getstr(st, 2), true);
	int type = script_getnum(st, 3);
	
	type = cap_value(type, 0, 1);
	
	if (pl_sd == NULL) {
		ShowWarning("buildin_sit: Invalid player.\n");
		return false;
	}
	
	if (type && pc_issit(pl_sd) && get_sitting(pl_sd)) {
		return false;
	}
	
	if (!type && !pc_issit(pl_sd)) {
		return false;
	}

	pc_sit(pl_sd, type);
	
	return true;
}

HPExport void plugin_init(void)
{
	addAtcommand("mapsit", mapsit);
	addAtcommand("mapstand", mapstand);
	addAtcommand("sit", sit);
	addAtcommand("stand", stand);
	addScriptCommand("sit", "si", sit);
	addScriptCommand("mapsit", "s??", mapsit);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
