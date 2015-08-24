#include "common/hercules.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common/HPMi.h"
#include "common/nullpo.h"

#include "map/atcommand.h"
#include "map/battle.h"
#include "map/clif.h"
#include "map/map.h"
#include "map/mob.h"
#include "common/HPMDataCheck.h"


HPExport struct hplugin_info pinfo = {
	"monster_nodropexp",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

struct tmp_data {
	bool no_drop_exp;
};

ACMD(monster_nde)
{
	char name[NAME_LENGTH];
	char monster[NAME_LENGTH];
	char eventname[EVENT_NAME_LENGTH] = "";
	int mob_id;
	int number = 0;
	int count;
	int i, range;
	short mx, my;
	unsigned int size;

	memset(name, '\0', sizeof(name));
	memset(monster, '\0', sizeof(monster));
	memset(atcmd_output, '\0', sizeof(atcmd_output));

	if (!message || !*message) {
		clif->message(fd, msg_fd(fd,80)); // Please specify a display name or monster name/id.
		return false;
	}
	if (sscanf(message, "\"%23[^\"]\" %23s %d", name, monster, &number) > 1 ||
		sscanf(message, "%23s \"%23[^\"]\" %d", monster, name, &number) > 1) {
		//All data can be left as it is.
	} else if ((count=sscanf(message, "%23s %d %23s", monster, &number, name)) > 1) {
		//Here, it is possible name was not given and we are using monster for it.
		if (count < 3) //Blank mob's name.
			name[0] = '\0';
	} else if (sscanf(message, "%23s %23s %d", name, monster, &number) > 1) {
		//All data can be left as it is.
	} else if (sscanf(message, "%23s", monster) > 0) {
		//As before, name may be already filled.
		name[0] = '\0';
	} else {
		clif->message(fd, msg_fd(fd,80)); // Give a display name and monster name/id please.
		return false;
	}

	if ((mob_id = mob->db_searchname(monster)) == 0) // check name first (to avoid possible name beginning by a number)
		mob_id = mob->db_checkid(atoi(monster));

	if (mob_id == 0) {
		clif->message(fd, msg_fd(fd,40)); // Invalid monster ID or name.
		return false;
	}

	if (number <= 0)
		number = 1;

	if( !name[0] )
		strcpy(name, "--ja--");

	// If value of atcommand_spawn_quantity_limit directive is greater than or equal to 1 and quantity of monsters is greater than value of the directive
	if (battle->bc->atc_spawn_quantity_limit && number > battle->bc->atc_spawn_quantity_limit)
		number = battle->bc->atc_spawn_quantity_limit;

	if (strcmpi(info->command, "monstersmall") == 0)
		size = SZ_MEDIUM;
	else if (strcmpi(info->command, "monsterbig") == 0)
		size = SZ_BIG;
	else
		size = SZ_SMALL;

	if (battle->bc->etc_log)
		ShowInfo("%s monster='%s' name='%s' id=%d count=%d (%d,%d)\n", command, monster, name, mob_id, number, sd->bl.x, sd->bl.y);

	count = 0;
	range = (int)sqrt((float)number) +2; // calculation of an odd number (+ 4 area around)
	for (i = 0; i < number; i++) {
		int k;
		struct mob_data *md;
		map->search_freecell(&sd->bl, 0, &mx,  &my, range, range, 0);
		k = mob->once_spawn(sd, sd->bl.m, mx, my, name, mob_id, 1, eventname, size, AI_NONE|(mob_id == MOBID_EMPERIUM?0x200:0x0));
		if (k) {
			struct tmp_data *tmpd;
			md = (TBL_MOB*)map->id2bl(k);
			CREATE(tmpd,struct tmp_data,1);
			tmpd->no_drop_exp = true;
			addToMOBDATA(md,tmpd,0,true);
		}
		count += (k != 0) ? 1 : 0;
	}

	if (count != 0)
		if (number == count)
			clif->message(fd, msg_fd(fd,39)); // All monster summoned!
		else {
			sprintf(atcmd_output, msg_fd(fd,240), count); // %d monster(s) summoned!
			clif->message(fd, atcmd_output);
		}
		else {
			clif->message(fd, msg_fd(fd,40)); // Invalid monster ID or name.
			return false;
		}

	return true;
}

int mob_dead_nde(struct mob_data *md, struct block_list *src, int *type) {
	struct tmp_data *tmpd;
	tmpd = getFromMOBDATA(md,0);
	if (tmpd != NULL)
		if (tmpd->no_drop_exp)
			*type = 3;
}

/* Server Startup */
HPExport void plugin_init (void)
{
	addAtcommand("monster_nde",monster_nde);
	
	addHookPre("mob->dead",mob_dead_nde);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}
