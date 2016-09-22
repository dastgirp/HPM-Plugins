//===== Hercules Plugin ======================================
//= AreaLoot
//===== By: ==================================================
//= Dastgir/Hercules
//= original by Streusel
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Picking an item from ground will loot all nearby items
//===== Additional Comments: =================================
//= Add 'arealoot_range: 4' to have areloot range of 4x4 cell
//===== Repo Link: ===========================================
//= https://github.com/dastgir/HPM-Plugins
//============================================================

#include "common/hercules.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common/HPMi.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/memmgr.h"
#include "common/nullpo.h"
#include "common/mapindex.h"
#include "common/strlib.h"
#include "common/utils.h"
#include "common/cbasetypes.h"
#include "common/timer.h"
#include "common/showmsg.h"
#include "common/conf.h"
#include "common/db.h"
#include "common/sql.h"

#include "map/atcommand.h"
#include "map/battle.h"
#include "map/clif.h"
#include "map/map.h"
#include "map/pc.h"
#include "map/skill.h"
#include "map/itemdb.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"@arealoot",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

int arealoot_range = 3;   // AxA Range

struct area_p_data {
	bool arealoot;        // is Arealoot on?
	bool in_process;      // Currently Looting from ground?
};

struct area_p_data* adb_search(struct map_session_data* sd){
	struct area_p_data *data;
	if ((data = getFromMSD(sd,0)) == NULL) {
		CREATE(data, struct area_p_data, 1);
		addToMSD(sd, data, 0, true);
	}
	return data;
}

/**
 * @arealoot
 */
ACMD(arealoot)
{
	struct area_p_data *data;
	data = adb_search(sd);
	if (data->arealoot) {
		data->arealoot = false;
		clif->message(fd, "Arealoot is now off.");
		return true;
	}
	data->arealoot = true;
	clif->message(fd, "Arealoot is now on.");
	
	return true;
}


int pc_takeitem_pre(struct map_session_data **sd_, struct flooritem_data **fitem_)
{
	struct area_p_data *data;
	struct map_session_data *sd = *sd_;
	struct flooritem_data *fitem = *fitem_;
	data = adb_search(sd);
	if (data->arealoot && data->in_process==false) {
		data->in_process = true;
		map->foreachinrange(skill->greed, &fitem->bl, arealoot_range, BL_ITEM, &sd->bl);
		hookStop();
		data->in_process = false;
		return 1;
	}
	return 1;
}

void arealoot_range_setting(const char *key, const char *val)
{
	int value = atoi(val);
	if (strcmpi(key,"arealoot_range") == 0) {  //1 to 9 Range.
		if (value < 1 || value > 10) {
			ShowError("'arealoot_range' is set to %d,(Min:1,Max:10)", value);
			return;
		}
		arealoot_range = value;
		ShowDebug("Arealoot_Range set to %d",arealoot_range);
	}
}

int arealoot_range_return(const char *key)
{
	if (strcmpi(key, "arealoot_range") == 0)
		return arealoot_range;

	return 0;
}

/* run when server starts */
HPExport void plugin_init (void) {
    addAtcommand("arealoot",arealoot);
	addHookPre(pc, takeitem, pc_takeitem_pre);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}

HPExport void server_preinit (void) {
	addBattleConf("arealoot_range", arealoot_range_setting, arealoot_range_return, false);
}
