/*
	By Shikazu
	Edited by Dastgir/Hercules
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
#include "common/mapindex.h"
#include "map/clif.h"
#include "map/script.h"
#include "map/skill.h"
#include "map/pc.h"
#include "map/map.h"
#include "map/mob.h"
#include "map/battle.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"@mapmoblist Atcommand",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

static int count_mob(struct block_list *bl, va_list ap) // [FE]
{
	struct mob_data *md = (struct mob_data*)bl;
	int id = va_arg(ap, int);
	if (md->class_ == id)
		return 1;
	return 0;
}

ACMD(mapmoblist)
{
	char temp[100];
	bool mob_searched[MAX_MOB_DB];
	int mob_mvp[MAX_MOB_DB];
	struct s_mapiterator* it;
	unsigned short count = 0, i, mapindex_ = 0;
	int m = 0;
	int mvp_index = 0;

	memset(mob_searched, 0, MAX_MOB_DB);
	memset(mob_mvp, 0, MAX_MOB_DB);

	if (*message) {
		// Player input map name, search mob list for that map
		mapindex_ = mapindex->name2id(message);
		if (!mapindex_) {
			clif->message(fd, "Map not found");
			return -1;
		}
		m = map->mapindex2mapid(mapindex_);
	} else {
		// Player doesn't input map name, search mob list in player current map
		mapindex_ = sd->mapindex;
		m = sd->bl.m;
	}

	clif->message(fd, "--------Monster List--------");

	sprintf(temp, "Map Name: %s", mapindex_id2name(mapindex_));
	clif->message(fd, temp);

	clif->message(fd, "Monsters: ");

	//Looping and search for mobs
	it = mapit_geteachmob();
	while (true) {
		TBL_MOB* md = (TBL_MOB*)mapit->next(it);
		if (md == NULL)
			break;

		if (md->bl.m != m || md->status.hp <= 0)
			continue;
		if (mob_searched[md->class_] == true)
			continue; // Already found, skip it
		if (mob->db(md->class_)->mexp) {
			mob_searched[md->class_] = true;
			mob_mvp[(mvp_index++)] = md->class_;
			continue; // It's MVP!
		}

		mob_searched[md->class_] = true;
		count = map->foreachinmap(count_mob, m, BL_MOB, md->class_);

		sprintf(temp, " %s[%d] : %d", mob->db(md->class_)->jname, md->class_, count);

		clif->message(fd, temp);
	}
	mapit->free(it);

	clif->message(fd, "MVP: ");

	// Looping again and search for mvp, not sure if this is the best way..
	for (i=0; i < mvp_index; i++) {
		count = map->foreachinmap(count_mob, m, BL_MOB, mob_mvp[i]);
		sprintf(temp, " %s[%d] : %d", (mob->db(mob_mvp[i]))->jname, mob_mvp[i], count);
		clif->message(fd, temp);
	}

	return true;
}

/* Server Startup */
HPExport void plugin_init(void)
{
	addAtcommand("mapmoblist", mapmoblist);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
