//===== Hercules Plugin ======================================
//= TraderMod
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Exposes item list to script.
//= You can use variable @bought_nameid and @bought_quantity
//= as an array on OnPayFunds:
//= Only works with NST_CUSTOM
//===== Changelog: ===========================================
//= v1.0 - Initial Release
//===== Repo Link: ===========================================
//= https://github.com/dastgir/HPM-Plugins
//============================================================

#include "common/hercules.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "common/HPMi.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/memmgr.h"
#include "common/strlib.h"
#include "common/nullpo.h"
#include "common/timer.h"

#include "map/battle.h"
#include "map/script.h"
#include "map/pc.h"
#include "map/clif.h"
#include "map/status.h"
#include "map/npc.h"
#include "map/mob.h"
#include "map/map.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {   // [Dastgir/Hercules]
	"TraderMod",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

struct trader_item_data {
	struct itemlist item_list;
	bool init;
};

struct trader_item_data* tid_search(struct map_session_data* sd)
{
	struct trader_item_data *data;
	if ((data = getFromMSD(sd,0)) == NULL) {
		CREATE(data, struct trader_item_data, 1);
		data->init = false;
		addToMSD(sd, data, 0, true);
	}
	return data;
}

int npc_cashshop_buylist_pre(struct map_session_data **sd_, int *points_, struct itemlist **item_list_)
{
	struct map_session_data *sd = *sd_;
	struct itemlist *item_list = *item_list_;
	struct trader_item_data *tid_data;
	struct npc_data *nd = NULL;
	int i;

	nullpo_retr(ERROR_TYPE_SYSTEM, sd);
	nullpo_retr(ERROR_TYPE_SYSTEM, item_list);

	nd = map->id2nd(sd->npc_shopid);
	if (nd == NULL)
		return ERROR_TYPE_NPC;

	// Check if it is trader
	if (nd->subtype != SCRIPT || nd->u.scr.shop->type != NST_CUSTOM)
		return ERROR_TYPE_NONE;	// Return type does not matter, there's no hookStop

	tid_data = tid_search(sd);

	if (tid_data->init == true)
		VECTOR_CLEAR(tid_data->item_list);

	VECTOR_INIT(tid_data->item_list);
	tid_data->init = true;

	VECTOR_ENSURE(tid_data->item_list, VECTOR_LENGTH(*item_list), 1);
	for (i = 0; i < VECTOR_LENGTH(*item_list); i++) {
		VECTOR_PUSH(tid_data->item_list, VECTOR_INDEX(*item_list, i));
	}
	
	return ERROR_TYPE_NONE;	
}

int npc_cashshop_buy_pre(struct map_session_data **sd_, int *nameid, int *amount, int *points)
{
	struct map_session_data *sd = *sd_;
	struct trader_item_data *tid_data;
	struct npc_data *nd = NULL;
	struct itemlist_entry entry = { 0 };

	nullpo_retr(ERROR_TYPE_SYSTEM, sd);

	nd = map->id2nd(sd->npc_shopid);
	if (nd == NULL)
		return ERROR_TYPE_NPC;

	if (nd->subtype != SCRIPT || nd->u.scr.shop->type != NST_CUSTOM)
		return ERROR_TYPE_NONE;	// Return type does not matter, there's no hookStop

	tid_data = tid_search(sd);

	if (tid_data->init == true)
		VECTOR_CLEAR(tid_data->item_list);

	VECTOR_INIT(tid_data->item_list);
	tid_data->init = true;

	VECTOR_ENSURE(tid_data->item_list, 1, 1);
	
	entry.amount = *amount;
	entry.id = *nameid;
	VECTOR_PUSH(tid_data->item_list, entry);

	return ERROR_TYPE_NONE;	
}

bool npc_trader_pay_pre(struct npc_data **nd, struct map_session_data **sd_, int *price, int *points)
{
	struct trader_item_data *tid_data;
	int key_nameid = 0;
	int key_amount = 0;
	int i;
	struct map_session_data *sd = *sd_;

	nullpo_retr(false, *nd);
	nullpo_retr(false, sd);

	tid_data = tid_search(sd);

	// discard old contents
	script->cleararray_pc(sd, "@bought_nameid", (void*)0);
	script->cleararray_pc(sd, "@bought_quantity", (void*)0);

	// save list of bought items
	for (i = 0; i < VECTOR_LENGTH(tid_data->item_list); i++) {
		struct itemlist_entry *entry = &VECTOR_INDEX(tid_data->item_list, i);
		intptr_t nameid = entry->id;
		intptr_t amount = entry->amount;
		script->setarray_pc(sd, "@bought_nameid", i, (void *)nameid, &key_nameid);
		script->setarray_pc(sd, "@bought_quantity", i, (void *)amount, &key_amount);
	}

	if (tid_data->init == true) {
		VECTOR_CLEAR(tid_data->item_list);
		tid_data->init = false;
	}

	return true; // doesn't matter
}


HPExport void plugin_init(void)
{
	addHookPre(npc, cashshop_buylist, npc_cashshop_buylist_pre);	
	addHookPre(npc, cashshop_buy, npc_cashshop_buy_pre);
	addHookPre(npc, trader_pay, npc_trader_pay_pre);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
