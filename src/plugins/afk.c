/*
	By Dastgir/Hercules

Changelog:
v1.0 - Initial Conversion
v1.1 - Dead Person cannot @afk.
v1.2 - Added afk_timeout option(Battle_Config too...) Yippy...
v1.3 - Added noafk mapflag :D

Battle Config Adjustment:
You can add "afk_timeout: seconds" in any of the files in conf/battle/ to make it work(so you don't have to recompile everytime you want to change timeout seconds)

MapFlags:
Add mapflag just like you add other mapflags,
e.g:
prontera	mapflag	noafk
^ Add Above to any script, and it will make prontera to be noafk zone.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/HPMi.h"
#include "../common/malloc.h"
#include "../common/mmo.h"
#include "../common/socket.h"
#include "../common/strlib.h"
#include "../common/timer.h"
#include "../common/mapindex.h"
#include "../map/battle.h"
#include "../map/clif.h"
#include "../map/script.h"
#include "../map/skill.h"
#include "../map/pc.h"
#include "../map/map.h"
#include "../map/status.h"

#include "../common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo =
{
	"@afk",			// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.3",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

int afk_timeout = 0;

struct plugin_mapflag {
	unsigned noafk : 1;
};

ACMD(afk){
	/*
	MapFlag Restriction
	*/
	struct plugin_mapflag *mf_data = getFromMAPD(&map->list[sd->bl.m], 0);
	if (mf_data && mf_data->noafk)
	{
		clif->message(fd, "@afk is forbidden in this map.");
		return true;
	}
	if( pc_isdead(sd) ) {
		clif->message(fd, "Cannot use @afk While dead.");
		return true;
	}
	if(DIFF_TICK(timer->gettick(),sd->canlog_tick) < battle->bc->prevent_logout) {
		clif->message(fd, "Failed to use @afk, please try again later.");
		return true;
	}
	sd->state.autotrade = 1;
	sd->state.monster_ignore = 1;
	pc_setsit(sd);
	skill->sit(sd,1);
	clif->sitting(&sd->bl);
	clif->changelook(&sd->bl,LOOK_HEAD_TOP,471);
	clif->specialeffect(&sd->bl, 234,AREA);
	if( afk_timeout )
	{
		status->change_start(NULL, &sd->bl, SC_AUTOTRADE, 10000, 0, 0, 0, 0, afk_timeout*1000,0);
	}
	clif->chsys_quit(sd); //Quit from Channels.
	clif->authfail_fd(fd, 15);
	return true;
}

void afk_timeout_adjust(const char *val) {	//In Seconds
	int value = config_switch(val);
	if (value < 0){
		ShowDebug("Received Invalid Setting for afk_timeout(%d), defaulting to 0\n",value);
		return;
	}
	afk_timeout = value;
	return;
	
}

void parse_noafk_mapflag(const char *name, char *w3, char *w4, const char* start, const char* buffer, const char* filepath, int *retval){
	int16 m = map->mapname2mapid(name);
	if (!strcmpi(w3,"noafk")){
		struct plugin_mapflag *mf_data;
		if ( !( mf_data = getFromMAPD(&map->list[m], 0) ) )
		{
			CREATE(mf_data,struct plugin_mapflag,1);
			mf_data->noafk = 1;
			addToMAPD(&map->list[m], mf_data, 0, true);
		}
		mf_data->noafk = 1;
		hookStop();
	}
	
	return;
}

/* Server Startup */
HPExport void plugin_init (void){
	iMalloc = GET_SYMBOL("iMalloc");
	clif = GET_SYMBOL("clif");
	script = GET_SYMBOL("script");
	skill = GET_SYMBOL("skill");
	pc = GET_SYMBOL("pc");
	strlib = GET_SYMBOL("strlib");
	battle = GET_SYMBOL("battle");
	timer = GET_SYMBOL("timer");
	map = GET_SYMBOL("map");
	status = GET_SYMBOL("status");

	addAtcommand("afk",afk);
	addHookPre("npc->parse_unknown_mapflag",parse_noafk_mapflag);
}

HPExport void server_preinit (void) {
	addBattleConf("afk_timeout",afk_timeout_adjust);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}

