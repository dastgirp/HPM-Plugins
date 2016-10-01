//===== Hercules Plugin ======================================
//= @pk command
//===== By: ==================================================
//= AnnieRuru
//===== Current Version: =====================================
//= 1.1
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

#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"@pk",
	SERVER_TYPE_MAP,
	"1.1",
	HPM_VERSION,
};

int config_delay = 5; // After turn pk on/off, how many seconds delay before the player allow to pk on/off ?

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
	if (!map->list[sd->bl.m].flag.town) {
		clif->message(sd->fd, "You can only change your PK state in towns.");
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
		struct player_data *src_pc = getFromMSD(BL_CAST(BL_PC, src), 0 );
		struct player_data *target_pc = getFromMSD(BL_CAST(BL_PC, target), 0 );
		if (src_pc != NULL && target_pc != NULL)
			if (src_pc->pkmode && target_pc->pkmode)
				return 1;
	}
	return retVal;
}

HPExport void plugin_init (void)
{
	addAtcommand("pk", pk);
	addHookPost(battle, check_target, battle_check_target_post);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
