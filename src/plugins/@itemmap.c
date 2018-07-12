/*
=============================================
@itemmap Plugin
Converted by: Dastgir
Original Made by: Xantara
================================================
v 1.0 Initial Release
*/

#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/HPMi.h"
#include "common/showmsg.h"

#include "map/atcommand.h"
#include "map/battleground.h"
#include "map/clif.h"
#include "map/guild.h"
#include "map/itemdb.h"
#include "map/log.h"
#include "map/map.h"
#include "map/party.h"
#include "map/pc.h"
#include "map/pet.h"
#include "map/script.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"@itemmap",			// Plugin name
	SERVER_TYPE_MAP,	// Which server types this plugin works with?
	"1.0",				// Plugin version
	HPM_VERSION,		// HPM Version (don't change, macro is automatically updated)
};

/*------------------------------------------
 * pc_getitem_map
 *------------------------------------------*/
int pc_getitem_map(struct map_session_data *sd,struct item it,int amt,int count,e_log_pick_type log_type)
{
	int i, flag;
	
	if (!sd) return false;

	for ( i = 0; i < amt; i += count )
	{
		if ( !pet->create_egg(sd,it.nameid) )
		{ // if not pet egg
			if( (flag = pc->additem(sd, &it, count, log_type)) ) {
				clif->additem(sd, 0, 0, flag);
				if( pc->candrop(sd,&it) )
					map->addflooritem(&sd->bl,&it,count,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0,false);
			}
		}
	}
	
	logs->pick_pc(sd, log_type, -amt, &sd->status.inventory[i],sd->inventory_data[i]);
	
	return 1;
}

/*====================================================================
	getitem_map <item id>,<amount>,"<mapname>"{,<type>,<ID for Type>};
		type: 0=everyone, 1=party, 2=guild, 3=bg
 =====================================================================*/
static int buildin_getitem_map_sub(struct block_list *bl,va_list ap)
{
	struct item it;

	int amt,count;
	TBL_PC *sd = (TBL_PC *)bl;

	it    = va_arg(ap,struct item);
	amt   = va_arg(ap,int);
	count = va_arg(ap,int);

	pc_getitem_map(sd,it,amt,count,LOG_TYPE_SCRIPT);

	return 0;
}

BUILDIN(getitem_map)
{
	struct item it;
	struct guild *g = NULL;
	struct party_data *p = NULL;
	struct battleground_data *bgd = NULL;
	struct script_data *data;

	int m,i,get_count,nameid,amount,type=0,type_id=0;
	const char *mapname;

	data = script_getdata(st,2);
	script->get_val(st,data);
	if( data_isstring(data) )
	{
		const char *name = script->conv_str(st,data);
		struct item_data *item_data = itemdb->search_name(name);
		if( item_data )
			nameid = item_data->nameid;
		else
			nameid = UNKNOWN_ITEM_ID;
	}
	else
		nameid = script->conv_num(st,data);

	if( (amount = script_getnum(st,3)) <= 0 )
		return true;

	mapname = script_getstr(st,4);
	if( (m = map->mapname2mapid(mapname)) < 0 )
		return true;

	if( script_hasdata(st,5) ){
		type    = script_getnum(st,5);
		type_id = script_getnum(st,6);
	}
	
	if( nameid <= 0 || !itemdb->exists(nameid) ){
		ShowError("buildin_getitem_map: Nonexistant item %d requested.\n", nameid);
		return false; //No item created.
	}

	memset(&it,0,sizeof(it));
	it.nameid = nameid;
	it.identify = itemdb->isidentified(nameid);

	if (!itemdb->isstackable(nameid))
		get_count = 1;
	else
		get_count = amount;

	switch(type)
	{
		case 1:
			if( (p = party->search(type_id)) != NULL )
			{
				for( i=0; i < MAX_PARTY; i++ )
					if( p->data[i].sd && m == p->data[i].sd->bl.m )
						pc_getitem_map(p->data[i].sd,it,amount,get_count,LOG_TYPE_SCRIPT);
			}
			break;
		case 2:
			if( (g = guild->search(type_id)) != NULL )
			{
				for( i=0; i < g->max_member; i++ )
					if( g->member[i].sd && m == g->member[i].sd->bl.m )
						pc_getitem_map(g->member[i].sd,it,amount,get_count,LOG_TYPE_SCRIPT);
			}
			break;
		case 3:
			if( (bgd = bg->team_search(type_id)) != NULL )
			{
				for( i=0; i < MAX_BG_MEMBERS; i++ )
					if( bgd->members[i].sd && m == bgd->members[i].sd->bl.m )
						pc_getitem_map(bgd->members[i].sd,it,amount,get_count,LOG_TYPE_SCRIPT);
			}
			break;
		default:
			map->foreachinmap(buildin_getitem_map_sub,m,BL_PC,it,amount,get_count);
			break;
	}

	return true;
}

/*==========================================
 *
 * 0 = @itemmap <item id/name> {<amount>}
 * 1 = @itemmap_p <item id/name> <amount>, <party name>
 * 2 = @itemmap_g <item id/name> <amount>, <guild name>
 * [Xantara]
 *------------------------------------------*/
bool itemmap(const int fd, struct map_session_data* sd, const char* command, const char* message, struct AtCommandInfo *info, int get_type) {
	char item_name[100], party_name[NAME_LENGTH], guild_name[NAME_LENGTH];
	int amount, get_count, i, m;
	struct item it;
	struct item_data *item_data;
	struct party_data *p;
	struct guild *g;
	struct s_mapiterator *iter = NULL;
	struct map_session_data *pl_sd = NULL;

	if (sd == NULL)
		return false;
	
	memset(item_name, '\0', sizeof(item_name));
	memset(party_name, '\0', sizeof(party_name));
	memset(guild_name, '\0', sizeof(guild_name));

	if ((!*message) ||
		(get_type == 0 &&
		(sscanf(message, "\"%99[^\"]\" %d", item_name, &amount) < 1) &&
		(sscanf(message, "%99s %d", item_name, &amount) < 1))) {
			clif->message(fd, "Please, enter an item name/id (usage: @itemmap <item name or ID> {amount}).");
			return false;
	}
	if ((!*message) ||
		(get_type == 1 &&
		(sscanf(message, "\"%99[^\"]\" %d, %23[^\n]", item_name, &amount, party_name) < 2) &&
		(sscanf(message, "%99s %d %23[^\n]", item_name, &amount, party_name) < 2))) {
			clif->message(fd, "Please, enter an item name/id (usage: @itemmap_p <item id/name> <amount> <party name>).");
			return false;
	}
	if ((!*message) ||
		(get_type == 2 &&
		(sscanf(message, "\"%99[^\"]\" %d, %23[^\n]", item_name, &amount, guild_name) < 2) &&
		(sscanf(message, "%99s %d %23[^\n]", item_name, &amount, guild_name) < 2))) {
		clif->message(fd, "Please, enter an item name/id (usage: @itemmap_g <item id/name> <amount> <guild name>).");
		return false;
	}
	if ((item_data = itemdb->search_name(item_name)) == NULL &&
	    (item_data = itemdb->exists(atoi(item_name))) == NULL) {
		clif->message(fd,"Invalid item ID or name."); // Invalid item ID or name.
		return false;
	}
	memset(&it, 0, sizeof(it));
	it.nameid = item_data->nameid;
	if (amount <= 0)
		amount = 1;
	//Check if it's stackable.
	if (!itemdb->isstackable2(item_data))
		get_count = 1;
	else
		get_count = amount;
	
	it.identify = 1;
	
	m = sd->bl.m;
	
	switch(get_type)
	{
		case 1:
			if ((p = party->searchname(party_name)) == NULL){
				clif->message(fd,"Incorrect name/ID, or no one from the specified party is online."); // Incorrect name or ID, or no one from the party is online.
				return false;
			}
			for( i=0; i < MAX_PARTY; i++ )
				if( p->data[i].sd && m == p->data[i].sd->bl.m )
					pc_getitem_map(p->data[i].sd,it,amount,get_count,LOG_TYPE_COMMAND);
			break;
		case 2:
			if ((g = guild->searchname(guild_name)) == NULL) {
				clif->message(fd,"Incorrect name/ID, or no one from the specified guild is online."); // Incorrect name/ID, or no one from the guild is online.
				return false;
			}
			for (i=0; i < g->max_member; i++)
				if( g->member[i].sd && m == g->member[i].sd->bl.m )
					pc_getitem_map(g->member[i].sd,it,amount,get_count,LOG_TYPE_COMMAND);
			break;
		default:
			iter = mapit_getallusers();
			for (pl_sd = (TBL_PC*)mapit->first(iter); mapit->exists(iter); pl_sd = (TBL_PC*)mapit->next(iter)) {
				if( m != pl_sd->bl.m )
					continue;
				pc_getitem_map(pl_sd,it,amount,get_count,LOG_TYPE_COMMAND);
			}
			mapit->free(iter);
			break;
	}

	clif->message(fd,"Item created"); // Item created.
	return true;
}

ACMD(itemmap) {
	return itemmap(fd, sd, command, message, info, 0);
}
ACMD(itemmap_p) {
	return itemmap(fd, sd, command, message, info, 1);
}
ACMD(itemmap_g) {
	return itemmap(fd, sd, command, message, info, 2);
}

/* Server Startup */
HPExport void plugin_init(void)
{
	addAtcommand("itemmap", itemmap);	// All
	addAtcommand("itemmap_p", itemmap_p);	// Party
	addAtcommand("itemmap_g", itemmap_g);	// Guild

	addScriptCommand("getitem_map", "iis??", getitem_map);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
