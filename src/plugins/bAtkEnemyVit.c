//===== Hercules Plugin ======================================
//= bonus2 bAtkEnemyVit
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Will increase Atk by y% if Enemy Vit is > x 
//===== Additional Comments: =================================
//= Format:
//= bonus2 bAtkEnemyVit, x, y;	// x = Vit, y = Atk%/100
//= e.g:
//=   bonus2 bAtkEnemyVit, 5, 1000; // Increase Atk by 10% if
//=                                     enemy vit >= 5.
//===== Repo Link: ===========================================
//= https://github.com/dastgir/HPM-Plugins
//===== Requested: ===========================================
//= http://herc.ws/board/topic/14024-how-to-get-the-vit-of-an-enemy
//============================================================

#include "common/hercules.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common/HPMi.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/memmgr.h"
#include "common/nullpo.h"
#include "common/mapindex.h"
#include "common/strlib.h"
#include "common/utils.h"
#include "common/cbasetypes.h"
#include "common/timer.h"
#include "common/showmsg.h"
#include "common/conf.h"
#include "common/db.h"
#include "common/sql.h"

#include "map/battle.h"
#include "map/map.h"
#include "map/pc.h"
#include "map/script.h"
#include "map/status.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"bAtkEnemyVit",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

/**
 * How many bAtkEnemyVit bonus can be
 * applied to a player?
 */
#define MAX_ATK_BONUS 10

/**
 * Bonus ID.
 * don't mess with this.
 * Dastgir/Hercules
 */
int bAtkEnemyVit = -1;

struct s_atk_vit {
	int vit;               // Enemy Vit
	int atk;               // Increase Atk by x%
};

struct s_bonus_data {
	struct s_atk_vit bonusAtk[MAX_ATK_BONUS];
};

struct s_bonus_data* bonus_search(struct map_session_data* sd){
	struct s_bonus_data *data = NULL;
	if ((data = getFromMSD(sd, 0)) == NULL) {
		CREATE(data, struct s_bonus_data, 1);
		memset(data->bonusAtk, 0, sizeof(data->bonusAtk));
		addToMSD(sd, data, 0, true);
	}
	return data;
}

int pc_bonus2_pre(struct map_session_data **sd, int *type, int *type2, int *val)
{
	if (*type == bAtkEnemyVit) {
		struct s_bonus_data *data = bonus_search(*sd);
		int i;

		if (*type2 <= 0) {
			ShowError("pc_bonus2_pre: Vit(%d) Required for bAtkEnemyVit is too low.\n", *type2);
			hookStop();
			return 0;
		}
		if (*val <= 0) {
			ShowError("pc_bonus2_pre: Atk Increase for bAtkEnemyVit cannot be 0.\n");
			hookStop();
			return 0;
		}

		// Find free Slot
		for (i = 0; i < MAX_ATK_BONUS; i++) {
			// If Vit or Atk is 0, that means slot is free
			if (data->bonusAtk[i].vit == 0 || data->bonusAtk[i].atk == 0) {
				data->bonusAtk[i].vit = *type2;
				data->bonusAtk[i].atk = *val;
				break;
			}
		}
		if (i == MAX_ATK_BONUS) {
			ShowError("pc_bonus2_pre: bAtkEnemyVit Reached %d Slots. No more Free slots for Player.", MAX_ATK_BONUS);
		}

		hookStop();
	}

	return 0;
}

// Reset on Recalc
int status_calc_pc_pre(struct map_session_data **sd, enum e_status_calc_opt *opt)
{
	struct s_bonus_data *data = NULL;
	if ((data = getFromMSD(*sd, 0)) != NULL) {
		memset(data->bonusAtk, 0, sizeof(data->bonusAtk));	// Reset Structure
	}
	return 1;
}

int64 battle_calc_weapon_damage_post(int64 retVal, struct block_list *src, struct block_list *bl, uint16 skill_id, uint16 skill_lv, struct weapon_atk *watk, int nk, bool n_ele, short s_ele, short s_ele_, int size, int type, int flag, int flag2)
{
#ifdef RENEWAL
	int i, vit;
	struct s_bonus_data *data;
	struct map_session_data *sd;

	if (src == NULL || src->type != BL_PC || bl == NULL || retVal == 0)
		return retVal;

	// Store SD
	sd = BL_CAST(BL_PC, src);

	// Get Bonus Data
	data = bonus_search(sd);

	// Store vit
	vit = status_get_vit(bl);

	for (i = 0; i < MAX_ATK_BONUS; i++) {
		// No more Atk Increase
		if (data->bonusAtk[i].vit == 0 || data->bonusAtk[i].atk == 0) {
			break;
		}
		// Enemy Vit is >= Vit in ItemBonus, then increase atk
		if (vit >= data->bonusAtk[i].vit) {
			retVal += retVal * data->bonusAtk[i].atk / 10000;
		}
	}
	return retVal;
#else
	return 0;
#endif
}

// Increase Atk
struct Damage battle_calc_weapon_attack_post(struct Damage wd, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, int wflag)
{
#ifndef RENEWAL
	struct map_session_data *sd;
	struct status_data *sstatus, *tstatus;
	struct status_change *sc, *tsc;
	struct {
		unsigned rh : 1;     ///< Attack considers right hand (wd.damage)
		unsigned lh : 1;     ///< Attack considers left hand (wd.damage2)
		unsigned hit : 1;    ///< the attack Hit? (not a miss)
		unsigned infdef : 1; ///< Infinite defense (plants)
	} flag;
	int nk;
	struct s_bonus_data *data = NULL;

	if (src == NULL || target == NULL || src->type != BL_PC) {
		return wd;
	}

	// These SkillID's won't increase AtkRate.
	switch(skill_id) {
		case PA_SACRIFICE:
		case NJ_ISSEN:
		case NJ_SYURIKEN:
		case LK_SPIRALPIERCE:
		case ML_SPIRALPIERCE:
		case CR_SHIELDBOOMERANG:
		case HFLI_SBR44:
			return wd;
	}

	// Get Source Player and Status.
	sd = BL_CAST(BL_PC, src);
	sc = status->get_sc(src);
	tsc = status->get_sc(target);
	sstatus = status->get_status_data(src);
	tstatus = status->get_status_data(target);

	// No Bonus Set.
	if ((data = getFromMSD(sd, 0)) == NULL) {
		return wd;
	}

	// Set Flags
	flag.rh = 1;
	if (!skill_id) { //Skills ALWAYS use ONLY your right-hand weapon (tested on Aegis 10.2)
		if (sd && sd->weapontype1 == 0 && sd->weapontype2 > 0) {
			flag.rh = 0;
			flag.lh = 1;
		}
		if (sstatus->lhw.atk) {
			flag.lh = 1;
		}
	}
	// Infinite Damage Flag
	flag.infdef = (tstatus->mode&MD_PLANT && skill_id != RA_CLUSTERBOMB?1:0);
	nk = skill->get_nk(skill_id);
	if(!skill_id && wflag) // If flag, this is splash damage from Baphomet Card and it always hits.
		nk |= NK_NO_CARDFIX_ATK|NK_IGNORE_FLEE;
	// Hit Flag
	flag.hit = (nk&NK_IGNORE_FLEE) ? 1 : 0;

	// Infinite Defense don't need attack increase.
	if (flag.infdef) {
		return wd;
	}

	// Check if InfDef is set to 1 somewhere
	if (!flag.infdef && target->type == BL_SKILL) {
		const struct skill_unit *su = BL_UCCAST(BL_SKILL, target);
		if (su->group->unit_id == UNT_REVERBERATION) {
			return wd;	// infdef = 1, we don't need infdef = 1
		}
	}

	// Conditions for hit flag.
	if (!flag.hit) {
		// Critical Attack would result in hit.
		if (wd.type == BDT_CRIT) {
			flag.hit = 1;
		} else {
			// Perfect Hit Check Missing
			// SC_FUSION always hit
			if (sc && sc->data[SC_FUSION]) {
				flag.hit = 1;
			}
			switch(skill_id) {
				case AS_SPLASHER:
					// Always hits the one exploding.
					if (!wflag) {
						flag.hit = 1;
					}
					break;
				case CR_SHIELDBOOMERANG:
					if (sc && sc->data[SC_SOULLINK] && sc->data[SC_SOULLINK]->val2 == SL_CRUSADER) {
						flag.hit = 1;
					}
					break;
			}
		if (tsc && !flag.hit && tsc->opt1 && tsc->opt1 != OPT1_STONEWAIT && tsc->opt1 != OPT1_BURNING)
			flag.hit = 1;
		}
		// Flee would not result in hit
		if (wd.dmg_lv == ATK_FLEE) {
			// Special Hit
			if (skill_id == SR_GATEOFHELL) {
				flag.hit = 1;
			}
		} else {
			flag.hit = 1;
		}
	}
	// Flee2 Rate succeed
	if (wd.dmg_lv == ATK_LUCKY && wd.type == BDT_PDODGE) {
		return wd;
	}
	ShowDebug("BC: 9\n");
#define ATK_RATE(a) do { int64 temp__ = (a); wd.damage += wd.damage*temp__/10000 ; if(flag.lh) wd.damage2 += wd.damage2*temp__/10000; } while(0)

	// No Math for Plants
	if (sd != NULL && flag.hit && !flag.infdef) {
		// Store vit
		int vit = status_get_vit(target), i;

		for (i = 0; i < MAX_ATK_BONUS; i++) {
			// No more Atk Increase
			if (data->bonusAtk[i].vit == 0 || data->bonusAtk[i].atk == 0) {
				break;
			}
			// Enemy Vit is >= Vit in ItemBonus, then increase atk
			if (vit >= data->bonusAtk[i].vit) {
				ATK_RATE(data->bonusAtk[i].atk);
			}
		}
	}
#undef ATK_RATE
#endif
	return wd;
}

HPExport void plugin_init(void)
{
    bAtkEnemyVit = map->get_new_bonus_id();
    script->set_constant("bAtkEnemyVit", bAtkEnemyVit, false, false);

    addHookPre(pc, bonus2, pc_bonus2_pre);
    addHookPre(status, calc_pc_, status_calc_pc_pre);
    addHookPost(battle, calc_weapon_damage, battle_calc_weapon_damage_post);
    addHookPost(battle, calc_weapon_attack, battle_calc_weapon_attack_post);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
	
}
