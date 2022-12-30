//===== Hercules Plugin ======================================
//= Security
//===== By: ==================================================
//= by Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Prohibits actions depending upon security level.
//===== Changelog: ===========================================
//= v1.0 - Initial Conversion
//===== Additional Comments: =================================
//= Use NPC too along with this plugin
//===== Repo Link: ===========================================
//= https://github.com/dastgir/HPM-Plugins
//============================================================

#include "common/hercules.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "common/HPMi.h"
#include "common/nullpo.h"
#include "common/memmgr.h"
#include "common/mmo.h"

#include "map/atcommand.h"
#include "map/battle.h"
#include "map/clif.h"
#include "map/guild.h"
#include "map/map.h"
#include "map/mob.h"
#include "map/npc.h"
#include "map/pc.h"
#include "map/rodex.h"
#include "map/storage.h"
#include "map/trade.h"
#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"


HPExport struct hplugin_info pinfo = {
	"Security",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};
const char* secure = "#security";
const char* secure_opt = "#secure_opt";

#define is_secure(x) pc_readaccountreg(x, script->add_str(secure))
#define security_opt(x) pc_readaccountreg(x, script->add_str(secure_opt))

enum S_Options {	//Security Options
	S_CANT_DROP = 0x0001,				// Cannot Drop Items
	S_CANT_TRADE_R = 0x0002,			// Cannot Receive Trade Request
	S_CANT_TRADE_S = 0x0004,			// Cannot Send Trade Request
	S_CANT_OPEN_GS = 0x0008,			// Cannot Open gStorage
	S_CANT_TAKE_GS = 0x0010,			// Cannot take Item from gStorage
	S_CANT_ADD_GS = 0x0020,				// Cannot add Item to gStorage
	S_CANT_SELL = 0x0040,				// Cannot Sell item
	S_CANT_VEND = 0x0080,				// Cannot Vend
	S_CANT_DELETE = 0x0100,				// Cannot Delete Items(By Drop/delitem function)
	S_CANT_BUY = 0x0200,				// Cannot Buy Items
	S_CANT_SEND_GINVITE = 0x0400,		// Cannot Send Guild Invite
	S_CANT_RECEIVE_GINVITE = 0x0800,	// Cannot Receive Guild Invite
	S_CANT_LEAVE_GUILD = 0x1000,		// Cannot Leave Guild
	S_CANT_RODEX = 0x2000,              // Cannot Send RoDEX Mail
	S_CANT_RODEX_ZENY = 0x4000,         // Cannot Send Zeny via RoDEX
	S_CANT_RODEX_ITEM = 0x8000,         // Cannot Send Item via RoDEX
};

int pc_cant_drop(struct map_session_data **sd, int *n, int *amount) {	// Can't Drop Items
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd)&S_CANT_DROP || security_opt(*sd)&S_CANT_DELETE){
			clif->message((*sd)->fd, "Security is on. You cannot drop item.");
			hookStop();
			return 0;
		}
	}
	return 1;	
}

void cant_trade(struct map_session_data **sd, struct map_session_data **target_sd){	// Can't receive/send Trade Requests
	if (*sd == NULL || *target_sd == NULL)
		return;
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd) & S_CANT_TRADE_S) {	//Cannot Send
			clif->message((*sd)->fd, "Security is on. You cannot initiate Trade.");
			hookStop();
			return;
		}
	}
	if (is_secure(*target_sd) > 0) {	// Cannot Receive
		if (security_opt(*target_sd) & S_CANT_TRADE_R) {
			clif->message((*sd)->fd, "Target Player Security is on, Player cannot receive Trade Requests.");
			hookStop();
			return;
		}
	}
	return;
}

int gstorage_cant_open(struct map_session_data **sd){
	if (sd==NULL)
		return 2;
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd) & S_CANT_OPEN_GS) {
			clif->message((*sd)->fd, "Security is on. You cannot open gStorage.");
			hookStop();
			return 1;
		}
	}
	return 0;
}

int gstorage_cant_add(struct map_session_data **sd, struct guild_storage **stor, struct item **item_data, int *amount) {
	if (*sd == NULL)
		return 1;
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd) & S_CANT_ADD_GS) {
			clif->message((*sd)->fd, "Security is on. You cannot add item to gStorage.");
			hookStop();
			return 1;
		}
	}
	return 0;
}

int gstorage_cant_take(struct map_session_data **sd, int *index, int *amount) {
	if (*sd == NULL)
		return 0;
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd) & S_CANT_TAKE_GS) {
			clif->message((*sd)->fd, "Security is on. You cannot take item from gStorage.");
			hookStop();
			return 0;
		}
	}
	return 1;
}

int pc_restrict_items(struct map_session_data **sd, int *n, int *amount, int *type, enum delitem_reason *reason, e_log_pick_type *log_type){
	if (*sd == NULL)
		return 1;
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd) & S_CANT_DELETE) {
			clif->message((*sd)->fd, "Security is on. You cannot delete item.");
			hookStop();
			return 1;
		}
	}
	return 0;
}

int npc_cant_sell(struct map_session_data **sd, struct itemlist **item_list)
{
	if (*sd == NULL)
		return 1;
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd) & S_CANT_SELL) {
			clif->message((*sd)->fd, "Security is on. You cannot sell item.");
			hookStop();
			return 1;
		}
	}
	return 0;
}

int npc_cant_buy(struct map_session_data **sd, struct itemlist **item_list) {
	if (*sd == NULL)
		return 1;
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd) & S_CANT_BUY) {
			clif->message((*sd)->fd, "Security is on. You cannot buy item.");
			hookStop();
			return 1;
		}
	}
	return 0;
}

void open_vending(struct map_session_data **sd, int *num){
	if (*sd == NULL)
		return;
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd) & S_CANT_VEND) {
			clif->message((*sd)->fd, "Security is on. You cannot vend.");
			(*sd)->state.prevend = (*sd)->state.workinprogress = 0;
			hookStop();
			return;
		}
	}
	return;
}

int guild_invite_permission(struct map_session_data **sd, struct map_session_data **tsd) {
	if (*sd == NULL || *tsd == NULL)
		return 0;
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd) & S_CANT_SEND_GINVITE) {
			clif->message((*sd)->fd, "Security is on. You cannot send Guild Invite.");
			hookStop();
			return 0;
		}
	}
	
	if (is_secure(*tsd) > 0) {
		if (security_opt(*tsd) & S_CANT_RECEIVE_GINVITE) {
			clif->message((*sd)->fd, "Target Player Security is on, Player cannot receive Guild Invites.");
			hookStop();
			return 0;
		}
	}
	return 0;
}

int guild_leave_permission(struct map_session_data **sd, int *guild_id, int *account_id, int *char_id, const char **mes) {
	if (*sd == NULL)
		return 0;
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd) & S_CANT_LEAVE_GUILD) {
			clif->message((*sd)->fd, "Security is on. You cannot leave Guild.");
			hookStop();
			return 0;
		}
	}
	return 0;
}

int rodex_send_mail_pre(struct map_session_data **sd, const char **receiver_name, const char **body, const char **title, int64 *zeny) {
	nullpo_retr(RODEX_SEND_MAIL_FATAL_ERROR, *sd);
	nullpo_retr(RODEX_SEND_MAIL_FATAL_ERROR, *receiver_name);
	nullpo_retr(RODEX_SEND_MAIL_FATAL_ERROR, *body);
	nullpo_retr(RODEX_SEND_MAIL_FATAL_ERROR, *title);
	if (!rodex->isenabled() || (*sd)->npc_id > 0) {
		rodex->clean(*sd, 1);
		return RODEX_SEND_MAIL_FATAL_ERROR;
	}
	if (is_secure(*sd) > 0) {
		if (security_opt(*sd) & S_CANT_RODEX) {
			clif->message((*sd)->fd, "Security is on. You cannot send RoDEX Mail.");
			hookStop();
			return RODEX_SEND_MAIL_FATAL_ERROR;
		}
		if (*zeny > 0 && security_opt(*sd) & S_CANT_RODEX_ZENY) {
			clif->message((*sd)->fd, "Security is on. You cannot send Zeny via RoDEX.");
			hookStop();
			return RODEX_SEND_MAIL_FATAL_ERROR;
		}
		int i = 0;
		bool rodex_item_found = false;
		for (i = 0; i < RODEX_MAX_ITEM; i++) {
			struct item *tmpItem = &(*sd)->rodex.tmp.items[i].item;

			if (tmpItem->nameid == 0)
				continue;
			
			rodex_item_found = true;
			break;
		}
		if (rodex_item_found && security_opt(*sd) & S_CANT_RODEX_ITEM) {
			clif->message((*sd)->fd, "Security is on. You cannot send items via RoDEX.");
			hookStop();
			return RODEX_SEND_MAIL_FATAL_ERROR;
		}
	}
	
	return RODEX_SEND_MAIL_SUCCESS;

}

/* Server Startup */
HPExport void plugin_init(void)
{
	addHookPre(pc, dropitem, pc_cant_drop);
	addHookPre(trade, request, cant_trade);
	addHookPre(gstorage, open, gstorage_cant_open);
	addHookPre(gstorage, additem, gstorage_cant_add);
	addHookPre(gstorage, get, gstorage_cant_take);
	addHookPre(gstorage, gettocart, gstorage_cant_take);
	addHookPre(pc, delitem, pc_restrict_items);
	addHookPre(npc, selllist, npc_cant_sell);
	addHookPre(npc, buylist, npc_cant_buy);
	addHookPre(clif, openvendingreq, open_vending);
	addHookPre(guild, invite, guild_invite_permission);
	addHookPre(guild, leave, guild_leave_permission);
	addHookPre(rodex, send_mail, rodex_send_mail_pre);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
