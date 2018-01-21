//===== Hercules Plugin ======================================
//= Restock Plugin
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 2.0
//===== Compatible With: ===================================== 
//= Hercules
//===== Description: =========================================
//= Enables Restock of Items.
//===== Additional Information: ==============================
//= Requires "Restock.txt" to be loaded too.
//============================================================
/*

CREATE TABLE IF NOT EXISTS `restock` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `char_id` int(11) NOT NULL,
  `item_id` int(11) NOT NULL,
  `quantity` int(11) NOT NULL,
  `restock_from` int(11) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `char_id` (`char_id`,`item_id`,`quantity`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
*/

#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/HPMi.h"
#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/strlib.h"
#include "common/sql.h"
#include "common/utils.h"
#include "common/nullpo.h"

#include "map/mob.h"
#include "map/map.h"
#include "map/clif.h"
#include "map/pc.h"
#include "map/script.h"
#include "map/elemental.h"
#include "map/npc.h"
#include "map/status.h"
#include "map/storage.h"
#include "map/itemdb.h"
#include "map/guild.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"Restock System",
	SERVER_TYPE_MAP,
	"2.0",
	HPM_VERSION,
};

struct restock_item {
	int id;
	unsigned short quantity;
	short restock_from;
};

struct restock_data {
	int idx;
	bool enabled;
	VECTOR_DECL(struct restock_item) restock;
};

enum { // restock_from
	RS_STORAGE = 0,
	RS_GSTORAGE,
	RS_MAX,
};

enum {
	RS_TYPE_ADD = 0,
	RS_TYPE_DEL,
	RS_TYPE_DEL_ALL,
	RS_TYPE_MAX,
};

bool rs_exists(struct map_session_data *sd)
{
	return (getFromMSD(sd, 0) == NULL) ? false : true;
}


struct restock_data* rs_search(struct map_session_data *sd, bool create)
{
	struct restock_data *data = NULL;
	if ((data = getFromMSD(sd, 0)) == NULL && create) {
		CREATE(data, struct restock_data, 1);
		data->idx = -1;
		data->enabled = false;
		addToMSD(sd, data, 0, true);
	}
	return data;
}

int pc_restock_misc_pre(struct map_session_data **sd, int *n, int *amount, int *type, short *reason, e_log_pick_type* log_type)
{
	int index = *n;
	struct restock_data *rsd;
	if (*sd == NULL)
		return 1;

	rsd = rs_search(*sd, false);

	if (rsd != NULL && rsd->enabled) {
		if ((*sd)->status.inventory[index].nameid > 0){
			int i, id = (*sd)->status.inventory[index].nameid;
			ARR_FIND(0, VECTOR_LENGTH(rsd->restock), i, VECTOR_INDEX(rsd->restock, i).id == id);

			if (i != VECTOR_LENGTH(rsd->restock)) {
				rsd->idx = i;
			}
		}
	}
	return 0;
}


int pc_restock_misc_post(int retVal, struct map_session_data *sd, int n, int amount, int type, short reason, e_log_pick_type log_type)
{
	struct restock_data *rsd;
	if (retVal == 1)
		return retVal;

	rsd = rs_search(*sd, false);

	if (rsd != NULL && rsd->enabled && rsd->idx > -1) {
		struct restock_item rsi = VECTOR_INDEX(rsd->restock, i);
		if (pc->search_inventory(sd, rsi.id) < rsi.quantity) {
			restock(sd, rsi.id, rsi.quantity, rsi.restock_from);
		}
		rsd->idx = -1;
	}
	return retVal;
}


const char* restock_from_name(int rs_from)
{
	switch (rs_from) {
		case RS_STORAGE:
			return "Storage";
		case RS_GSTORAGE:
			return "GStorage";
	}
	return "Unknown";
}

bool resotck_add_item(struct map_session_data *sd, int item_id, int quantity, int rs_from)
{
	int i;
	bool first_time = false;
	struct restock_data *rsd = NULL;
	struct restock_item *rsi = NULL;

	if (!rs_exists(sd)) {
		first_time = true;
	}
	rsd = rs_search(sd, true);

	if (first_time) {
		VECTOR_INIT(rsd->restock);
	}

	ARR_FIND(0, VECTOR_LENGTH(rsd->restock), i, VECTOR_INDEX(rsd->restock, i).id == item_id);
	// New restock id.
	if (i == VECTOR_LENGTH(rsd->restock)) {
		VECTOR_PUSHZEROED(rsd->restock, 1, 1);
		rsi = &VECTOR_LAST(rsd->restock);
	} else {
		rsi = &VECTOR_INDEX(rsd->restock, i);
	}
	rsi->id = item_id;
	rsi->quantity = quantity;
	rsi->restock_from = rs_from;

	if (SQL_ERROR == SQL->Query(map->mysql_handle,
		"INSERT INTO `restock` (`char_id`, `item_id`, `quantity`, `restock_from`) VALUES ('%d', '%d', '%d', '%d') 
			ON DUPLICATE KEY UPDATE `quantity`='%d', `restock_from`='%d'",
								sd->status.char_id,
								item_id,
								quantity,
								rs_from,
								quantity,
								rs_from
								))
		Sql_ShowDebug(map->mysql_handle);
}

bool restock(struct map_session_data *sd, int item_id, int quantity, int rs_from)
{
	int i, j, flag;
	struct item item_tmp;
	bool pushed_one = false;

	if (sd == NULL) {
		return false;
	}
	switch (rs_from) {
		case RS_GSTORAGE: {
			struct guild *g;
			struct guild_storage *gstorage2;
			g = guild->search(sd->status.guild_id);
			if (g == NULL) {
				clif->message(sd->fd, msg_txt(43));
				return false;
			}
			gstorage2 = gstorage->ensure(sd->status.guild_id);
			if (gstorage == NULL) {// Doesn't have opened @gstorage yet, so we skip the deletion since *shouldn't* have any item there.
				script_pushint(st, 0);
				return false;
			}
			j = gstorage2->storage_amount;
			gstorage2->lock = 1; 
			for (i = 0; i < j; ++i) {
				if (gstorage2->items[i].nameid == item_id && gstorage2->items[i].amount >= quantity) {
					memset(&item_tmp, 0, sizeof(item_tmp));
					item_tmp.nameid = gstorage2->items[i].nameid;
					item_tmp.identify = 1;
					gstorage->delitem(sd, gstorage2, i, quantity);
					if ((flag = pc->additem(sd,&item_tmp,quantity,LOG_TYPE_STORAGE))) {
						clif->additem(sd, 0, 0, flag);
						gstorage->close(sd);
						gstorage2->lock = 0;
						pushed_one = true;
						break;
					}
				}
			}
			gstorage->close(sd);
			gstorage2->lock = 0;
			break;
		}

		case RS_STORAGE: {
			struct storage_data* stor = &sd->status.storage;
			if (stor == NULL) {
				return false;
			}
			j = stor->storage_amount;
			if (sd->state.storage_flag) {
				sd->state.storage_flag = 0;
				storage->close(sd);
			}
			sd->state.storage_flag = 1;
			for (i = 0; i < j; ++i) {
				if (stor->items[i].nameid == item_id && stor->items[i].amount >= quantity) {
					memset(&item_tmp, 0, sizeof(item_tmp));
					item_tmp.nameid = stor->items[i].nameid;
					item_tmp.identify = 1;
					storage->delitem(sd, i, quantity);
					if ((flag = pc->additem(sd,&item_tmp,quantity,LOG_TYPE_STORAGE))) {
						clif->additem(sd, 0, 0, flag);
						sd->state.storage_flag = 0;
						storage->close(sd);
						pushed_one = true;
						break;
					}
				}
			}
			sd->state.storage_flag = 0;
			storage->close(sd);
			break;
		}
	}

	if (!pushed_one) {
		return false;
	}

	return true;
}

void pc_load_restock_data(int *fd, struct map_session_data **sd_) {
	char *data = NULL;
	struct map_session_data *sd = *sd_;

	if (sd->state.connect_new) {
		if (SQL_ERROR == SQL->Query(map->mysql_handle,
			"SELECT `item_id`, `quantity`, `restock_from` FROM `restock` WHERE `char_id` = '%d'", sd->status.char_id))
			Sql_ShowDebug(map->mysql_handle);

		while (SQL_SUCCESS == SQL->NextRow(map->mysql_handle)) {
			int id, quantity, rs_from;
			SQL->GetData(map->mysql_handle, 0, &data, NULL); id = atoi(data);
			SQL->GetData(map->mysql_handle, 1, &data, NULL); quantity = atoi(data);
			SQL->GetData(map->mysql_handle, 2, &data, NULL); rs_from = atoi(data);
			restock_add_item(sd, id, quantity, rs_from);
			break;
		}
		SQL->FreeResult(map->mysql_handle);
	}
}

BUILDIN(restock_list)
{
	struct restock_data *rsd = NULL;
	struct map_session_data *sd = script->rid2sd(st);
	int i;
	char output[1024];

	if (sd == NULL) {
		script_pushint(st, 0);
		return true;
	}

	if ((rsd = rs_search(sd, false)) == NULL) {
		script_pushint(st, 0);
		return true;
	}

	sprintf(output, "Restock From\tQuantity\tItemID\n");
	clif->message(sd->fd, output);
	for (i = 0; i < VECTOR_LENGTH(rsd->restock); i++) {
		struct restock_item *rsi = &VECTOR_INDEX(rsd->restock, i);
		sprintf(output, "%d.) %s\t%d\t%d", (i + 1), restock_from_name(rsi->restock_from), rsi->quantity, rsi->id);
		clif->message(sd->fd, output);
	}

	script_pushint(st, 1);
	return true;
}

/**
 * restock(<itemID>,<Quantity>,<restock From>,<Add/Remove>)
 */
BUILDIN(restock)
{
	int item_id = 0;
	int quantity = script_getnum(st, 3);
	int rs_from = script_getnum(st, 4);
	int type = script_getnum(st, 5);
	struct map_session_data *sd = script->rid2sd(st);

	if (sd == NULL) {
		script_pushint(st, 0);
		return false;
	}

	if (quantity < 0) {
		clif->message(sd->fd, "restock: quantity should be greater than 0.");
		script_pushint(st, 0);
		return true;
	}

	if (rs_from < 0 || rs_from >= RS_MAX) {
		clif->message(sd->fd, "restock: Invalid 'restock From' Value.");
		script_pushint(st, 0);
		return true;
	}

	if (type < 0 || type > RS_TYPE_MAX) {
		clif->message(sd->fd, "restock: Invalid Type mentioned.");
		script_pushint(st, 0);
		return true;
	}

	if (script_isstringtype(st, 2)) {
		struct item_data* id = itemdb->search_name(script_getstr(st, 2));
		if (id != NULL) {
			item_id = id->nameid;
		}
	} else {
		item_id = script_getnum(st, 2);
		if (itemdb->exists(item_id) == NULL) {
			item_id = 0;
		}
	}

	if (item_id == 0) {
		ShowError("restock: Invalid ItemID(%d).\n", item_id);
		script_pushint(st,0);
		return true;
	}

	switch (type) {
		case RS_TYPE_ADD: {
			restock_add_item(sd, item_id, quantity, rs_from);
			break;
		}
		case RS_TYPE_DEL: {
			int i;
			struct restock_data *rsd = NULL;
			struct restock_item *rsi = NULL;
			if (!rs_exists(sd)) {
				clif->message("restock: No Item exists in restock list.");
				script_pushint(st, 0);
				return true;
			}
			rsd = rs_search(sd, false);

			ARR_FIND(0, VECTOR_LENGTH(rsd->restock), i, VECTOR_INDEX(rsd->restock, i).id == item_id);
			// Not Found
			if (i == VECTOR_LENGTH(rsd->restock)) {
				clif->message("restock: No Such Item exists in restock list.");
				script_pushint(st, 0);
				return true;
			}

			VECTOR_ERASE(rsd->restock, i);

			if (SQL_ERROR == SQL->Query(map->mysql_handle,
				"DELETE FROM `restock` WHERE `item_id`='%d' AND `char_id`='%d'",
										item_id,
										sd->status.char_id
										))
				Sql_ShowDebug(map->mysql_handle);

		}
			break;
		case RS_TYPE_DEL_ALL: {
			if (SQL_ERROR == SQL->Query(map->mysql_handle, "DELETE FROM `restock` `char_id`='%d'", sd->status.char_id))
				Sql_ShowDebug(map->mysql_handle);
			}
			break;
	}

	script_pushint(st, 1);
	return true;
}


/**
 * restock_toggle({true/false})
 * Returns -1 (error), 0(disabled), 1(enabled)
 */
BUILDIN(restock_toggle)
{
	struct map_session_data *sd = script->rid2sd(st);
	int toggle = -1;
	struct restock_data *rsd = NULL;

	if (sd == NULL) {
		script_pushint(st, -1);
		return false;
	}

	if (script_hasdata(st, 2)) {
		toggle = script_getnum(st, 2);
		if (toggle < 0 || toggle > 1) {
			ShowError("restock_toggle: Invalid Toggle Value(%d).\n", toggle);
			script_pushint(st, -1);
			return true;
		}
	}

	if (toggle == 0 && !rs_exists(sd)) {
		script_pushint(st, 1);
		return true;
	}
	
	rsd = rs_search(sd, true);

	if (rsd->enabled && toggle <= 0) {
		rsd->enabled = false;
		script_pushint(st, 0);
	} else if (!rsd->enabled && (toggle == 1 || toggle == -1)) {
		rsd->enabled = true;
		script_pushint(st, 1);
	} else {
		script_pushint(st, -1);
	}
	
	return true;
}

BUILDIN(restock_status)
{
	struct map_session_data *sd = script->rid2sd(st);
	struct restock_data *rsd = NULL;

	if (sd == NULL) {
		script_pushint(st, 0);
		return false;
	}

	if (!rs_exists(sd)) {
		script_pushint(st, 0);
		return true;
	}
	
	rsd = rs_search(sd, true);

	if (rsd->enabled) {
		script_pushint(st, 1);
	} else {
		script_pushint(st, 0);
	}
	
	return true;
}


HPExport void plugin_init(void)
{
	addHookPre(pc, delitem, pc_restock_misc_pre);
	addHookPost(pc, delitem, pc_restock_misc_post);
	addHookPre(clif, pLoadEndAck, pc_load_restock_data);
	addScriptCommand("restock", "viii", restock);
	addScriptCommand("restock_list", "", restock_list);
	addScriptCommand("restock_toggle", "?", restock_toggle);
	addScriptCommand("restock_status", "", restock_status);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
	// Constants
	script->set_constant("RS_TYPE_ADD", RS_TYPE_ADD, false, false);
	script->set_constant("RS_TYPE_DEL", RS_TYPE_DEL, false, false);
	script->set_constant("RS_TYPE_DEL_ALL", RS_TYPE_DEL_ALL, false, false);

	script->set_constant("RS_STORAGE", RS_STORAGE, false, false);
	script->set_constant("RS_GSTORAGE", RS_GSTORAGE, false, false);
}
