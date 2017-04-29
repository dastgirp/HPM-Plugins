/*
	By Dastgir/Hercules
*/

#include "common/hercules.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common/cbasetypes.h"
#include "common/db.h"
#include "common/ers.h"
#include "common/memmgr.h"
#include "common/nullpo.h"
#include "common/showmsg.h"
#include "common/socket.h"
#include "common/strlib.h"
#include "common/timer.h"
#include "common/utils.h"
#include "map/battle.h"
#include "map/clif.h"
#include "map/map.h"
#include "map/mob.h"
#include "map/pc.h"
#include "map/script.h"
#include "map/status.h"
#include "map/unit.h"
#include "map/npc.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"Duplicate NPC's Command",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.1",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

// duplicatenpc("NpcName", "DuplicateName", "DupHiddenName", "map", x, y, dir{, sprite{, xs, ys}});
BUILDIN(duplicatenpc)
{
	const char *npc_name = script_getstr(st, 2);
	const char *dup_name = script_getstr(st, 3);
	const char *dup_hidden_name = script_getstr(st, 4);
	const char *tmap = script_getstr(st, 5);
	int tx = script_getnum(st, 6);
	int ty = script_getnum(st, 7);
	int tdir = script_getnum(st, 8);
	int tclass_, txs = -1, tys = -1, tmapid;
	struct npc_data *nd_source, *nd_target;
	char targetname[24] = "";

	if(script_hasdata(st, 10))
		txs = (script_getnum(st, 10) < -1) ? -1 : script_getnum(st, 10);
	if(script_hasdata(st, 11))
		tys = (script_getnum(st, 11) < -1) ? -1 : script_getnum(st, 10);

	if(txs == -1 && tys != -1)
		txs = 0;
	if(txs != - 1 && tys == -1)
		tys = 0;

	if(strlen(dup_name) + strlen(dup_hidden_name) > NAME_LENGTH)
	{
		ShowError("duplicatenpc: Name#HiddenName is to long (max %d chars). (%s)\n",NAME_LENGTH, npc_name);
		script_pushint(st, 0);
		return 0;
	}

	nd_source = npc->name2id(npc_name);

	if(script_hasdata(st, 9))
		tclass_ = (script_getnum(st, 9) < -1) ? -1 : script_getnum(st, 9);
	else
		tclass_ = nd_source->class_;

	if( nd_source == NULL)
	{
		ShowError("duplicatenpc: original npc not found for duplicate. (%s)\n", npc_name);
		script_pushint(st, 0);
		return 0;
	}

	tmapid = map->mapname2mapid(tmap);
	if(tmapid < 0)
	{
		ShowError("duplicatenpc: target map not found. (%s)\n", tmap);
		script_pushint(st, 0);
		return 0;
	}

	nd_target = npc->create_npc(nd_source->subtype, tmapid, tx, ty, tdir, tclass_);
	strcat(targetname, dup_name);
	if (strcmp(dup_hidden_name, "") != 0) {
		strncat(targetname, "#", 1);
		strncat(targetname, dup_hidden_name, strlen(dup_hidden_name));
	}

	safestrncpy(nd_target->name, targetname , sizeof(nd_target->name));
	safestrncpy(nd_target->exname, targetname, sizeof(nd_target->exname));

	npc->duplicate_sub(nd_target, nd_source, txs, tys, NPO_ONINIT);
	
	
	script_pushint(st, 1);
	return true;
}


// [Kenpachi]
// DuplicateRemove({"NPCname"});
BUILDIN(duplicateremove)
{
	struct npc_data *nd;

	if(script_hasdata(st, 2))
	{
		nd = npc->name2id(script_getstr(st, 2));
		if(nd == NULL)
		{
			ShowError("duplicateremove: NPC not found: %s\n", script_getstr(st, 2));
			script_pushint(st, -1);
			return 0;
		}
	}
	else
		nd = (struct npc_data *)map->id2bl(st->oid);

	if (nd == NULL){
	}else if(nd->src_id == 0){	//remove all dupicates for this source npc
		map->foreachnpc(npc->unload_dup_sub,nd->bl.id);
	}else// just remove this duplicate
		npc->unload(nd,true);

	script_pushint(st, 1);
	return true;
}


/* Server Startup */
HPExport void plugin_init(void)
{
	addScriptCommand("duplicatenpc", "ssssiii???", duplicatenpc);
	addScriptCommand("duplicateremove", "?", duplicateremove);

}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
