/*
	By Dastgir/Hercules
	Critical Attack even for Magic Attacks!!!
	You can have a battle_config on any of the files in conf/battle with "magic_critical_color: x".
	x can be 1 or 2
	1 = Blue Critical
	2 = Red Critical
	
v1.0a - Fix Warnings on Windows
v1.0b - Fix Bug of double drop
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
#include "common/timer.h"
#include "common/mapindex.h"
#include "common/utils.h"
#include "map/clif.h"
#include "map/script.h"
#include "map/skill.h"
#include "map/pc.h"
#include "map/map.h"
#include "map/battle.h"
#include "map/status.h"
#include "map/mob.h"
#include "map/npc.h"

#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo =
{
	"Critical Magic Attack",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.1",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

//Configuration 
int blue_red_critical = 1;		//1=Red,2=Blue
/* Don't Change below this. */

struct tmp_data {
	struct block_list *src;
	struct block_list *bl;
	uint16 num[2];
};

int skill_mcri_hit(int tid, int64 tick, int id, intptr_t data)
{
	struct mob_data *md = (struct mob_data *)data;
	struct tmp_data *tmpd;
	tmpd = getFromMOBDATA(md,0);
	if(tmpd!=NULL){
		switch(blue_red_critical){	// 1 = red ; 2 = blue
			case 1:
				clif->damage(tmpd->src, tmpd->bl, 1, 1, id, 0, 10, 0);		  
				break;
			case 2:
				clif->skill_damage(tmpd->src,tmpd->bl,timer->gettick(), 1, 1, id, 0, TK_STORMKICK, 1, 8);
				clif->skill_nodamage(tmpd->src,tmpd->src,tmpd->num[0],tmpd->num[1],1);
				break;
		}
	}
	return 0;
}
int skill_mcri_kill_delay(int tid, int64 tick, int id, intptr_t data)
{
	struct block_list *bl = map->id2bl(id);
	struct block_list *src = (struct block_list *)data;
	struct mob_data *md = BL_CAST(BL_MOB, bl);
	if(bl!=NULL){
		if (md != NULL)
			mob->dead(md, src, 0);
	}
	return 0;
}

void magic_critical_attack(int *attack_type, struct block_list* src, struct block_list *dsrc, struct block_list *bl, uint16 *skill_id_, uint16 *skill_lv_, int64 *tick_, int *flag_, int *type_, struct Damage *dmg, int64 *damage_) {
	uint16 skill_id = *skill_id_;
	uint16 skill_lv = *skill_lv_;
	int64 tick = *tick_;
	int64 damage = *damage_;
	struct map_session_data *sd;
	struct status_data *tstatus;
	
	sd = BL_CAST(BL_PC, src);
	tstatus = status->get_status_data(bl);
	if ( src && (src->type == BL_PC || (battle->get_master(src))->type == BL_PC) ){
		int m_cri = 0;
		int random = rand()%100;
		if(sd==NULL)
			m_cri = cap_value(map->id2sd((battle->get_master(src))->id)->battle_status.cri/10,1,100);
		else
			m_cri = cap_value(sd->battle_status.cri/10,1,100);
		if( random < m_cri )
		{
			struct mob_data *md=NULL;
			struct tmp_data *tmpd=NULL;
			int i=0, d_=200;
			int64 _damage = 0,u_=0;
			int num=abs(skill->get_num(skill_id,skill_lv));
			damage = damage*2;
			md = mob->once_spawn_sub(src, src->m, src->x, src->y, "--en--",1083,"", SZ_SMALL, AI_NONE);
			if( !(tmpd = getFromMOBDATA(md,0)) ) {
				CREATE(tmpd,struct tmp_data,1);
				addToMOBDATA(md,tmpd,0,true);
			}
			md->deletetimer=timer->add(tick+d_*num+1,mob->timer_delete,md->bl.id,0);
			status->set_viewdata(&md->bl, INVISIBLE_CLASS);
			if(skill->get_splash(skill_id,skill_lv)>1 && num>1)
				num = 1;
			_damage = damage/num;
			tmpd->src = src;
			tmpd->bl = bl;
			tmpd->num[0]=skill_id;
			tmpd->num[1]=skill_lv;
			u_ = tick+d_*num+1;
			if( tstatus->hp <= damage )//delay to kill it
			{
				damage = 1;
				status->change_start(NULL, bl, SC_BLADESTOP_WAIT, 10000, 1, 0, 0, 0, (int)u_, 2);   
				status->change_start(NULL, bl, SC_INVINCIBLE, 10000, 1, 0, 0, 0, (int)u_, 2);			  
				timer->add(u_,skill_mcri_kill_delay,bl->id,(intptr_t)src);
			}
			clif->skill_nodamage(src,src,skill_id,skill_lv,1);
			for(i=0;i<num;i++)
				timer->add(tick+d_*i +1,skill_mcri_hit,(int)_damage,(intptr_t)md);
			u_ = d_ = 0;
			_damage = 0;
			hookStop();
		}
	}
}

void critical_color(const char *key, const char *val) {
	/* do anything with the var e.g. config_switch(val) */
	if (strcmpi(key,"magic_critical_color") == 0){
		int value = config_switch(val);
		if (value < 1 || value >2){
			ShowDebug("Received Invalid Setting for magic_critical_color(%d), defaulting to %d\n",value,blue_red_critical);
			return;
		}
		blue_red_critical = value;
	}
	return;
	
}
int critical_color_return(const char *key)
{
	if (strcmpi(key,"magic_critical_color") == 0){
		return blue_red_critical;
	}
	return 0;
}

/* Server Startup */
HPExport void plugin_init (void)
{
	addHookPre("skill->attack_display_unknown",magic_critical_attack);
}

HPExport void server_preinit (void) {
	addBattleConf("magic_critical_color",critical_color, critical_color_return);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}