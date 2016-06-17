//===== Hercules Plugin ======================================
//= CostumeItem
//===== By: ==================================================
//= Dastgir/Hercules
//= Mhalicot
//= Mr. [Hercules/Ind]
//===== Current Version: =====================================
//= 3.5a
//===== Description: =========================================
//= Converts an ordinary equipable to Costume Item
//===== Changelog: ===========================================
//= v1.0  - Initial Release [Mhalicot]
//= v1.0a - Fixed Typo (usage: @ci <item name/ID>) changed to 
//=          (usage: @costumeitem <item name/ID>) thx to DP
//= v2.0  - Converted Costume Items will now removed normal
//=          stats and Bonuses. [Mhalicot]
//= v3.0  - Item Combos will now Ignore Converted
//=          Costume Items. [Mhalicot]
//= v3.1  - Fixed HP/SP becomes 1/1 [Mhalicot]
//= v3.2  - Fixed Sinx Can't Equipt dagger/sword on both arms 
//=          (L/R), Special Thanks to Haru for Help [Mhalicot]
//= v3.3  - Fixed Error when compiling.
//= v3.4  - Fixed Error when compiling.
//= v3.4a - Updated According to new hercules.[Dastgir]
//= v3.5  - Updated Costume with new Hercules,
//=          some other changes. [Dastgir]
//= v3.5a - Attributes are no longer given. [Dastgir]
//===== Additional Comments: =================================
//= Reserved Costume ID(BattleConf):
//= (Should not conflict with CharID)
//= 	reserved_costume_id: ID
//===== Repo Link: ===========================================
//= https://github.com/dastgir/HPM-Plugins
//============================================================

#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/memmgr.h"
#include "common/timer.h"
#include "common/HPMi.h"
#include "common/mmo.h"
#include "map/itemdb.h"
#include "map/battle.h"
#include "map/script.h"
#include "map/status.h"
#include "map/clif.h"
#include "map/pet.h"
#include "map/map.h"
#include "map/mob.h"
#include "map/pc.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

#define cap_value(a, min, max) (((a) >= (max)) ? (max) : ((a) <= (min)) ? (min) : (a))

HPExport struct hplugin_info pinfo = {
	"costumeitem",
	SERVER_TYPE_MAP,
	"3.5a",
	HPM_VERSION,
};

static inline void status_cpy(struct status_data* a, const struct status_data* b)
{
	memcpy((void*)&a->max_hp, (const void*)&b->max_hp, sizeof(struct status_data)-(sizeof(a->hp)+sizeof(a->sp)));
}

// Costume System
int reserved_costume_id = INT_MAX-100; // Very High Number

void costume_id(const char *key, const char *val)
{
	if (strcmpi(key,"reserved_costume_id") == 0)
		reserved_costume_id = atoi(val);
}

int costume_id_return(const char *key)
{
	if (strcmpi(key,"reserved_costume_id") == 0)
		return reserved_costume_id;

	return 0;
}

uint16 GetWord(uint32 val, int idx)
{
	switch(idx) {
	case 0:
		return (uint16)( (val & 0x0000FFFF)         );
	case 1:
		return (uint16)( (val & 0xFFFF0000) >> 0x10 );
	default:
		ShowDebug("GetWord: invalid index (idx=%d)\n", idx);
		return 0;
	}
}

uint32 MakeDWord(uint16 word0, uint16 word1)
{
	return
		((uint32)(word0        )) |
		((uint32)(word1 << 0x10));
}

int alternate_item(int index)
{
	switch(index) {
		case EQP_HEAD_LOW:
			return EQP_COSTUME_HEAD_LOW;
		case EQP_HEAD_TOP:
			return EQP_COSTUME_HEAD_TOP;
		case EQP_HEAD_MID:
			return EQP_COSTUME_HEAD_MID;
		case EQP_HAND_R:
			return EQP_SHADOW_WEAPON;
		case EQP_HAND_L:
			return EQP_SHADOW_SHIELD;
		case EQP_ARMOR:
			return EQP_SHADOW_ARMOR;
		case EQP_SHOES:
			return EQP_SHADOW_SHOES;
		case EQP_GARMENT:
			return EQP_COSTUME_GARMENT;
		case EQP_ACC_L:
			return EQP_SHADOW_ACC_L;
		case EQP_ACC_R:
			return EQP_SHADOW_ACC_R;
		default:
			return -1;
	}
}

void script_stop_costume(struct map_session_data **sd_, struct item_data **data_, int *oid)
{
	struct item_data *data = *data_;
	struct map_session_data *sd = *sd_;
	if (data->equip <= EQP_HEAD_MID) {
		int alternate = alternate_item(data->equip);
		if (alternate != -1) {
			int equip_index = pc->checkequip(sd, alternate);
			if (equip_index < 0 || !sd->inventory_data[equip_index])
				return;
			if (sd->inventory_data[equip_index]->nameid == data->nameid)
				hookStop();
		}
	}
}

/**
 * called when a item with combo is worn
 */
int pc_checkcombo_mine(struct map_session_data *sd, struct item_data *data)
{	
	int i, j, k, z;
	int index, success = 0;
	struct pc_combos *combo;

	for (i = 0; i < data->combos_count; i++) {
		// ensure this isn't a duplicate combo
		if (sd->combos != NULL) {
			int x;
			
			ARR_FIND( 0, sd->combo_count, x, sd->combos[x].id == data->combos[i]->id );

			// found a match, skip this combo
			if( x < sd->combo_count )
				continue;
		}

		for (j = 0; j < data->combos[i]->count; j++) {
			int id = data->combos[i]->nameid[j];
			bool found = false;

			for (k = 0; k < EQI_MAX; k++) {
				index = sd->equip_index[k];
				if (index < 0)
					continue;
				if (k == EQI_HAND_R && sd->equip_index[EQI_HAND_L] == index)
					continue;
				if (k == EQI_HEAD_MID && sd->equip_index[EQI_HEAD_LOW] == index)
					continue;
				if (k == EQI_HEAD_TOP && (sd->equip_index[EQI_HEAD_MID] == index || sd->equip_index[EQI_HEAD_LOW] == index))
					continue;

				if ((int)MakeDWord(sd->status.inventory[index].card[2],sd->status.inventory[index].card[3]) == reserved_costume_id)
					continue;

				if (sd->inventory_data[index] == NULL)
					continue;
				
				if (itemdb_type(id) != IT_CARD) {
					if (sd->inventory_data[index]->nameid != id)
						continue;
					found = true;
					break;
				} else { //Cards
					if (sd->inventory_data[index]->slot == 0 || itemdb_isspecial(sd->status.inventory[index].card[0]))
						continue;

					for (z = 0; z < sd->inventory_data[index]->slot; z++) {
						if (sd->status.inventory[index].card[z] != id)
							continue;

						// We have found a match
						found = true;
						break;
					}
				}

			}

			if (found == false) // we haven't found all the ids for this combo, so we can return
				break; 
		}

		// we broke out of the count loop w/o finding all ids, we can move to the next combo
		if (j < data->combos[i]->count)
			continue;

		// All items in the combo are matching

		RECREATE(sd->combos, struct pc_combos, ++sd->combo_count);
		
		combo = &sd->combos[sd->combo_count - 1];
		
		combo->bonus = data->combos[i]->script;
		combo->id = data->combos[i]->id;
		
		success++;
	}

	return success;
}

void map_reqnickdb_pre(struct map_session_data **sd, int *char_id)
{

	if (*sd == NULL)
		return;

	if (reserved_costume_id && reserved_costume_id == *char_id) {
		clif->solved_charname((*sd)->fd, *char_id, "Costume");
		hookStop();
	}
	return;
}

int pc_equippoint_post(int retVal, struct map_session_data *sd, int n)
{ 
	int char_id = 0;

	if (!sd || !retVal)	// If the original function returned zero, we don't need to process it anymore
		return retVal;

	if( reserved_costume_id &&
		sd->status.inventory[n].card[0] == CARD0_CREATE &&
		(char_id = MakeDWord(sd->status.inventory[n].card[2],sd->status.inventory[n].card[3])) == reserved_costume_id )
	{ // Costume Item - Converted
		if (retVal&EQP_HEAD_TOP) {
			retVal &= ~EQP_HEAD_TOP;
			retVal |= EQP_COSTUME_HEAD_TOP;
		}
		if (retVal&EQP_HEAD_LOW) {
			retVal &= ~EQP_HEAD_LOW;
			retVal |= EQP_COSTUME_HEAD_LOW;
		}
		if (retVal&EQP_HEAD_MID) {
			retVal &= ~EQP_HEAD_MID;
			retVal |= EQP_COSTUME_HEAD_MID;
		}
	}
	return retVal;
}

ACMD(costumeitem)
{
	char item_name[100];
	int item_id, flag = 0;
	struct item item_tmp;
	struct item_data *item_data;

	if (sd == NULL)
		return false;

	if (!message || !*message || (
		sscanf(message, "\"%99[^\"]\"", item_name) < 1 && 
		sscanf(message, "%99s", item_name) < 1 )) {
 			clif->message(fd, "Please enter an item name or ID (usage: @costumeitem <item name/ID>).");
			return false;
	}

	if ((item_data = itemdb->search_name(item_name)) == NULL &&
	    (item_data = itemdb->exists(atoi(item_name))) == NULL) {
			clif->message(fd, "Invalid item ID or name.");
			return false;
	}

	if (!reserved_costume_id) {
		clif->message(fd, "Costume conversion is disabled.");
		return false;
	}
	if( !(item_data->equip&EQP_HEAD_LOW) &&
		!(item_data->equip&EQP_HEAD_MID) &&
		!(item_data->equip&EQP_HEAD_TOP) &&
		!(item_data->equip&EQP_COSTUME_HEAD_LOW) &&
		!(item_data->equip&EQP_COSTUME_HEAD_MID) &&
		!(item_data->equip&EQP_COSTUME_HEAD_TOP) ) {
			clif->message(fd, "You cannot costume this item. Costume only work for headgears.");
			return false;
		}

	item_id = item_data->nameid;
	// Check if it's stackable.
	if (!itemdb->isstackable2(item_data)) {
		if ((item_data->type == IT_PETEGG || item_data->type == IT_PETARMOR)) {
			clif->message(fd, "Cannot create costume pet eggs or pet armors.");
			return false;
		}
	}

	// if not pet egg
	if (!pet->create_egg(sd, item_id)) {
		memset(&item_tmp, 0, sizeof(item_tmp));
		item_tmp.nameid = item_id;
		item_tmp.identify = 1;
		item_tmp.card[0] = CARD0_CREATE;
		item_tmp.card[2] = GetWord(reserved_costume_id, 0);
		item_tmp.card[3] = GetWord(reserved_costume_id, 1);

		if ((flag = pc->additem(sd, &item_tmp, 1, LOG_TYPE_COMMAND)))
			clif->additem(sd, 0, 0, flag);
	}

	if (flag == 0)
		clif->message(fd,"item created.");
	return true;
}

/*==========================================
 * Costume Items Hercules/[Mhalicot]
 *------------------------------------------*/
BUILDIN(costume)
{
	int i = -1, num, ep;
	map_session_data *sd;

	num = script_getnum(st,2); // Equip Slot
	sd = script->rid2sd(st);

	if (sd == NULL)
		return 0;
	if (num > 0 && num <= ARRAYLENGTH(script->equip))
		i = pc->checkequip(sd, script->equip[num - 1]);

	if (i < 0)
		return 0;

	ep = sd->status.inventory[i].equip;

	if (!(ep&EQP_HEAD_LOW) && !(ep&EQP_HEAD_MID) && !(ep&EQP_HEAD_TOP))
		return 0;

	logs->pick_pc(sd, LOG_TYPE_SCRIPT, -1, &sd->status.inventory[i],sd->inventory_data[i]);
	pc->unequipitem(sd,i,2);
	clif->delitem(sd,i,1,3);
	// --------------------------------------------------------------------
	sd->status.inventory[i].refine = 0;
	sd->status.inventory[i].attribute = 0;
	sd->status.inventory[i].card[0] = CARD0_CREATE;
	sd->status.inventory[i].card[1] = 0;
	sd->status.inventory[i].card[2] = GetWord(reserved_costume_id, 0);
	sd->status.inventory[i].card[3] = GetWord(reserved_costume_id, 1);

	if (ep&EQP_HEAD_TOP) {
		ep &= ~EQP_HEAD_TOP;
		ep |= EQP_COSTUME_HEAD_TOP;
	}
	if (ep&EQP_HEAD_LOW) {
		ep &= ~EQP_HEAD_LOW;
		ep |= EQP_COSTUME_HEAD_LOW;
	}
	if (ep&EQP_HEAD_MID) {
		ep &= ~EQP_HEAD_MID;
		ep |= EQP_COSTUME_HEAD_MID;
	}
	// --------------------------------------------------------------------
	logs->pick_pc(sd, LOG_TYPE_SCRIPT, 1, &sd->status.inventory[i], sd->inventory_data[i]);

	clif->additem(sd, i, 1, 0);
	pc->equipitem(sd, i, ep);
	clif->misceffect(&sd->bl, 3);

	return true;
}

HPExport void server_preinit (void)
{
	addBattleConf("reserved_costume_id", costume_id,costume_id_return);
}

/* Server Startup */
HPExport void plugin_init (void) {
	
	pc->checkcombo = pc_checkcombo_mine;
	
	//Hook
	addHookPre(script, run_item_equip_script, script_stop_costume);
	addHookPre(script, run_item_unequip_script, script_stop_costume);
	addHookPre(script, run_use_script, script_stop_costume);
	addHookPre(map, reqnickdb, map_reqnickdb_pre);
	addHookPost(pc, equippoint, pc_equippoint_post);
	
	//atCommand
	addAtcommand("costumeitem", costumeitem);

	//scriptCommand
	addScriptCommand("costume", "i", costume);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}

