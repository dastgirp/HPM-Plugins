//===== Hercules Plugin ======================================
//= Store Equipments
//===== By: ==================================================
//= by Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Stores All Equipments
//===== Changelog: ===========================================
//= v1.0 - Initial Release
//===== Additional Comments: =================================
//= AtCommand: @storeequip
//= ScriptCommand: storeequip();
//===== Repo Link: ===========================================
//= https://github.com/dastgir/HPM-Plugins
//============================================================

#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/HPMi.h"
#include "common/mmo.h"
#include "map/clif.h"
#include "map/atcommand.h"
#include "map/storage.h"
#include "map/pc.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"Store Equipments(storeequip)",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

bool store_all_equip(struct map_session_data *sd) {
	int fd, i;
	
	if (sd == NULL)
		return false;
	
	fd = sd->fd;

	if (sd->npc_id > 0) {
		clif->message(fd, "You cannot be talking to an NPC and use this command.");
		return false;
	}

	if (sd->state.storage_flag != 1) {
		switch (storage->open(sd)) {
			case 2: //Try again
				clif->message(fd, "Failed to Open Storage, Try Again..");
				return false;
			case 1: //Failure
				clif->message(fd, "You can't open the storage currently.");
				return false;
		}
	}
	for (i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].amount) {
			if (sd->status.inventory[i].equip) {
				storage->add(sd,  i, sd->status.inventory[i].amount);
			}
		}
	}
	storage->close(sd);
	return true;
}

/*==========================================
 * @storeequip
 * Put everything equipped into storage.
 *------------------------------------------*/
ACMD(storeequip) {	
	if (!store_all_equip(sd))
		return false;

	clif->message(fd, "Stored All Items.");
	return true;
}

BUILDIN(storeequip) {
	struct map_session_data *sd = script->rid2sd(st);

	if (!store_all_equip(sd))
		script_pushint(st, 0);
	else
		script_pushint(st, 1);

	return true;
}


/* run when server starts */
HPExport void plugin_init(void)
{
	addAtcommand("storeequip", storeequip);
	addScriptCommand("storeequip", "", storeequip);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
