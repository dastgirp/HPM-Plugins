/*
	By Dastgir/Hercules
	Warp Delays only if hit, if miss, you can still warp.
	You can have a battle_config on any of the files in conf/battle with "go_warp_delay: Seconds*1000".
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
#include "../map/clif.h"
#include "../map/script.h"
#include "../map/skill.h"
#include "../map/pc.h"
#include "../map/map.h"
#include "../map/battle.h"

#include "../common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo =
{
	"Warp Delay",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

int64 warp_delay = 10000;	//Seconds*1000 Second Delay.
struct warp_delay_tick {
	int64 last_hit;
};

void pc_damage_received(struct map_session_data *sd,struct block_list *src,unsigned int hp, unsigned int sp){
	struct warp_delay_tick *delay_data;
	if( !(delay_data = getFromMSD(sd,0)) ) {
		CREATE(delay_data,struct warp_delay_tick,1);
		delay_data->last_hit = timer->gettick();
		addToMSD(sd,delay_data,0,true);
	}
	delay_data->last_hit = timer->gettick();
	return;
	
}
int pc_setpos_delay(struct map_session_data* sd, unsigned short *map_index, int *x, int *y, clr_type *clrtype) {
	int16 m;
	struct warp_delay_tick *delay_data;
	unsigned short mapindex_ = *map_index;

	if (!sd)
		return 0;

	if( !mapindex_ || !mapindex_id2name(mapindex_) || ( m = map->mapindex2mapid(mapindex_) ) == -1 ) {
		ShowDebug("pc_setpos: Passed mapindex(%d) is invalid!\n", mapindex_);
		return 1;
	}
	if( !(delay_data = getFromMSD(sd,0)) ) {
		return 0;
	}
	if (DIFF_TICK(timer->gettick(),delay_data->last_hit) < warp_delay ){
		char output[50];
		sprintf(output,"Please Wait %d second before warping.",(int)((warp_delay-DIFF_TICK(timer->gettick(),delay_data->last_hit))/1000));
		clif->message(sd->fd,output);
		hookStop();
	}
	return 0;
}
void go_warp_delay_setting(const char *val) {
	/* do anything with the var e.g. config_switch(val) */
	int value = config_switch(val);
	if (value <= 0){
		ShowDebug("Received Invalid Setting for go_warp_delay(%d), defaulting to %d\n",value,(int)warp_delay);
		return;
	}
	warp_delay = (int64)value;
	return;
	
}

/* Server Startup */
HPExport void plugin_init (void)
{
	iMalloc = GET_SYMBOL("iMalloc");
	clif = GET_SYMBOL("clif");
	script = GET_SYMBOL("script");
	skill = GET_SYMBOL("skill");
	pc = GET_SYMBOL("pc");
	strlib = GET_SYMBOL("strlib");
	battle = GET_SYMBOL("battle");
	timer = GET_SYMBOL("timer");
	map = GET_SYMBOL("map");
	mapindex = GET_SYMBOL("mapindex");
	

	addHookPre("pc->setpos",pc_setpos_delay);
	addHookPre("pc->damage",pc_damage_received);
}

HPExport void server_preinit (void) {
	addBattleConf("go_warp_delay",go_warp_delay_setting);
}

HPExport void server_online (void) {
	ShowInfo ("%s Plugin by Dastgir/Hercules\n",pinfo.name);
}
