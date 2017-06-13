//===== Hercules Plugin ======================================
//= @pk command
//===== By: ==================================================
//= AnnieRuru (v1.1)
//===== Modified By: =========================================
//= Dastgir
//===== Current Version: =====================================
//= v1.3
//===== Compatible With: ===================================== 
//= Hercules
//===== Description: =========================================
//= PK Mode
//===== Topic ================================================
//= http://herc.ws/board/topic/11004-/
//============================================================

#include "common/hercules.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "map/atcommand.h"
#include "map/battle.h"
#include "map/clif.h"
#include "map/map.h"
#include "map/pc.h"

#include "common/memmgr.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"@pk",
	SERVER_TYPE_MAP,
	"1.3",
	HPM_VERSION,
};

// bitwise checking
enum {
	PK_ENABLE_TOWN = 1,
	PK_ENABLE_OTHER = 2,
	PK_ENABLE_ALL = 3,
};

int config_delay = 5; // After turn pk on/off, how many seconds delay before the player allow to pk on/off ?

// maps where @pk command can be used
int enable_maps = PK_ENABLE_TOWN;
// Which map player can actually pk
int pk_maps = PK_ENABLE_OTHER;

struct player_data {
	unsigned int pkmode :1;
	int pkmode_delay;
};

ACMD(pk)
{
	struct player_data *ssd;
	char output[CHAT_SIZE_MAX];
	if (!( ssd = getFromMSD(sd,0))) {
		CREATE(ssd, struct player_data, 1);
		ssd->pkmode = 0;
		addToMSD(sd, ssd, 0, true);
	}
	if (enable_maps&PK_ENABLE_TOWN && !map->list[sd->bl.m].flag.town) {
		clif->message(sd->fd, "You can only change your PK state in towns.");
		return false;
	}
	if (enable_maps&PK_ENABLE_OTHER && map->list[sd->bl.m].flag.town) {
		clif->message(sd->fd, "You cannot change your PK state in towns.");
		return false;
	}
	if (ssd->pkmode_delay + config_delay > (int)time(NULL)) {
		safesnprintf(output, CHAT_SIZE_MAX, "You must wait %d seconds before using this command again.", ssd->pkmode_delay + config_delay - (int)time(NULL));
		clif->message(sd->fd, output);
		return false;
	}
	if (!ssd->pkmode) {
		ssd->pkmode = 1;
		clif->message(sd->fd, "Your PK state is now ON");
	} else {
		ssd->pkmode = 0;
		clif->message(sd->fd, "Your PK state is now OFF");
	}
	ssd->pkmode_delay = (int)time(NULL);
	return true;
}

int battle_check_target_post(int retVal, struct block_list *src, struct block_list *target, int flag)
{
	if (retVal != 1 && src->type == BL_PC && target->type == BL_PC) {
		struct map_session_data *sd = BL_CAST(BL_PC, src);
		struct map_session_data *targetsd = BL_CAST(BL_PC, target);

		// Can only PK in town
		if (pk_maps&PK_ENABLE_TOWN && !map->list[sd->bl.m].flag.town) {
			return retVal;
		}
		// Can only pk in fields
		if (pk_maps&PK_ENABLE_OTHER && map->list[sd->bl.m].flag.town) {
			return retVal;
		}

		if (sd->status.account_id != targetsd->status.account_id) {
			struct player_data *src_pc = getFromMSD(sd, 0);
			struct player_data *target_pc = getFromMSD(targetsd, 0);
			if (src_pc != NULL && target_pc != NULL && src_pc->pkmode && target_pc->pkmode) {
				hookStop();
				return 1;
			}
		}
	}
	return retVal;
}

HPExport void plugin_init(void)
{
	addAtcommand("pk", pk);
	addHookPost(battle, check_target, battle_check_target_post);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
