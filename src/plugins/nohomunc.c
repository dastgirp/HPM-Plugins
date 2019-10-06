//===== Hercules Plugin ======================================
//= Disable homunculus mapflag
//===== By: ==================================================
//= Originally by Samuel[Hercules]
//= Modified by Dastgir[Hercules]
//===== Current Version: =====================================
//= 1.1
//===== Compatible With: ===================================== 
//= Hercules/RagEmu
//===== Description: =========================================
//= Disable Homunculus in a certain map
//===== Usage: ===============================================
//= alberta <tab> mapflag <tab> nohomunc
//============================================================


#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/utils.h"
#include "common/memmgr.h"
#include "common/nullpo.h"
#include "common/random.h"

#include "map/pc.h"
#include "map/npc.h"
#include "map/intif.h"
#include "map/homunculus.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {	
	"nohomunc mapflag",
	SERVER_TYPE_MAP,
	"1.1",
	HPM_VERSION,
};

struct mapflag_data {
	unsigned nohomunc : 1;
};

void npc_parse_unknown_mapflag_pre(const char **name, const char **w3, const char **w4, const char **start, const char **buffer, const char **filepath, int **retval)
{
	if (strcmpi(*w3,"nohomunc") == 0) {
		int16 m = map->mapname2mapid(*name);
		struct mapflag_data *mf;
		if (( mf = getFromMAPD(&map->list[m], 0)) == NULL) {
			CREATE(mf, struct mapflag_data, 1);
			addToMAPD(&map->list[m], mf, 0, true);
		}
		mf->nohomunc = 1;
		hookStop();
	}
	return;
}
	
void clif_parse_LoadEndAck_post(int fd, struct map_session_data *sd)
{
	nullpo_retv(sd);

	if (sd->hd != NULL && homun_alive(sd->hd)) {
		struct mapflag_data *mf;
		mf = getFromMAPD(&map->list[sd->bl.m], 0);
		if (mf != NULL && mf->nohomunc == 1) {
			homun->vaporize(sd, HOM_ST_REST, true);
			clif->messagecolor_self(sd->fd, COLOR_RED, "You cannot spawn homunculus here.");
			hookStop();
		}
	}
	return;
}

bool homunculus_call_post(bool retVal, struct map_session_data *sd)
{
	struct mapflag_data *mf;
	if (!retVal || sd == NULL)
		return retVal;
	
	mf = getFromMAPD(&map->list[sd->bl.m], 0);

	if (mf != NULL && mf->nohomunc == 1 && sd->status.hom_id > 0) {
		homun->vaporize(sd, HOM_ST_REST, true);
		clif->messagecolor_self(sd->fd, COLOR_RED, "You can't spawn homunculus here.");
		hookStop();
	}
	return retVal;
}

bool homunculus_create_post(bool retVal, struct map_session_data *sd, const struct s_homunculus *hom, bool is_new)
{
	struct homun_data *hd;
	struct mapflag_data *mf;

	if (!retVal || sd == NULL || hom == NULL)
		return false;

	hd = sd->hd;
	mf = getFromMAPD(&map->list[sd->bl.m], 0);
	if (hd != NULL && mf != NULL && mf->nohomunc > 0) {
		homun->vaporize(sd, HOM_ST_REST, true);
		clif->messagecolor_self(sd->fd, COLOR_RED, "You can't spawn homunculus here.");
		hookStop();
		return false;
	}
	return retVal;
}

void map_flags_init_pre(void)
{
	int i;
	for (i = 0; i < map->count; i++) {
		struct mapflag_data *mf = getFromMAPD(&map->list[i], 0);
		if (mf != NULL)
			removeFromMAPD(&map->list[i], 0);
	}
	return;
}

HPExport void plugin_init (void) {
	addHookPre(npc, parse_unknown_mapflag, npc_parse_unknown_mapflag_pre);
	addHookPost(clif, pLoadEndAck, clif_parse_LoadEndAck_post);
	addHookPost(homun, call, homunculus_call_post);
	addHookPost(homun, create, homunculus_create_post);
	addHookPre(map, flags_init, map_flags_init_pre);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
