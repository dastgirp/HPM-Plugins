//===== Hercules Plugin ======================================
//= Soul Link Boost: Single Strip bypass FCP 
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Related Topic:
//= http://herc.ws/board/files/file/177-soul-link-boost-single-strip-bypass-fcp/
//===== Changelog: ===========================================
//= v1.0 - Initial Release.
//===== Repo Link: ===========================================
//= https://github.com/dastgir/HPM-Plugins
//============================================================
#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/HPMi.h"
#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/nullpo.h"
#include "common/strlib.h"
#include "common/timer.h"

#include "map/battle.h"
#include "map/clif.h"
#include "map/guild.h"
#include "map/map.h"
#include "map/mob.h"
#include "map/party.h"
#include "map/pc.h"
#include "map/skill.h"
#include "map/status.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo =
{
	"SoulLink Boost(FCP Bypass)",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

int skill_castend_nodamage_id_pre(struct block_list **src_, struct block_list **bl_, uint16 *skill_id_, uint16 *skill_lv_, int64 *tick_, int *flag_)
{
	struct map_session_data *sd, *dstsd;
	struct mob_data *dstmd;
	struct status_data *sstatus, *tstatus;
	struct status_change *tsc;
	bool hookS = false;

	struct block_list *src = *src_, *bl = *bl_;
	uint16 skill_id = *skill_id_, skill_lv = *skill_lv_;
	int64 tick = *tick_;
	int flag = *flag_;
	
	int element = 0;
	enum sc_type type;

	if(skill_id > 0 && !skill_lv)
		return 0; // [Celest]

	nullpo_retr(1, src);
	nullpo_retr(1, bl);

	if (src->m != bl->m)
		return 1;

	sd = BL_CAST(BL_PC, src);

	dstsd = BL_CAST(BL_PC, bl);
	dstmd = BL_CAST(BL_MOB, bl);

	if(bl->prev == NULL)
		return 1;
	if(status->isdead(src)) {
		return 1;
	}
	
	switch (skill_id) {
		case RG_STRIPWEAPON:
		case RG_STRIPSHIELD:
		case RG_STRIPARMOR:
		case RG_STRIPHELM:
		case ST_FULLSTRIP:
		case GC_WEAPONCRUSH:
		case SC_STRIPACCESSARY:
			hookS = true;
			break;
		default:
			return 0;
	}
	if (src != bl && status->isdead(bl))
		if (skill->castend_nodamage_id_dead_unknown(src, bl, &skill_id, &skill_lv, &tick, &flag))
			return 1;

	// Supportive skills that can't be cast in users with mado
	if (sd && dstsd && pc_ismadogear(dstsd)) {
		if (skill->castend_nodamage_id_mado_unknown(src, bl, &skill_id, &skill_lv, &tick, &flag))
			return 0;
	}

	tstatus = status->get_status_data(bl);
	sstatus = status->get_status_data(src);

	type = status->skill2sc(skill_id);
	tsc = status->get_sc(bl);


	if (src != bl && type > SC_NONE
	 && (element = skill->get_ele(skill_id, skill_lv)) > ELE_NEUTRAL
	 && skill->get_inf(skill_id) != INF_SUPPORT_SKILL
	 && battle->attr_fix(NULL, NULL, 100, element, tstatus->def_ele, tstatus->ele_lv) <= 0)
		return 1; //Skills that cause an status should be blocked if the target element blocks its element.

	map->freeblock_lock();
	
	switch (skill_id) {
		case RG_STRIPWEAPON:
		case RG_STRIPSHIELD:
		case RG_STRIPARMOR:
		case RG_STRIPHELM:
		case ST_FULLSTRIP:
		case GC_WEAPONCRUSH:
		case SC_STRIPACCESSARY: {
			unsigned short location = 0;
			int d = 0, rate;

			//Rate in percent
			if (skill_id == ST_FULLSTRIP)
				rate = 5 + 2*skill_lv + (sstatus->dex - tstatus->dex)/5;
			else if (skill_id == SC_STRIPACCESSARY)
				rate = 12 + 2 * skill_lv + (sstatus->dex - tstatus->dex)/5;
			else
				rate = 5 + 5*skill_lv + (sstatus->dex - tstatus->dex)/5;

			if (rate < 5) rate = 5; //Minimum rate 5%

			//Duration in ms
			if (skill_id == GC_WEAPONCRUSH) {
				d = skill->get_time(skill_id,skill_lv);
				if (bl->type == BL_PC)
					d += 1000 * ( skill_lv * 15 + ( sstatus->dex - tstatus->dex ) );
				else
					d += 1000 * ( skill_lv * 30 + ( sstatus->dex - tstatus->dex ) / 2 );
			} else
				d = skill->get_time(skill_id,skill_lv) + (sstatus->dex - tstatus->dex)*500;

			if (d < 0)
				d = 0; // Minimum duration 0ms

			switch (skill_id) {
			case RG_STRIPWEAPON:
			case GC_WEAPONCRUSH:
				location = EQP_WEAPON;
				break;
			case RG_STRIPSHIELD:
				location = EQP_SHIELD;
				break;
			case RG_STRIPARMOR:
				location = EQP_ARMOR;
				break;
			case RG_STRIPHELM:
				location = EQP_HELM;
				break;
			case ST_FULLSTRIP:
				location = EQP_WEAPON|EQP_SHIELD|EQP_ARMOR|EQP_HELM;
				break;
			case SC_STRIPACCESSARY:
				location = EQP_ACC;
				break;
			}

			//Special message when trying to use strip on FCP [Jobbie]
			if (sd && skill_id == ST_FULLSTRIP && tsc && tsc->data[SC_PROTECTWEAPON] && tsc->data[SC_PROTECTHELM] && tsc->data[SC_PROTECTARMOR] && tsc->data[SC_PROTECTSHIELD])
			{
				clif->gospel_info(sd, 0x28);
				break;
			}
			
			// FCP
			// By pass FCP when using single strip skills by 15%(requires Glistening Coat).
			if (sd && tsc && sd->sc.data[SC_SOULLINK] && sd->sc.data[SC_SOULLINK]->val2 == SL_ROGUE && rand()%100 < 15 &&
				((skill_id == RG_STRIPWEAPON && tsc->data[SC_PROTECTWEAPON]) ||
				(skill_id == RG_STRIPSHIELD && tsc->data[SC_PROTECTSHIELD]) ||
				(skill_id == RG_STRIPARMOR && tsc->data[SC_PROTECTARMOR]) ||
				(skill_id == RG_STRIPHELM && tsc->data[SC_PROTECTHELM]))
				) {
				int item_id = 7139; // Glistening Coat
				int ii;
				ARR_FIND(0, MAX_INVENTORY, ii, sd->status.inventory[ii].nameid == item_id);
				if (ii < MAX_INVENTORY) {
					pc->delitem(sd, ii, 1, 0, 0, LOG_TYPE_CONSUME);
					switch (skill_id) {
						case RG_STRIPWEAPON:
							status_change_end(bl, SC_PROTECTWEAPON, INVALID_TIMER);
							sc_start(NULL, bl, SC_NOEQUIPWEAPON, 100, skill_lv, d);
							break;
						case RG_STRIPSHIELD:
							status_change_end(bl, SC_PROTECTSHIELD, INVALID_TIMER);
							sc_start(NULL, bl, SC_NOEQUIPSHIELD, 100, skill_lv, d);
							break;
						case RG_STRIPARMOR:
							status_change_end(bl, SC_PROTECTARMOR, INVALID_TIMER );
							sc_start(NULL, bl, SC_NOEQUIPARMOR, 100, skill_lv, d);
							break;
						case RG_STRIPHELM:
							status_change_end(bl, SC_PROTECTHELM, INVALID_TIMER );
							sc_start(NULL, bl, SC_NOEQUIPHELM, 100, skill_lv, d);
							break;
					}
					clif->skill_nodamage(src,bl,skill_id,skill_lv,1);
					break;
				}
			}

			// Attempts to strip at rate i and duration d
			if ((rate = skill->strip_equip(bl, location, rate, skill_lv, d)) || (skill_id != ST_FULLSTRIP && skill_id != GC_WEAPONCRUSH))
				clif->skill_nodamage(src,bl,skill_id,skill_lv,rate);

			// Nothing stripped.
			if (sd && !rate)
				clif->skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			break;
		}
	}

	if (skill_id != SR_CURSEDCIRCLE) {
		struct status_change *sc = status->get_sc(src);
		if( sc && sc->data[SC_CURSEDCIRCLE_ATKER] )//Should only remove after the skill had been casted.
			status_change_end(src,SC_CURSEDCIRCLE_ATKER,INVALID_TIMER);
	}

	if (dstmd) { //Mob skill event for no damage skills (damage ones are handled in battle_calc_damage) [Skotlex]
		mob->log_damage(dstmd, src, 0); //Log interaction (counts as 'attacker' for the exp bonus)
		mob->skill_event(dstmd, src, tick, MSC_SKILLUSED|(skill_id<<16));
	}

	if (sd && !(flag&1)) { // ensure that the skill last-cast tick is recorded
		sd->canskill_tick = timer->gettick();

		if (sd->state.arrow_atk) { // consume arrow on last invocation to this skill.
			battle->consume_ammo(sd, skill_id, skill_lv);
		}
		skill->onskillusage(sd, bl, skill_id, tick);
		// perform skill requirement consumption
		if (skill_id != NC_SELFDESTRUCTION)
			skill->consume_requirement(sd,skill_id,skill_lv,2);
	}

	map->freeblock_unlock();
	if (hookS)
		hookStop();
	return 0;
}

HPExport void plugin_init (void)
{
	addHookPre(skill, castend_nodamage_id, skill_castend_nodamage_id_pre);
}

HPExport void server_online (void)
{
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
