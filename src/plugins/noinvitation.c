//===== Hercules Plugin ======================================
//= noinvitation mapflag
//===== By: ==================================================
//= AnnieRuru
//===== Current Version: =====================================
//= 1.1
//===== Compatible With: ===================================== 
//= Hercules 2020-05-16
//===== Description: =========================================
//= block players from invite friends
//===== Topic ================================================
//= http://herc.ws/board/topic/11553-noinvitation-mapflag/
//===== Additional Comments: =================================  
//= this mapflag only blocks friend invitation
//= to block others like trade/party/guild invitation
//= use mf_notrade, mf_partylock, mf_guildlock
//============================================================

#include "common/hercules.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common/nullpo.h"
#include "common/memmgr.h"

#include "map/clif.h"
#include "map/pc.h"
#include "map/map.h"
#include "map/npc.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"noinvitation",
	SERVER_TYPE_MAP,
	"1.1",
	HPM_VERSION,
};

struct mapflag_data {
	unsigned noinvitation : 1;
	int group_bypass;
};

void npc_parse_unknown_mapflag_pre(const char **name, const char **w3, const char **w4, const char **start, const char **buffer, const char **filepath, int **retval)
{
	if (!strcmp(*w3, "noinvitation")) {
		int16 m = map->mapname2mapid(*name);
		struct mapflag_data *mf;
		int bypass_level = atoi(*w4);
		if (bypass_level == 0)
			bypass_level = 100;
		if (!(mf = getFromMAPD(&map->list[m], 0))) {
			CREATE(mf, struct mapflag_data, 1);
			addToMAPD(&map->list[m], mf, 0, true);
		}
		mf->group_bypass = bypass_level;
		mf->noinvitation = 1;
		hookStop();
	}
	return;
}

void clif_friendlist_req_pre(struct map_session_data **sd, int *account_id, int *char_id, const char **name)
{
	struct map_session_data *ssd = map->id2sd(*account_id);
	struct mapflag_data *mf;
	nullpo_retv(ssd);
	mf = getFromMAPD(&map->list[ssd->bl.m], 0);
	if (mf && mf->noinvitation && pc_get_group_level(ssd) < mf->group_bypass) {
		clif->messagecolor_self(ssd->fd, COLOR_RED, "You can't invite friends in this map.");
		hookStop();
	}
	return;
}

void map_flags_init_pre(void)
{
	int i;
	for (i = 0; i < map->count; ++i) {
		struct mapflag_data *mf = getFromMAPD(&map->list[i], 0);
		if (mf)
			removeFromMAPD(&map->list[i], 0);
	}
	return;
}

HPExport void plugin_init(void)
{
	addHookPre(npc, parse_unknown_mapflag, npc_parse_unknown_mapflag_pre);
	addHookPre(clif, friendlist_req, clif_friendlist_req_pre);
	addHookPre(map, flags_init, map_flags_init_pre);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
