//===== Hercules Plugin ======================================
//= sellitem2
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Sell items with Refines/Cards into shop
//===== Changelog: ===========================================
//= v1.0 - Initial Release
//===== Additional Info: =====================================
// *sellitem2 <Item_ID>,identify,refine,attribute,card1,card2,card3,card4{,price};
// 
// adds (or modifies) <Item_ID> data to the shop,
// when <price> is not provided (or when it is -1) itemdb default is used.
// qty is only necessary for NST_MARKET trader types.
// 
// when <Item_ID> is already in the shop,
// the previous data (price/qty), is overwritten with the new.
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
#include "common/socket.h"
#include "common/strlib.h"
#include "common/utils.h"

#include "map/clif.h"
#include "map/itemdb.h"
#include "map/map.h"
#include "map/npc.h"
#include "map/pc.h"
#include "map/pet.h"
#include "map/script.h"
#include "map/skill.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"sellitem2",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

struct shop_litem_entry {
	int id;       ///< Item ID
	int16 amount; ///< Amount
	int identify; ///< Identified?
	int refine;   ///< Refine Count
	int attribute;///< Damaged?
	int card1;    ///< Crad 1
	int card2;    ///< Card 2
	int card3;    ///< Card 3
	int card4;    ///< Card 4
};
VECTOR_STRUCT_DECL(shop_litem, struct shop_litem_entry);

struct npc_shop_litem {	// NPC Shop ItemList
	unsigned short nameid;
	unsigned int identify;
	unsigned int refine;
	unsigned int attribute;
	unsigned int card1;
	unsigned int card2;
	unsigned int card3;
	unsigned int card4;
	unsigned int value;
};

struct npc_extra_data {
	struct npc_shop_litem *item;	// List
	unsigned short items;	// Total Items
};

//Converts item type in case of pet eggs.
static inline int itemtype(int type) {
	switch( type ) {
#if PACKETVER >= 20080827
		case IT_WEAPON:
			return IT_ARMOR;
		case IT_ARMOR:
		case IT_PETARMOR:
#endif
		case IT_PETEGG:
			return IT_WEAPON;
		default:
			return type;
	}
}

struct npc_extra_data *nsd_search(struct npc_data *nd, bool create) {
	struct npc_extra_data *nsd = NULL;
	if (nd == NULL)
		return NULL;
	nsd = getFromNPCD(nd, 0);
	if (create == true && nsd == NULL) {
		CREATE(nsd, struct npc_extra_data, 1);
		nsd->items = 0;
		addToNPCD(nd, nsd, 0, true);
	}
	return nsd;
}

/**
 * Creates (npc_data)->u.scr.shop and updates all duplicates across the server to match the created pointer
 *
 * @param master id of the original npc
 **/
void npc_trader_update2(int master) {
	struct DBIterator *iter;
	struct block_list* bl;
	struct npc_data *master_nd = map->id2nd(master);
	struct npc_extra_data *master_nsd = nsd_search(master_nd, true);

	CREATE(master_nd->u.scr.shop, struct npc_shop_data, 1);

	iter = db_iterator(map->id_db);
	for (bl = dbi_first(iter); dbi_exists(iter); bl = dbi_next(iter)) {
		if (bl->type == BL_NPC) {
			struct npc_data *nd = BL_UCAST(BL_NPC, bl);
			struct npc_extra_data *nsd = nsd_search(nd, true);
			if (nd->src_id == master) {
				nd->u.scr.shop = master_nd->u.scr.shop;
				nsd->item = master_nsd->item;
				nsd->items = master_nsd->items;
			}
		}
	}
	dbi_destroy(iter);
}

/**
 * @call sellitem2 <Item_ID>,identify,refine,attribute,card1,card2,card3,card4{,price};
 *
 * adds <Item_ID> (or modifies if present) to shop
 * if price not provided (or -1) uses the item's value_sell
 **/
BUILDIN(sellitem2) {
	struct npc_data *nd;
	struct item_data *it;
	int i = 0, id = script_getnum(st,2);
	int value = 0;
	int identify = script_getnum(st, 3);
	int refine = script_getnum(st, 4);
	int attribute = script_getnum(st, 5);
	int card1 = script_getnum(st, 6);
	int card2 = script_getnum(st, 7);
	int card3 = script_getnum(st, 8);
	int card4 = script_getnum(st, 9);
	struct npc_extra_data *nsd;

	if (!(nd = map->id2nd(st->oid))) {
		ShowWarning("buildin_sellitem2: trying to run without a proper NPC!\n");
		return false;
	} else if (!(it = itemdb->exists(id))) {
		ShowWarning("buildin_sellitem2: unknown item id '%d'!\n",id);
		return false;
	}
	
	nsd = nsd_search(nd, true);

	value = script_hasdata(st, 10) ? script_getnum(st,  10) : it->value_buy;
	if (value == -1)
		value = it->value_buy;

	if (!nd->u.scr.shop)
		npc_trader_update2(nd->src_id?nd->src_id:nd->bl.id);
	else {
		for (i = 0; i < nsd->items; i++) {
			if (nsd->item[i].nameid == id &&
				nsd->item[i].identify == identify &&
				nsd->item[i].refine == refine &&
				nsd->item[i].attribute == attribute &&
				nsd->item[i].card1 == card1 &&
				nsd->item[i].card2 == card2 &&
				nsd->item[i].card3 == card3 &&
				nsd->item[i].card4 == card4)	// Found Same Item
				break;
		}
	}

	if (nd->u.scr.shop->type == NST_ZENY && value*0.75 < it->value_sell*1.24) {
		ShowWarning("buildin_sellitem2: Item %s [%d] discounted buying price (%d->%d) is less than overcharged selling price (%d->%d) in NPC %s (%s)\n",
					it->name, id, value, (int)(value*0.75), it->value_sell, (int)(it->value_sell*1.24), nd->exname, nd->path);
	}

	if (i != nsd->items) {
		nsd->item[i].value = value;
	} else {
		for (i = 0; i < nsd->items; i++) {
			if (nsd->item[i].nameid == 0)
				break;
		}

		if (i == nsd->items) {
			if(nsd->items == USHRT_MAX ) {
				ShowWarning("buildin_sellitem2: Can't add %s (%s/%s), shop list is full!\n", it->name, nd->exname, nd->path);
				return false;
			}
			i = nsd->items;
			RECREATE(nsd->item, struct npc_shop_litem, ++nsd->items);
		}

		
		nsd->item[i].nameid = it->nameid;
		nsd->item[i].identify = identify;
		nsd->item[i].refine = refine;
		nsd->item[i].attribute = attribute;
		nsd->item[i].card1 = card1;
		nsd->item[i].card2 = card2;
		nsd->item[i].card3 = card3;
		nsd->item[i].card4 = card4;
		nsd->item[i].value = value;
	}
	return true;
}

int npc_unload_pre(struct npc_data **nd, bool *single)
{
	struct npc_extra_data *nsd;
	nullpo_ret(*nd);
	nsd = nsd_search(*nd, false);
	if (nsd)
		aFree(nsd->item);	// Free the item struct.

	return 0;
}

/// Sends a list of items in a shop.
/// R 0133 <packet len>.W <owner id>.L { <price>.L <amount>.W <index>.W <type>.B <name id>.W <identified>.B <damaged>.B <refine>.B <card1>.W <card2>.W <card3>.W <card4>.W }* (ZC_PC_PURCHASE_ITEMLIST_FROMMC)
/// R 0800 <packet len>.W <owner id>.L <unique id>.L { <price>.L <amount>.W <index>.W <type>.B <name id>.W <identified>.B <damaged>.B <refine>.B <card1>.W <card2>.W <card3>.W <card4>.W }* (ZC_PC_PURCHASE_ITEMLIST_FROMMC2)
void clif_buylist_pre(struct map_session_data **sd_, struct npc_data **nd_) {
	struct npc_extra_data *nsd;
	struct npc_shop_litem *shop;
	struct map_session_data *sd = *sd_;
	struct npc_data *nd = *nd_;
	unsigned int shop_size = 0;
	int fd, i;
#if PACKETVER < 20100105
	const int cmd = 0x133;
	const int offset = 8;
#else
	const int cmd = 0x800;
	const int offset = 12;
#endif

#if PACKETVER >= 20150226
	const int item_length = 47;
#else
	const int item_length = 22;
#endif

	nullpo_retv(sd);
	nullpo_retv(nd);
	if ((nsd = nsd_search(nd, false)) == NULL)
		return;

	shop = nsd->item;
	shop_size = nsd->items;

	fd = sd->fd;
	sd->vended_id = nd->bl.id;

	WFIFOHEAD(fd, offset+shop_size*item_length);
	WFIFOW(fd, 0) = cmd;
	WFIFOW(fd, 2) = offset+shop_size*item_length;
	WFIFOL(fd, 4) = -(nd->bl.id);	// -ve indicates buy from shop
#if PACKETVER >= 20100105
	WFIFOL(fd, 8) = -1;
#endif
	
	for (i = 0; i < shop_size; i++) {
#if PACKETVER >= 20150226
		int temp = 0;
#endif
		struct item_data* data = itemdb->search(shop[i].nameid);
		WFIFOL(fd, offset+ 0+i*item_length) = shop[i].value;
		WFIFOW(fd, offset+ 4+i*item_length) = 1;	// Amount(ToDo)
		WFIFOW(fd, offset+ 6+i*item_length) = i+2;	// Idx, send as i
		WFIFOB(fd, offset+ 8+i*item_length) = itemtype(data->type);
		WFIFOW(fd, offset+ 9+i*item_length) = (data->view_id > 0) ? data->view_id : shop[i].nameid;
		WFIFOB(fd, offset+11+i*item_length) = shop[i].identify;
		WFIFOB(fd, offset+12+i*item_length) = shop[i].attribute;
		WFIFOB(fd, offset+13+i*item_length) = shop[i].refine;
		WFIFOW(fd, offset+14+i*item_length) = shop[i].card1;
		WFIFOW(fd, offset+16+i*item_length) = shop[i].card2;
		WFIFOW(fd, offset+18+i*item_length) = shop[i].card3;
		WFIFOW(fd, offset+20+i*item_length) = shop[i].card4;
#if PACKETVER >= 20150226
		for (temp = 0; temp < 5; temp++){
			WFIFOW(fd, offset+22+i*item_length+temp*5+0) = 0;
			WFIFOW(fd, offset+22+i*item_length+temp*5+2) = 0;
			WFIFOB(fd, offset+22+i*item_length+temp*5+4) = 0;
		}
#endif
	}
	WFIFOSET(fd,WFIFOW(fd,2));
	hookStop();
}

/**
 * Processes a player item purchase from npc shop.
 *
 * @param nd		NPC Data
 * @param sd        Buyer character.
 * @param item_list List of items.
 * @return result code for clif->parse_NpcBuyListSend.
 */
int shop_buylist(struct npc_data *nd, struct npc_extra_data *nsd, struct map_session_data *sd, struct shop_litem *item_list)
{
	struct npc_shop_litem *shop = NULL;
	int64 z;
	int i,j,w,new_;
	unsigned short shop_size = 0;

	nullpo_retr(3, sd);
	nullpo_retr(3, item_list);
	nullpo_retr(3, nd);
	nullpo_retr(3, nsd);
	
	if (nd->subtype != SHOP) {
		if (nd->subtype == SCRIPT && nd->u.scr.shop && nsd->item && nd->u.scr.shop->type == NST_ZENY) {
			shop = nsd->item;
			shop_size = nsd->items;
		} else
			return 3;
	} else
		return 3;

	z = 0;
	w = 0;
	new_ = 0;
	// process entries in buy list, one by one
	for (i = 0; i < VECTOR_LENGTH(*item_list); ++i) {
		int value;
		struct shop_litem_entry *entry = &VECTOR_INDEX(*item_list, i);

		// find this entry in the shop's sell list
		ARR_FIND( 0, shop_size, j,
				 entry->id == shop[j].nameid && //Normal items
				 entry->identify == shop[j].identify &&
				 entry->refine == shop[j].refine &&
				 entry->attribute == shop[j].attribute &&
				 entry->card1 == shop[j].card1 &&
				 entry->card2 == shop[j].card2 &&
				 entry->card3 == shop[j].card3 &&
				 entry->card4 == shop[j].card4
				 );
		if (j == shop_size)
			return 3; // no such item in shop

		value = shop[j].value;

		if (!itemdb->exists(entry->id))
			return 3; // item no longer in itemdb

		if (!itemdb->isstackable(entry->id) && entry->amount > 1) {
			//Exploit? You can't buy more than 1 of equipment types o.O
			ShowWarning("Player %s (%d:%d) sent a hexed packet trying to buy %d of non-stackable item %d!\n",
						sd->status.name, sd->status.account_id, sd->status.char_id, entry->amount, entry->id);
			entry->amount = 1;
		}

		if (nd->master_nd) {
			// Script-controlled shops decide by themselves, what can be bought and for what price.
			continue;
		}

		switch (pc->checkadditem(sd, entry->id, entry->amount)) {
			case ADDITEM_EXIST:
				break;

			case ADDITEM_NEW:
				new_++;
				break;

			case ADDITEM_OVERAMOUNT:
				return 2;
		}

		//value = pc->modifybuyvalue(sd, value);	// Should we give discount?

		z += (int64)value * entry->amount;
		w += itemdb_weight(entry->id) * entry->amount;
	}

	/*
	No support for Script Based Shop for now [Dastgir]
	if (nd->master_nd != NULL) //Script-based shops.
		return npc->buylist_sub(sd, item_list, nd->master_nd);
	*/
	if (z > sd->status.zeny)
		return 1; // Not enough Zeny
	if( w + sd->weight > sd->max_weight )
		return 2; // Too heavy
	if( pc->inventoryblank(sd) < new_ )
		return 3; // Not enough space to store items

	pc->payzeny(sd, (int)z, LOG_TYPE_NPC, NULL);

	for (i = 0; i < VECTOR_LENGTH(*item_list); ++i) {
		struct shop_litem_entry *entry = &VECTOR_INDEX(*item_list, i);
		int refine = entry->refine;
		struct item item_tmp;
		switch(itemdb_type(entry->id)) {
			case IT_WEAPON:
			case IT_ARMOR:
				refine = cap_value(refine, 0, MAX_REFINE);
				break;
			case IT_PETEGG:
				pet->create_egg(sd, entry->id);			
				continue;
			default:
				break;
		}
		memset(&item_tmp, 0, sizeof(item_tmp));
		item_tmp.nameid = entry->id;
		item_tmp.identify = entry->identify;
		item_tmp.refine = refine;
		item_tmp.attribute = entry->attribute;
		item_tmp.card[0] = (short)entry->card1;
		item_tmp.card[1] = (short)entry->card2;
		item_tmp.card[2] = (short)entry->card3;
		item_tmp.card[3] = (short)entry->card4;

		pc->additem(sd, &item_tmp, entry->amount, LOG_TYPE_NPC);
		
	}
	return 0;
}

void clif_parse_purchase(struct map_session_data* sd, int bl_id, const uint8* data, int n)
{
	int result;
	struct npc_data *nd = map->id2nd(bl_id);
	struct npc_extra_data *nsd;

	if (nd == NULL || ((nsd = nsd_search(nd, false)) == NULL) || sd->state.trading || !sd->vended_id || pc_has_permission(sd,PC_PERM_DISABLE_STORE) ) {
		result = 1;
	} else {
		struct shop_litem item_list = { 0 };
		int i;

		VECTOR_INIT(item_list);
		VECTOR_ENSURE(item_list, n, 1);
		for (i = 0; i < n; i++) {
			struct shop_litem_entry entry = { 0 };
			int idx, amount;
			amount = *(const uint16*)(data + 4*i + 0);
			idx     = (*(const uint16*)(data + 4*i + 2));
			idx -= 2;
			if (amount <= 0 || idx < 0 || idx > nsd->items)
				break;
			entry.amount = amount;
			entry.id = nsd->item[idx].nameid;
			entry.identify = nsd->item[idx].identify;
			entry.refine = nsd->item[idx].refine;
			entry.attribute = nsd->item[idx].attribute;
			entry.card1 = nsd->item[idx].card1;
			entry.card2 = nsd->item[idx].card2;
			entry.card3 = nsd->item[idx].card3;
			entry.card4 = nsd->item[idx].card4;

			VECTOR_PUSH(item_list, entry);
		}
		if (i == n) {
			result = shop_buylist(nd, nsd, sd, &item_list);
		}
		VECTOR_CLEAR(item_list);
	}
	clif->npc_buy_result(sd, result);
	//Clear shop data.
	sd->vended_id = 0;
}

/// Shop item(s) purchase request (CZ_PC_PURCHASE_ITEMLIST_FROMMC).
/// 0134 <packet len>.W <account id>.L { <amount>.W <index>.W }*
void clif_parse_PurchaseReq_pre(int *fd, struct map_session_data **sd)
{
	int len = (int)RFIFOW(*fd,2) - 8;
	int id = RFIFOL(*fd, 4);
	const uint8 *data;
	if (id > 0)
		return;
	data = RFIFOP(*fd, 8);
	clif_parse_purchase(*sd, -id, data, len/4);
	hookStop();
}

/// Shop item(s) purchase request (CZ_PC_PURCHASE_ITEMLIST_FROMMC2).
/// 0801 <packet len>.W <account id>.L <unique id>.L { <amount>.W <index>.W }*
void clif_parse_PurchaseReq2_pre(int *fd, struct map_session_data **sd)
{
	int len = (int)RFIFOW(*fd,2) - 12;
	int aid = RFIFOL(*fd,4);
	int uid = RFIFOL(*fd,8);
	const uint8 *data;

	if (aid > 0 && uid != -1)
		return;
	
	data = RFIFOP(*fd, 12);
	
	clif_parse_purchase(*sd, -aid, data, len/4);
	hookStop();
}


/*==========================================
 * Chk if valid call then open buy or selling list
 *------------------------------------------*/
int npc_buysellsel_pre(struct map_session_data **sd_, int *id_, int *type_)
{
	struct map_session_data *sd = *sd_;
	int id = *id_, type = *type_;
	struct npc_data *nd;
	struct npc_extra_data *nsd;

	nullpo_retr(1, sd);

	if ((nd = npc->checknear(sd,map->id2bl(id))) == NULL)
		return 1;
	
	if ((nsd = nsd_search(nd, false)) == NULL)
		return 1;

	if (nd->subtype != SHOP && !(nd->subtype == SCRIPT && nd->u.scr.shop && nsd->items)) {
		if (nd->subtype == SCRIPT )
			ShowError("npc_buysellsel_pre: trader '%s'(%d:%d) has no shop list!\n", nd->exname, nsd->items, nd->subtype);
		else
			ShowError("npc_buysellsel_pre: no such shop npc %d (%s)\n", id, nd->exname);

		if (sd->npc_id == id)
			sd->npc_id = 0;
		return 1;
	}

	if (nd->option & OPTION_INVISIBLE) { // can't buy if npc is not visible (hack?)
		return 1;
	}

	if (nd->class_ < 0 && !sd->state.callshop) {// not called through a script and is not a visible NPC so an invalid call
		return 1;
	}

	// reset the callshop state for future calls
	sd->state.callshop = 0;
	sd->npc_shopid = id;

	if (type == 0) {
		clif->buylist(sd, nd);
	} else {
		clif->selllist(sd);
	}
	hookStop();
	return 0;
}

bool npc_trader_open_pre(struct map_session_data **sd_, struct npc_data **nd_) {
	struct map_session_data *sd = *sd_;
	struct npc_data *nd = *nd_;
	struct npc_extra_data *nsd;
	
	nullpo_retr(false, sd);
	nullpo_retr(false, nd);
	
	nsd = nsd_search(nd, false);
	
	if (nsd == NULL)
		return true;
	hookStop();
	if (!nd->u.scr.shop || !nsd->items)
		return false;

	switch(nd->u.scr.shop->type) {
		case NST_ZENY:
			sd->state.callshop = 1;
			clif->npcbuysell(sd, nd->bl.id);
			return true;/* we skip sd->npc_shopid, npc->buysell will set it then when the player selects */
		case NST_MARKET:
			return false;
		default:
			clif->cashshop_show(sd,nd);
			break;
	}
	sd->npc_shopid = nd->bl.id;
	return true;
}

int npc_click_pre(struct map_session_data **sd_, struct npc_data **nd_)
{
	struct map_session_data *sd = *sd_;
	struct npc_data *nd = *nd_;
	struct npc_extra_data *nsd;
	
	nullpo_retr(1, sd);
	
	if (nd) {
		nsd = nsd_search(nd, false);
		if (nsd && nd->subtype == SCRIPT) {
			hookStop();
		} else {
			return 0;
		}
	} else {
		return 1;
	}
	
	if (sd->npc_id != 0) {
		// The player clicked a npc after entering an OnTouch area
		if( sd->areanpc_id != sd->npc_id )
			ShowError("npc_click: npc_id != 0\n");
		return 1;
	}

	if ((nd = npc->checknear(sd,&nd->bl)) == NULL)
		return 1;

	//Hidden/Disabled npc.
	if (nd->class_ < 0 || nd->option&(OPTION_INVISIBLE|OPTION_HIDE))
		return 1;
	
	if (nd->u.scr.shop && nsd->items && nd->u.scr.trader) {
		if (!npc->trader_open(sd, nd)) {
			return 1;
		}
	}
	return 0;
}

HPExport void plugin_init(void) 
{
	addHookPre(clif, buylist, clif_buylist_pre);
	addHookPre(clif, pPurchaseReq, clif_parse_PurchaseReq_pre);
	addHookPre(clif, pPurchaseReq2, clif_parse_PurchaseReq2_pre);
	addHookPre(npc, unload, npc_unload_pre);
	addHookPre(npc, buysellsel, npc_buysellsel_pre);
	addHookPre(npc, trader_open, npc_trader_open_pre);
	addHookPre(npc, click, npc_click_pre);
	addScriptCommand("sellitem2", "iiiiiiii?", sellitem2);
}

HPExport void server_online (void)
{
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}
