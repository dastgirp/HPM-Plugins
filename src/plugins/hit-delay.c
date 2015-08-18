/*
	By Dastgir/Hercules
	Warp Delays only if hit, if miss, you can still warp.
	You can have a battle_config on any of the files in conf/battle with "go_warp_delay: Seconds*1000".
	6 Battle Configs:
	warp_delay
	warp_delay_mob
	warp_delay_pet
	warp_delay_homun
	warp_delay_merc
	warp_delay_others
	Format same as mentioned above.

v1.0 - Initial Release.
v1.1 - Now Adjustable Delay from hits from player/homun/mobs/etc..
v1.1a- Fix Crash from @die.
v1.2 - Teleportation does not cause delay.
v1.3 - Players and Others are now separated.
v1.3a- You can now warp once dead.
v1.3b- Some Crash Fixes.
*/
#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/HPMi.h"
#include "common/malloc.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/strlib.h"
#include "common/timer.h"
#include "common/mapindex.h"
#include "map/clif.h"
#include "map/script.h"
#include "map/skill.h"
#include "map/pc.h"
#include "map/map.h"
#include "map/battle.h"

#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo =
{
	"Warp Delay",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.3b",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

int64 warp_delay = 5000;		//Seconds*1000 (Second) Delay(For Player).
int64 warp_delay_mob = 6000;	//Seconds*1000 (Second) Delay(For Monster).
int64 warp_delay_pet = 7000;	//Seconds*1000 (Second) Delay(For Pet).
int64 warp_delay_homun = 8000;	//Seconds*1000 (Second) Delay(For Homunculus).
int64 warp_delay_merc = 9000;	//Seconds*1000 (Second) Delay(For Mercenary).
int64 warp_delay_others = 10000;//Seconds*1000 (Second) Delay(For Others).

struct warp_delay_tick {
	int64 last_hit;
	enum bl_type who_hit;
};

void pc_damage_received(struct map_session_data *sd,struct block_list *src,unsigned int hp, unsigned int sp){
	struct warp_delay_tick *delay_data;
	if( !(delay_data = getFromMSD(sd,0)) ) {
		CREATE(delay_data,struct warp_delay_tick,1);
		delay_data->last_hit = timer->gettick();
		delay_data->who_hit = BL_NUL;
		addToMSD(sd,delay_data,0,true);
	}
	delay_data->last_hit = timer->gettick();
	if (src)
		delay_data->who_hit = src->type;
	else
		delay_data->who_hit = BL_NUL;
	return;
	
}
int pc_setpos_delay(struct map_session_data* sd, unsigned short *map_index, int *x, int *y, clr_type *clrtype) {
	int16 m;
	struct warp_delay_tick *delay_data;
	unsigned short mapindex_ = *map_index;
	int64 temp_delay;

	if (!sd)
		return 0;

	if( !mapindex_ || !mapindex_id2name(mapindex_) || ( m = map->mapindex2mapid(mapindex_) ) == -1 ) {
		ShowDebug("pc_setpos: Passed mapindex(%d) is invalid!\n", mapindex_);
		return 1;
	}
	if( !(delay_data = getFromMSD(sd,0)) ) {
		return 0;
	}
	switch(delay_data->who_hit){
		case BL_MOB:
			temp_delay = warp_delay_mob;
			break;
		case BL_PET:
			temp_delay = warp_delay_pet;
			break;
		case BL_HOM:
			temp_delay = warp_delay_homun;
			break;
		case BL_MER:
			temp_delay = warp_delay_merc;
			break;
		case BL_NUL:
			temp_delay = 0;
			break;
		case BL_PC:
			temp_delay = warp_delay;
			break;
		default:
			temp_delay = warp_delay_others;
			break;
	}
	if (sd->status.hp == 0 && DIFF_TICK(timer->gettick(),delay_data->last_hit) < temp_delay ){
		char output[50];
		sprintf(output,"Please Wait %d second before warping.",(int)((temp_delay-DIFF_TICK(timer->gettick(),delay_data->last_hit))/1000));
		clif->message(sd->fd,output);
		hookStop();
	}
	return 0;
}

int battle_config_validate(const char *val,const char *setting,int64 default_delay){
	int value = config_switch(val);
	if (value <= 0){
		ShowDebug("Received Invalid Setting for %s(%d), Defaulting to %d\n",setting,value,(int)default_delay);
		return (int)default_delay;
	}
	return value;
}

void go_warp_delay_setting(const char *val) {
	warp_delay = (int64)battle_config_validate(val,"warp_delay",warp_delay);
}

void go_warp_delay_others_setting(const char *val) {
	warp_delay_others = (int64)battle_config_validate(val,"warp_delay_others",warp_delay_others);
}

void go_warp_delay_pet_setting(const char *val) {
	warp_delay_pet = (int64)battle_config_validate(val,"warp_delay_pet",warp_delay_pet);
}

void go_warp_delay_homun_setting(const char *val) {
	warp_delay_homun = (int64)battle_config_validate(val,"warp_delay_homun",warp_delay_homun);
}

void go_warp_delay_mob_setting(const char *val) {
	warp_delay_mob = (int64)battle_config_validate(val,"warp_delay_mob",warp_delay_mob);
}

void go_warp_delay_merc_setting(const char *val) {
	warp_delay_merc = (int64)battle_config_validate(val,"warp_delay_merc",warp_delay_merc);
}

/* Server Startup */
HPExport void plugin_init (void)
{
	addHookPre("pc->setpos",pc_setpos_delay);
	addHookPre("pc->damage",pc_damage_received);
}

HPExport void server_preinit (void) {
	addBattleConf("warp_delay",go_warp_delay_setting);
	addBattleConf("warp_delay_mob",go_warp_delay_mob_setting);
	addBattleConf("warp_delay_pet",go_warp_delay_pet_setting);
	addBattleConf("warp_delay_homun",go_warp_delay_homun_setting);
	addBattleConf("warp_delay_merc",go_warp_delay_merc_setting);
	addBattleConf("warp_delay_others",go_warp_delay_others_setting);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}
