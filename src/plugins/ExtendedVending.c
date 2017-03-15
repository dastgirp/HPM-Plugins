/**
 * Extended Vending System
 * Please Check conf/ExtendedVending.conf, db/item_vending.txt, db/item_db2.conf
 * By Dastgir/Hercules
 */

/*
Add Following Items to ClientSide:

idnum2itemdisplaynametable.txt
	30000#Zeny#
	30001#Cash#

idnum2itemresnametable.txt
	30000#±ÝÈ#
	30001#¹Ì½º¸±È­#
*/

#include "common/hercules.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "common/HPMi.h"
#include "common/utils.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/memmgr.h"
#include "common/timer.h"
#include "common/ers.h"
#include "common/nullpo.h"
#include "common/strlib.h"
#include "common/sql.h"

#include "map/battle.h"
#include "map/clif.h"
#include "map/pc.h"
#include "map/map.h"
#include "map/mob.h"
#include "map/skill.h"
#include "map/atcommand.h"
#include "map/itemdb.h"
#include "map/vending.h"
#include "map/intif.h"
#include "map/chrif.h"
#include "map/path.h"
#include "map/pet.h"
#include "map/homunculus.h"
#include "map/status.h"
#include "map/searchstore.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h" /* should always be the last file included! (if you don't make it last, it'll intentionally break compile time) */

HPExport struct hplugin_info pinfo =
{
	"Extended Vending System",	// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

#define VEND_COLOR 0x00FFFF // Cyan

struct s_ext_vend{
	int itemid;
};
struct s_ext_vend ext_vend[MAX_INVENTORY];

struct player_data {
	int vend_loot;
	int vend_lvl;
};

struct autotrade_data {
	int vend_loot;
};

int bc_extended_vending;
int bc_show_item_vending;
int bc_ex_vending_info;
int bc_item_zeny;
int bc_item_cash;

//Clif Edits
void clif_vend_message(struct map_session_data *sd, const char* msg, uint32 color);
void clif_vend_message(struct map_session_data *sd, const char* msg, uint32 color)
{
	int fd;
	unsigned short len = strlen(msg) + 1;
	
	nullpo_retv(sd);
	
	color = (color & 0x0000FF) << 16 | (color & 0x00FF00) | (color & 0xFF0000) >> 16; // RGB to BGR
	
	fd = sd->fd;
	WFIFOHEAD(fd, len+12);
	WFIFOW(fd,0) = 0x2C1;
	WFIFOW(fd,2) = len+12;
	WFIFOL(fd,4) = 0;
	WFIFOL(fd,8) = color;
	memcpy(WFIFOP(fd,12), msg, len);
	WFIFOSET(fd, WFIFOW(fd,2));
}

//Skill_Vending
int skill_vending_ev( struct map_session_data *sd, int nameid);
int skill_vending_ev( struct map_session_data *sd, int nameid) {
	struct item_data *item;
	struct player_data *ssd;
	char output[1024];
	int i;
	
	nullpo_ret(sd);


	if (nameid <= 0) {
		clif->skill_fail(sd, MC_VENDING, USESKILL_FAIL_LEVEL, 0);
		return 0;
	}
	
	if (nameid > MAX_ITEMDB) {
		return 0;
	}

	if (nameid != bc_item_zeny && nameid != bc_item_cash) {
		ARR_FIND(0, MAX_INVENTORY, i, ext_vend[i].itemid == nameid);
		if (i == MAX_INVENTORY)
			return 0;
	}

	if (named )
	item = itemdb->exists(nameid);
	if (item == NULL) {
		return 0;
	}

	if (!( ssd = getFromMSD(sd,0))) {
		CREATE(ssd, struct player_data, 1);
		ssd->vend_loot = 0;
		ssd->vend_lvl = 0;
		addToMSD(sd, ssd, 0, true);
	}

	ssd->vend_loot = nameid;
	
	sprintf(output,"You have selected: %s", item->jname);
	clif_vend_message(sd, output, VEND_COLOR);

	if (!pc_can_give_items(sd)){
		clif->skill_fail(sd,MC_VENDING,USESKILL_FAIL_LEVEL,0);
	} else {
		sd->state.prevend = 1;
		clif->openvendingreq(sd,2+ssd->vend_lvl);
	}
	
	return 0;
}


//Had to duplicate
void assert_report(const char *file, int line, const char *func, const char *targetname, const char *title) {
	if (file == NULL)
		file = "??";
	
	if (func == NULL || *func == '\0')
		func = "unknown";
	
	ShowError("--- %s --------------------------------------------\n", title);
	ShowError("%s:%d: '%s' in function `%s'\n", file, line, targetname, func);
	ShowError("--- end %s ----------------------------------------\n", title);
}

void ev_bc(const char *key, const char *val) {
	if (strcmpi(key,"battle_configuration/extended_vending") == 0) {
		bc_extended_vending = config_switch(val);
		if (bc_extended_vending > 1 || bc_extended_vending < 0){
			ShowDebug("Wrong Value for extended_vending: %d\n",config_switch(val));
			bc_extended_vending = 0;
		}
	} else if (strcmpi(key,"battle_configuration/show_item_vending") == 0) {
		bc_show_item_vending = config_switch(val);
		if (bc_show_item_vending > 1 || bc_show_item_vending < 0){
			ShowDebug("Wrong Value for show_item_vending: %d\n",config_switch(val));
			bc_extended_vending = 0;
		}
	} else if (strcmpi(key,"battle_configuration/ex_vending_info") == 0) {
		bc_ex_vending_info = config_switch(val);
		if (bc_ex_vending_info>1 || bc_ex_vending_info<0){
			ShowDebug("Wrong Value for ex_vending_info: %d\n",config_switch(val));
			bc_extended_vending = 0;
		}
	} else if (strcmpi(key,"battle_configuration/item_zeny") == 0) {
		bc_item_zeny = config_switch(val);
		if (bc_item_zeny != 0 && bc_item_zeny > MAX_ITEMDB ){
			ShowDebug("Wrong Value for item_zeny: %d\n",config_switch(val));
			bc_extended_vending = 0;
		}
	} else if (strcmpi(key,"battle_configuration/item_cash") == 0) {
		bc_item_cash = config_switch(val);
		if (bc_item_cash != 0 && bc_item_cash > MAX_ITEMDB ){
			ShowDebug("Wrong Value for item_cash: %d\n",config_switch(val));
			bc_extended_vending = 0;
		}
	}
	return;
}

int ev_return_bc(const char *key)
{
	if (strcmpi(key,"battle_configuration/extended_vending") == 0) {
		return bc_extended_vending;
	} else if (strcmpi(key,"battle_configuration/show_item_vending") == 0) {
		return bc_show_item_vending;
	} else if (strcmpi(key,"battle_configuration/ex_vending_info") == 0) {
		return bc_ex_vending_info;
	} else if (strcmpi(key,"battle_configuration/item_zeny") == 0) {
		return bc_item_zeny;
	} else if (strcmpi(key,"battle_configuration/item_cash") == 0) {
		return bc_item_cash;
	}
	return 0;
}

//Clif Edits
void clif_parse_SelectArrow_pre(int *fd,struct map_session_data **sd)
{
	if (pc_istrading(*sd)) {
	//Make it fail to avoid shop exploits where you sell something different than you see.
		clif->skill_fail(*sd,(*sd)->ud.skill_id,USESKILL_FAIL_LEVEL,0);
		clif_menuskill_clear(*sd);
		return;
	}
	switch( (*sd)->menuskill_id ) {
		case MC_VENDING: // Extended Vending system 
			skill_vending_ev(*sd, RFIFOW(*fd,2));
			clif_menuskill_clear(*sd);
			hookStop();
			break;
	}
	return;
}

void clif_parse_OpenVending_pre(int *fd, struct map_session_data **sd_) {
	int fd2 = *fd;
	short len = (short)RFIFOW(fd2,2) - 85;
	const char* mes_orig = (const char*)RFIFOP(fd2,4);
	bool flag = (bool)RFIFOB(fd2,84);
	const uint8* data = (const uint8*)RFIFOP(fd2,85);
	char message[1024];
	struct player_data* ssd;
	struct item_data *item;
	struct map_session_data *sd = *sd_;
	
	if (!( ssd = getFromMSD(sd,0))) {
		CREATE( ssd, struct player_data, 1 );
		ssd->vend_loot = 0;
		ssd->vend_lvl = 0;
		addToMSD( sd, ssd, 0, true );
	}
	item = itemdb->exists(ssd->vend_loot);

	if ((bc_extended_vending == 1) && (bc_show_item_vending==1) && ssd->vend_loot){
		memset(message, '\0', sizeof(message));
		strcat(strcat(strcat(strcat(message,"["),item->jname),"] "),mes_orig);
	}

	
	if ( !flag )
		sd->state.prevend = sd->state.workinprogress = 0;

	if ( sd->sc.data[SC_NOCHAT] && sd->sc.data[SC_NOCHAT]->val1&MANNER_NOROOM )
		return;
	if ( map->list[sd->bl.m].flag.novending ) {
		clif->message (sd->fd, msg_txt(276)); // "You can't open a shop on this map"
		return;
	}
	if ( map->getcell(sd->bl.m,&sd->bl,sd->bl.x,sd->bl.y,CELL_CHKNOVENDING) ) {
		clif->message (sd->fd, msg_txt(204)); // "You can't open a shop on this cell."
		return;
	}

	if ( message[0] == '\0' ) // invalid input
		return;
	if ((bc_extended_vending == 1) && (bc_show_item_vending == 1) && ssd->vend_loot){
		vending->open(sd, message, data, len/8);
	}else{
		vending->open(sd, mes_orig, data, len/8);
	}

}

int clif_vend(struct map_session_data *sd, int skill_lv) {
	struct item_data *item;
	int c, i, d = 0;
	int fd;
	nullpo_ret(sd);
	fd = sd->fd;
	WFIFOHEAD(fd, 8 * 8 + 8);
	WFIFOW(fd,0) = 0x1ad;
	if (bc_item_zeny){
		WFIFOW(fd, d * 2 + 4) = bc_item_zeny;
		d++;
	}
	if (bc_item_cash){
		WFIFOW(fd, d * 2 + 4) = bc_item_cash;
		d++;
	}
	for( c = d, i = 0; i < ARRAYLENGTH(ext_vend); i ++ ) {
		if ((item = itemdb->exists(ext_vend[i].itemid)) != NULL && 
			(int)item->nameid != bc_item_zeny && (int)item->nameid != bc_item_cash){
		WFIFOW(fd, c * 2 + 4) = (int)item->nameid;
			c++;
		}
	}
	if ( c > 0 ) {
		sd->menuskill_id = MC_VENDING;
		sd->menuskill_val = skill_lv;
		WFIFOW(fd,2) = c * 2 + 4;
		WFIFOSET(fd, WFIFOW(fd, 2));
	} else {
		clif->skill_fail(sd,MC_VENDING,USESKILL_FAIL_LEVEL,0);
		return 0;
	}

	return 1;
}

/**
 * Extended Vending system 
 **/
static bool itemdb_read_vendingdb(char* fields[], int columns, int current)
{
	struct item_data* id;
	int nameid;
 
	nameid = atoi(fields[0]);
 
	if ( ( id = itemdb->exists(nameid) ) == NULL )
	{
		ShowWarning("itemdb_read_vending: Invalid item id %d.\n", nameid);
		return false;
	}

	if ( id->type == IT_ARMOR || id->type == IT_WEAPON )
	{
		ShowWarning("itemdb_read_vending: item id %d cannot be equipment or weapon.\n", nameid);
		return false;
	}

	ext_vend[current].itemid = nameid;

	return true;
}

//ItemDB.c
void itemdb_read_post(bool minimal) {
	if (minimal) return;
	sv->readdb(map->db_path, "item_vending.txt", ',', 1, 1, ARRAYLENGTH(ext_vend), itemdb_read_vendingdb);
	return;
}
//Skill.c
int skill_castend_nodamage_id_pre(struct block_list **src_, struct block_list **bl_, uint16 *skill_id2, uint16 *skill_lv2, int64 *tick2, int *flag2){
	uint16 skill_id = *skill_id2;
	uint16 skill_lv = *skill_lv2;
	struct map_session_data *sd;
	struct player_data* ssd;
	struct block_list *src = *src_, *bl = *bl_;
	if (skill_id > 0 && !skill_lv)
		return 0;	// celest


	nullpo_retr(1, src);
	nullpo_retr(1, bl);


	if (src->m != bl->m)
		return 1;


	sd = BL_CAST(BL_PC, src);
	if (sd == NULL)
		return 1;
	if ( !( ssd = getFromMSD(sd,0) ) ) {
		CREATE( ssd, struct player_data, 1 );
		ssd->vend_loot = 0;
		ssd->vend_lvl = 0;
		addToMSD( sd, ssd, 0, true );
	}

	//dstsd deleted
	if (bl->prev == NULL)
		return 1;
	if (status->isdead(src))
		return 1;
	switch(skill_id){
		case MC_VENDING:
			if (sd){
				if ( !pc_can_give_items(sd) ) //Prevent vending of GMs with unnecessary Level to trade/drop. [Skotlex]
					clif->skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				else { // Extended Vending system 
					if (bc_extended_vending == 1){
						struct item_data *item;
						char output[1024];

						int c = 0, i, d = 0;
					
						ssd->vend_lvl = (int)skill_lv;
						if (bc_item_zeny)
							d++;
						if (bc_item_cash)
							d++;
						for ( c = d, i = 0; i < ARRAYLENGTH(ext_vend); i ++ ) {
							if ( (item = itemdb->exists(ext_vend[i].itemid)) != NULL && 
								item->nameid != bc_item_zeny && item->nameid != bc_item_cash)
								c++;
						}
					
						if ( c > 1 ){
							clif_vend(sd,ssd->vend_lvl);
						} else {
							sd->state.prevend = 1;
							if ( c ) {
								item = itemdb->exists(bc_item_zeny?bc_item_zeny:bc_item_cash?bc_item_cash:ext_vend[0].itemid);
								ssd->vend_loot = item->nameid;
								sprintf(output,"Current Currency: %s",itemdb_jname(ssd->vend_loot));
								clif->messagecolor_self(sd->fd,COLOR_WHITE,output);
								clif->openvendingreq(sd,2+ssd->vend_lvl);
							} else {
								ssd->vend_loot = 0;
								clif->openvendingreq(sd,2+ssd->vend_lvl);
							}
						}
						hookStop();
					} else {
						ssd->vend_loot = 0;
						sd->state.prevend = sd->state.workinprogress = 3;
						clif->openvendingreq(sd,2+skill_lv);
					}
				}
			}
			break;
	}
	return 1;
}

void vending_list_pre(struct map_session_data **sd, unsigned int *id2) {
	unsigned int id = *id2;
	struct map_session_data *vsd;
	struct player_data *ssd;
	char output[1024];
	int vend_loot = 0;
	nullpo_retv(*sd);

	if ((vsd = map->id2sd(id)) == NULL)
		return;
	if (!vsd->state.vending)
		return; // not vending
	ssd = getFromMSD(vsd, 0);
	if (ssd) {
		vend_loot = ssd->vend_loot;
	}
	
	if ( !pc_can_give_items(*sd) || !pc_can_give_items(vsd) ) { //check if both GMs are allowed to trade
		// GM is not allowed to trade
		if ((*sd)->lang_id >= atcommand->max_message_table)
			clif->message((*sd)->fd, atcommand->msg_table[0][246]);
		else
			clif->message((*sd)->fd, atcommand->msg_table[(*sd)->lang_id][246]);
		return;
	} 

	if ( bc_extended_vending == 1 && vend_loot ) {
		sprintf(output,"You've opened %s's shop. Sale is carried out: %s",vsd->status.name, itemdb_jname(vend_loot));
		clif->messagecolor_self((*sd)->fd,COLOR_WHITE,output);
	}
}

void vending_purchasereq_mod(struct map_session_data **sd_, int *aid2, unsigned int *uid2, const uint8 **data_, int *count2) {
	int aid = *aid2;
	unsigned int uid = *uid2;
	int count = *count2;
	int i, j, cursor, w, new_ = 0, blank, vend_list[MAX_VENDING];
	double z;
	struct s_vending vend[MAX_VENDING]; // against duplicate packets
	struct map_session_data* vsd = map->id2sd(aid);
	struct player_data* ssd;
	struct map_session_data *sd = *sd_;
	int vend_loot = 0;
	const uint8 *data = *data_;

	nullpo_retv(sd);
	if (vsd == NULL || !vsd->state.vending || vsd->bl.id == sd->bl.id)
		return; // invalid shop
	
	ssd = getFromMSD(vsd, 0);
	if (ssd)
		vend_loot = ssd->vend_loot;
	
	if ( vsd->vender_id != uid ) { // shop has changed
		clif->buyvending(sd, 0, 0, 6);  // store information was incorrect
		return;
	}

	if (!searchstore->queryremote(sd, aid) &&
		(sd->bl.m != vsd->bl.m || !check_distance_bl(&sd->bl, &vsd->bl, AREA_SIZE)))
		return; // shop too far away

	searchstore->clearremote(sd);

	if ( count < 1 || count > MAX_VENDING || count > vsd->vend_num )
		return; // invalid amount of purchased items

	blank = pc->inventoryblank(sd); //number of free cells in the buyer's inventory
	// duplicate item in vending to check hacker with multiple packets
	memcpy(&vend, &vsd->vending, sizeof(vsd->vending)); // copy vending list

	// some checks
	z = 0.; // zeny counter
	w = 0;  // weight counter
	for( i = 0; i < count; i++ ) {
		short amount = *(const uint16*)(data + 4*i + 0);
		short idx	= *(const uint16*)(data + 4*i + 2);
		idx -= 2;

		if ( amount <= 0 )
			return;

		// check of item index in the cart
		if ( idx < 0 || idx >= MAX_CART )
			return;

		ARR_FIND( 0, vsd->vend_num, j, vsd->vending[j].index == idx );
		if ( j == vsd->vend_num )
			return; //picked non-existing item
		else
			vend_list[i] = j;

		z += ((double)vsd->vending[j].value * (double)amount);
		/**
		 * Extended Vending system 
		 **/
		ShowDebug("i:%d, count:%d, amount:%d, z:%d, loot:%d\n", ii, count, amount, z, vend_loot);
		ShowDebug("Configs: %d:%d:%d:%d:%d\n", bc_extended_vending, bc_item_zeny, bc_item_cash, bc_ex_vending_info, bc_show_item_vending);	
		if ( bc_extended_vending == 1 ){
			if ( vend_loot == bc_item_zeny || !vend_loot ) {
				if ( z > (double)sd->status.zeny || z < 0. || z > (double)MAX_ZENY ) {
					//clif_buyvending(sd, idx, amount, 1); // you don't have enough zeny
					hookStop();
					return;
				}
				if ( z + (double)vsd->status.zeny > (double)MAX_ZENY && !battle->bc->vending_over_max ) {
					clif->buyvending(sd, idx, vsd->vending[j].amount, 4); // too much zeny = overflow
					hookStop();
					return;
		
				}
			} else if (vend_loot == bc_item_cash){
				if ( z > sd->cashPoints || z < 0. || z > (double)MAX_ZENY ) {
					clif->messagecolor_self(sd->fd,COLOR_WHITE,"You do not have enough CashPoint");
					hookStop();
					return;
				}
			} else {
				int k, loot_count = 0, vsd_w = 0;
				for (k = 0; k < MAX_INVENTORY; k++) {
					if (sd->status.inventory[k].nameid == vend_loot) {
						loot_count += sd->status.inventory[k].amount;
					}
				}
						
				if ( z > loot_count || z < 0) {
					clif->messagecolor_self(sd->fd,COLOR_WHITE,"You do not have enough items");
					hookStop();
					return;
				}
				if ( pc->inventoryblank(vsd) <= 0 ) {
					clif->messagecolor_self(sd->fd,COLOR_WHITE,"Seller has not enough space in your inventory");
					hookStop();
					return;
				}
				vsd_w += itemdb_weight(vend_loot) * (int)z;
				if ( vsd_w + vsd->weight > vsd->max_weight ) {
					clif->messagecolor_self(sd->fd,COLOR_WHITE,"Seller can not take all the item");
					hookStop();
					return;
				} 
			}
		} else {
			if ( z > (double)sd->status.zeny || z < 0. || z > (double)MAX_ZENY ) {
				clif->buyvending(sd, idx, amount, 1); // you don't have enough zeny
				hookStop();
				return;
			}
			if ( z + (double)vsd->status.zeny > (double)MAX_ZENY && !battle->bc->vending_over_max ) {
				clif->buyvending(sd, idx, vsd->vending[j].amount, 4); // too much zeny = overflow
				hookStop();
				return;
			}

		}
		w += itemdb_weight(vsd->status.cart[idx].nameid) * amount;
		if ( w + sd->weight > sd->max_weight ) {
			clif->buyvending(sd, idx, amount, 2); // you can not buy, because overweight
			hookStop();
			return;
		}
		
		//Check to see if cart/vend info is in sync.
		if ( vend[j].amount > vsd->status.cart[idx].amount )
			vend[j].amount = vsd->status.cart[idx].amount;
		
		// if they try to add packets (example: get twice or more 2 apples if marchand has only 3 apples).
		// here, we check cumulative amounts
		if ( vend[j].amount < amount ) {
			// send more quantity is not a hack (an other player can have buy items just before)
			clif->buyvending(sd, idx, vsd->vending[j].amount, 4); // not enough quantity
			hookStop();
			return;
		}
		
		vend[j].amount -= amount;

		switch( pc->checkadditem(sd, vsd->status.cart[idx].nameid, amount) ) {
			case ADDITEM_EXIST:
				break;	//We'd add this item to the existing one (in buyers inventory)
			case ADDITEM_NEW:
				new_++;
				if (new_ > blank) {
					hookStop();
					return; //Buyer has no space in his inventory
				}
				break;
			case ADDITEM_OVERAMOUNT:
				hookStop();
				return; //too many items
		}
	}
/**
 * Extended Vending system 
 **/
	if (bc_extended_vending == 1) {
		if ( vend_loot == bc_item_zeny || !vend_loot ) {
		
			//Logs (V)ending Zeny [Lupus]
		
			pc->payzeny(sd, (int)z, LOG_TYPE_VENDING, vsd);
			if ( battle->bc->vending_tax )
				z -= z * (battle->bc->vending_tax/10000.);
			pc->getzeny(vsd, (int)z, LOG_TYPE_VENDING, sd);

		} else if ( vend_loot == bc_item_cash ) {
			pc->paycash(sd,(int)z,0);
			pc->getcash(vsd,(int)z,0);
		} else {
			for( i = 0; i < MAX_INVENTORY; i++)
				if ( sd->status.inventory[i].nameid == vend_loot ) {
					struct item *item;
					item = &sd->status.inventory[i];
					pc->additem(vsd,item,(int)z, LOG_TYPE_VENDING);
				}
			pc->delitem(sd,pc->search_inventory(sd, vend_loot),(int)z,0,6, LOG_TYPE_VENDING);
			
		}
	} else {
		//Logs (V)ending Zeny [Lupus]
			pc->payzeny(sd, (int)z, LOG_TYPE_VENDING, vsd);
		if ( battle->bc->vending_tax )
			z -= z * (battle->bc->vending_tax/10000.);
		pc->getzeny(vsd, (int)z, LOG_TYPE_VENDING, sd);
	}


	for (i = 0; i < count; i++) {
		short amount = *(const uint16*)(data + 4*i + 0);
		short idx	= *(const uint16*)(data + 4*i + 2);
		const char *item_name = itemdb_jname(vsd->status.cart[idx].nameid);
		double rev = 0.;
		idx -= 2;

		if ( bc_ex_vending_info ) // Extended Vending system 
			rev = ((double)vsd->vending[vend_list[i]].value * (double)amount);
		
		// vending item
		pc->additem(sd, &vsd->status.cart[idx], amount, LOG_TYPE_VENDING);
		vsd->vending[vend_list[i]].amount -= amount;
		pc->cart_delitem(vsd, idx, amount, 0, LOG_TYPE_VENDING);
		clif->vendingreport(vsd, idx, amount, sd->status.char_id, (int)z);

		//print buyer's name
		if ( battle->bc->buyer_name ) {
			char temp[256];
			if ( bc_ex_vending_info ) { // Extended Vending system 
				
				sprintf(temp, "%s has bought '%s' in the amount of %d. Revenue: %d %s", sd->status.name, item_name, amount, (int)(rev -= rev * (battle->bc->vending_tax/10000.)), vend_loot?itemdb_jname(vend_loot):"Zeny");
			} else {
				if (sd->lang_id >= atcommand->max_message_table)
					sprintf(temp, atcommand->msg_table[0][265], sd->status.name);
				else
					sprintf(temp, atcommand->msg_table[sd->lang_id][265], sd->status.name);
			}
			clif_disp_onlyself(vsd, temp);
		}
	}

	if ( bc_ex_vending_info ) { // Extended Vending system 
		char temp[256];
		sprintf(temp, "Full revenue from %s is %d %s", sd->status.name, (int)z, vend_loot?itemdb_jname(vend_loot):"Zeny");
		clif_disp_onlyself(vsd, temp);
	}

	// compact the vending list
	for( i = 0, cursor = 0; i < vsd->vend_num; i++ ) {
		if ( vsd->vending[i].amount == 0 )
			continue;
		
		if ( cursor != i ) { // speedup
			vsd->vending[cursor].index = vsd->vending[i].index;
			vsd->vending[cursor].amount = vsd->vending[i].amount;
			vsd->vending[cursor].value = vsd->vending[i].value;
		}

		cursor++;
	}
	vsd->vend_num = cursor;

	//Always save BOTH: buyer and customer
	if ( map->save_settings&2 ) {
		chrif->save(sd,0);
		chrif->save(vsd,0);
	}

	//check for @AUTOTRADE users [durf]
	if ( vsd->state.autotrade ) {
		//see if there is anything left in the shop
		ARR_FIND( 0, vsd->vend_num, i, vsd->vending[i].amount > 0 );
		if ( i == vsd->vend_num ) {
			//Close Vending (this was automatically done by the client, we have to do it manually for autovenders) [Skotlex]
			vending->close(vsd);
			map->quit(vsd); //They have no reason to stay around anymore, do they?
		}
	}
	if (bc_extended_vending){
		hookStop();
	}
}

void pc_autotrade_prepare_pre(struct map_session_data **sd) {
	struct player_data *ssd;
	ssd = getFromMSD(*sd, 0);
	if (ssd){
		if (SQL_ERROR == SQL->Query(map->mysql_handle, "INSERT INTO `evs_info` (`vend_loot`,`vend_lvl`,`char_id`) VALUES ('%d','%d','%d')ON DUPLICATE KEY UPDATE `vend_lvl`='%d', `vend_loot`='%d'",
										ssd->vend_loot,
										ssd->vend_lvl,
										(*sd)->status.char_id,
										ssd->vend_lvl,
										ssd->vend_loot
										))
			Sql_ShowDebug(map->mysql_handle);
	}
}

void pc_autotrade_populate_pre(struct map_session_data **sd) {
	struct player_data *ssd;
	char *mdata = NULL;
	
	if (!( ssd = getFromMSD(*sd,0))) {
		CREATE(ssd, struct player_data, 1);
		ssd->vend_loot = 0;
		ssd->vend_lvl = 0;
		addToMSD(*sd, ssd, 0, true);
	}
	if (SQL_ERROR == SQL->Query(map->mysql_handle, "SELECT `vend_loot`,`vend_lvl` FROM `evs_info` WHERE `char_id` = '%d'", (*sd)->status.char_id))
		Sql_ShowDebug(map->mysql_handle);

	while( SQL_SUCCESS == SQL->NextRow(map->mysql_handle) ) {
		SQL->GetData(map->mysql_handle, 0, &mdata, NULL); ssd->vend_loot = atoi(mdata);
		SQL->GetData(map->mysql_handle, 1, &mdata, NULL); ssd->vend_lvl = atoi(mdata);
		break;
	}
	return;
}

HPExport void plugin_init (void){
	
	addHookPre(clif, pSelectArrow, clif_parse_SelectArrow_pre);
	addHookPre(clif, pOpenVending, clif_parse_OpenVending_pre);
	addHookPre(skill, castend_nodamage_id, skill_castend_nodamage_id_pre);
	addHookPre(vending, list, vending_list_pre);
	addHookPre(vending, purchase, vending_purchasereq_mod);
	addHookPre(pc, autotrade_prepare, pc_autotrade_prepare_pre);
	addHookPre(pc, autotrade_populate, pc_autotrade_populate_pre);
	addHookPost(itemdb, read, itemdb_read_post);
	if (SQL_ERROR == SQL->Query(map->mysql_handle, "CREATE TABLE IF NOT EXISTS `evs_info` (`char_id` int(10) NOT NULL,`vend_loot` int(6) NOT NULL,`vend_lvl` int(5) NOT NULL, PRIMARY KEY(`char_id`))")){
		Sql_ShowDebug(map->mysql_handle);
	}
}

HPExport void server_preinit (void) {
	addBattleConf("battle_configuration/extended_vending",ev_bc, ev_return_bc, false);
	addBattleConf("battle_configuration/show_item_vending",ev_bc, ev_return_bc, false);
	addBattleConf("battle_configuration/ex_vending_info",ev_bc, ev_return_bc, false);
	addBattleConf("battle_configuration/item_zeny",ev_bc, ev_return_bc, false);
	addBattleConf("battle_configuration/item_cash",ev_bc, ev_return_bc, false);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}
