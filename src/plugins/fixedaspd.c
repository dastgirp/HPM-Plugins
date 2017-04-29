//===== Hercules Plugin ======================================
//= fixedaspd mapflag
//===== By: ==================================================
//= AnnieRuru
//===== Current Version: =====================================
//= 1.2
//===== Compatible With: ===================================== 
//= Hercules 2016-06-30
//===== Description: =========================================
//= fixed players attack speed on particular map
//===== Topic ================================================
//= http://herc.ws/board/topic/11155-fixedaspd-mapflag/
//===== Additional Comments: =================================  
//= prontera   mapflag   fixedaspd   150
//= fixed players attack speed at 150 speed at prontera map
//============================================================

#include "common/hercules.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common/utils.h"
#include "common/memmgr.h"

#include "map/pc.h"
#include "map/npc.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"fixedaspd",
	SERVER_TYPE_MAP,
	"1.2",
	HPM_VERSION,
};

struct mapflag_data {
	int fixedaspd;
};

void npc_parse_unknown_mapflag_pre(const char **name, const char **w3, const char **w4, const char **start, const char **buffer, const char **filepath, int **retval)
{
	if (!strcmp(*w3,"fixedaspd")) {
		int16 m = map->mapname2mapid(*name);
		int fixedaspd = atoi(*w4);
		struct mapflag_data *mf = getFromMAPD(&map->list[m], 0);
		if (fixedaspd > 0) {
			fixedaspd = cap_value(fixedaspd, 85, 199);
			if (!mf) {
				CREATE(mf, struct mapflag_data, 1);
				addToMAPD(&map->list[m], mf, 0, true);
			}
			mf->fixedaspd = 2000 - fixedaspd *10;
		}
		else
			ShowWarning( "npc_parse_mapflag: Missing 4th param for 'fixedaspd' flag ! removing flag from %s in file '%s', line '%d'.\n",  *name, *filepath, strline(*buffer,(*start)-(*buffer)));
		hookStop();
	}
	return;
}
	
short status_calc_fix_aspd_post(short retVal, struct block_list *bl, struct status_change *sc, int aspd) {
	if (bl->type == BL_PC) {
		TBL_PC *sd = BL_CAST(BL_PC, bl);
		struct mapflag_data *mf = getFromMAPD(&map->list[sd->bl.m], 0);
		if (mf && mf->fixedaspd)
			return cap_value(mf->fixedaspd, 0, 2000);
	}
	return retVal;
}

void map_flags_init_pre(void)
{
	int i;
	for (i = 0; i < map->count; i++) {
		struct mapflag_data *mf = getFromMAPD(&map->list[i], 0);
		if (mf)
			removeFromMAPD(&map->list[i], 0);
	}
	return;
}

HPExport void plugin_init(void)
{
	addHookPre(npc, parse_unknown_mapflag, npc_parse_unknown_mapflag_pre);
	addHookPost(status, calc_fix_aspd, status_calc_fix_aspd_post);
	addHookPre(map, flags_init, map_flags_init_pre);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
