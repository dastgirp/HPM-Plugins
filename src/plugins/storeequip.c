/*
=============================================
@storeequip Plugin
================================================
v 1.0 Initial Release
Stores all item except equipment
*/
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

#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"@storeequip",			// Plugin name
	SERVER_TYPE_MAP,	// Which server types this plugin works with?
	"1.0",				// Plugin version
	HPM_VERSION,		// HPM Version (don't change, macro is automatically updated)
};

bool store_all_equip(struct map_session_data *sd) {
	int fd;
	
	if (sd == NULL)
		return false;
	
	fd = sd->fd;

	if (sd->npc_id) {
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
			if(sd->status.inventory[i].equip)
				storage->add(sd,  i, sd->status.inventory[i].amount);
		}
	}
	storage->close(sd);
}

/*==========================================
 * @storeequip
 * Put everything equipped into storage.
 *------------------------------------------*/
ACMD(storeequip) {

	int i;
	
	if (store_all_equip(sd) == false)
		return false;

	clif->message(fd, "Stored All Items.");
	return true;
}

BUILDIN(storeequip) {
	TBL_PC *sd;

	sd = script->rid2sd(st);
	if (store_all_equip(sd) == false)
		script_pushint(st, 0);
	return true;
}


/* run when server starts */
HPExport void plugin_init (void) {
    addAtcommand("storeequip",storeequip);
	addScriptCommand("storeequip","",storeequip);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}
