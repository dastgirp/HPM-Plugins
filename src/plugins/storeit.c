/*
=============================================
@storeit Plugin
Converted by: Dastgir
Original Made by: Akinari
================================================
v 1.0 Initial Release
Stores all item except equipment
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/HPMi.h"
#include "../common/mmo.h"
#include "../map/clif.h"
#include "../map/atcommand.h"
#include "../map/storage.h"
#include "../map/pc.h"

#include "../common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"@storeit",			// Plugin name
	SERVER_TYPE_MAP,	// Which server types this plugin works with?
	"1.0",				// Plugin version
	HPM_VERSION,		// HPM Version (don't change, macro is automatically updated)
};

/*==========================================
 * @storeit
 * Put everything not equipped into storage.
 *------------------------------------------*/
ACMD(storeit) {

	int i;
	
	if (!sd) return false;

	if (sd->npc_id)
	{
		clif->message(fd, "You cannot be talking to an NPC and use this command.");
		return -1;
	}

	if (sd->state.storage_flag != 1)
  	{	//Open storage.
		switch (storage->open(sd)) {
		case 2: //Try again
			clif->message(fd, "Run this command again..");
			return true;
		case 1: //Failure
			clif->message(fd, "You can't open the storage currently.");
			return true;
		}
	}
	for (i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].amount) {
			if(sd->status.inventory[i].equip == 0)
				storage->add(sd,  i, sd->status.inventory[i].amount);
		}
	}
	storage->close(sd);

	clif->message(fd, "Stored All Items.");
	return true;
}


/* run when server starts */
HPExport void plugin_init (void) {
	storage = GET_SYMBOL("storage");
	clif = GET_SYMBOL("clif");
	atcommand = GET_SYMBOL("atcommand");
	pc = GET_SYMBOL("pc");
	
    addAtcommand("storeit",storeit);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}
