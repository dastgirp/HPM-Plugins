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

#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"Duplicate NPC's Command",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",			// Plugin version
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
	int tclass_, txs = -1, tys = -1, sourceid, type, tmapid, i;
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
	
	sourceid = nd_source->bl.id;
	type = nd_source->subtype;

	tmapid = map->mapname2mapid(tmap);
	if(tmapid < 0)
	{
		ShowError("duplicatenpc: target map not found. (%s)\n", tmap);
		script_pushint(st, 0);
		return 0;
	}

	nd_target = npc->create_npc(tmapid, tx, ty);
	
	strcat(targetname, dup_name);
	strncat(targetname, "#", 1);
	strncat(targetname, dup_hidden_name, strlen(dup_hidden_name));

	safestrncpy(nd_target->name, targetname , sizeof(nd_target->name));
	safestrncpy(nd_target->exname, targetname, sizeof(nd_target->exname));

	nd_target->class_ = tclass_;
	nd_target->speed = 200;
	nd_target->src_id = sourceid;
	nd_target->bl.type = BL_NPC;
	nd_target->subtype = (enum npc_subtype)type;
	switch(type)
	{
		case SCRIPT:
			nd_target->u.scr.xs = txs;
			nd_target->u.scr.ys = tys;
			nd_target->u.scr.script = nd_source->u.scr.script;
			nd_target->u.scr.label_list = nd_source->u.scr.label_list;
			nd_target->u.scr.label_list_num = nd_source->u.scr.label_list_num;
			nd_target->u.scr.shop = nd_source->u.scr.shop;
			nd_target->u.scr.trader = nd_source->u.scr.trader;
			break;

		case SHOP:
		case CASHSHOP:
			nd_target->u.shop.shop_item = nd_source->u.shop.shop_item;
			nd_target->u.shop.count = nd_source->u.shop.count;
			break;

		case WARP:
			if( !battle->bc->warp_point_debug )
				nd_target->class_ = WARP_CLASS;
			else
				nd_target->class_ = WARP_DEBUG_CLASS;
			nd_target->u.warp.xs = txs;
			nd_target->u.warp.ys = tys;
			nd_target->u.warp.mapindex = nd_source->u.warp.mapindex;
			nd_target->u.warp.x = nd_source->u.warp.x;
			nd_target->u.warp.y = nd_source->u.warp.y;
			break;
	}

	map->addnpc(tmapid, nd_target);
	//status->change_init(&nd_target->bl);
	//unit->dataset(&nd_target->bl);
	nd_target->ud = &npc->base_ud;
	nd_target->dir = tdir;
	npc->setcells(nd_target);
	map->addblock(&nd_target->bl);
	if(tclass_ >= 0)
	{
		status->set_viewdata(&nd_target->bl, nd_target->class_);
		if( map->list[tmapid].users )
			clif->spawn(&nd_target->bl);
	}
	strdb_put(npc->name_db, nd_target->exname, nd_target);

	if(type == SCRIPT)
	{
		for (i = 0; i < nd_target->u.scr.label_list_num; i++)
		{
			if (npc->event_export(nd_target, i)) {
				ShowWarning("duplicatenpc: duplicate event %s::%s.\n",
							 nd_target->exname, nd_target->u.scr.label_list[i].name);
			}
			npc->timerevent_export(nd_target, i);
		}
		nd_target->u.scr.timerid = INVALID_TIMER;
	}

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
HPExport void plugin_init (void) 
{
	addScriptCommand("duplicatenpc", "ssssiii???", duplicatenpc);
	addScriptCommand("duplicateremove", "?", duplicateremove);

}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}