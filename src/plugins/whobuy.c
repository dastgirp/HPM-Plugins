/*
=============================================
@whobuy plugin
Author: Dastgir/Hercules
=============================================
v 1.0 Initial Release
*/

#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/HPMi.h"
#include "map/atcommand.h"
#include "map/battle.h"
#include "map/clif.h"
#include "map/itemdb.h"
#include "map/map.h"
#include "map/pc.h"
#include "map/script.h"
#include "common/mapindex.h"
#include "common/nullpo.h"
#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"


HPExport struct hplugin_info pinfo = {
	"whobuy",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

ACMD(whobuy)
{
	char item_name[100];
	int item_id, j, count = 0, sat_num = 0;
	bool flag = 0;		// Show Location on minimap?
	struct map_session_data* pl_sd;
	struct s_mapiterator* iter;
	unsigned int MinPrice = battle->bc->vending_max_value, MaxPrice = 0;
	struct item_data *item_data;
	char output[256];

	memset(item_name, '\0', sizeof(item_name));

	if (!*message || sscanf(message, "%99[^\n]", item_name) < 1) {
		clif->message(fd, "Input item name or ID (use: @whobuy <name or ID>).");
		return false;
	}
	if ((item_data = itemdb->search_name(item_name)) == NULL &&
		(item_data = itemdb->exists(atoi(item_name))) == NULL) {
		clif->message(fd, msg_txt(19)); // Invalid item ID or name.
		return false;
	}

	item_id = item_data->nameid;

	iter = mapit_getallusers();
	for (pl_sd = BL_UCAST(BL_PC, mapit->first(iter)); mapit->exists(iter); pl_sd = BL_UCAST(BL_PC, mapit->next(iter))) {
		if (sd == pl_sd)
			continue;
		if (pl_sd->state.buyingstore) {	// Check if player is autobuying
			for (j = 0; j < pl_sd->buyingstore.slots; j++) {
				if(pl_sd->buyingstore.items[j].nameid == item_id) {
					snprintf(output, CHAT_SIZE_MAX, "Price %d | Amount %d | Buyer %s | Map %s[%d,%d]", pl_sd->buyingstore.items[j].price, pl_sd->buyingstore.items[j].amount, pl_sd->status.name, mapindex_id2name(pl_sd->mapindex), pl_sd->bl.x, pl_sd->bl.y);
					if (pl_sd->buyingstore.items[j].price < MinPrice)
						MinPrice = pl_sd->buyingstore.items[j].price;
					if (pl_sd->buyingstore.items[j].price > MaxPrice)
						MaxPrice = pl_sd->buyingstore.items[j].price;
					clif->message(fd, output);
					count++;
					flag = 1;
				}
			}
			if (flag && pl_sd->mapindex == sd->mapindex) {
				clif->viewpoint(sd, 1, 1, pl_sd->bl.x, pl_sd->bl.y, ++sat_num, 0xFFFFFF);
				flag = 0;
			}
		}
	}
	mapit->free(iter);

	if (count > 0) {
		snprintf(output, CHAT_SIZE_MAX, "Found %d ea. Prices from %udz to %udz", count, MinPrice, MaxPrice);
		clif->message(fd, output);
	} else
		clif->message(fd, "Nobody buying it now.");

	return true;
}

/* Server Startup */
HPExport void plugin_init (void)
{
	addAtcommand("whobuy", whobuy);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}
