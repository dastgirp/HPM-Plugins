/**
 * @noview Plugin
 * By Dastgir
 */


#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/HPMi.h"
#include "map/script.h"
#include "map/chat.h"
#include "map/party.h"
#include "map/guild.h"
#include "map/pc.h"
#include "map/npc.h"
#include "map/map.h"
#include "map/itemdb.h"
#include "map/vending.h"
#include "map/packets.h"
#include "common/memmgr.h"
#include "common/socket.h"
#include "common/nullpo.h"


#include "plugins/HPMHooking.h"
#define sd_chat(sd) (sd->chat_id)
#define CONST_PARAMETER

#include "common/HPMDataCheck.h"


HPExport struct hplugin_info pinfo = {
	"noview",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

const char* updated_at = "9th May 2020";

struct player_data {
	int headgearFilter;
};

enum viewhg_e {
	VHG_NONE = 0,
	VHG_UPPER = 1,
	VHG_MID = 2,
	VHG_LOW = 4
} viewhg_e;

struct player_data *search_pdb(struct map_session_data *sd, bool create) {
	struct player_data *pdb = NULL;
	if (sd != NULL && (pdb = getFromMSD(sd, 0)) == NULL && create) {
		CREATE(pdb, struct player_data, 1);
		pdb->headgearFilter = VHG_NONE;
		addToMSD(sd, pdb, 0, true);
	}
	return pdb;
}

ACMD(noview) {
	struct player_data *pdb;
    if (!*message) {
		clif->message(fd, "View Headgear Usage:");
		clif->message(fd, "@noview <options>");
		clif->message(fd, "- on : To enable ALL Filters.");
		clif->message(fd, "- off : To disable ALL Filters.");
		clif->message(fd, "- U : To filter Upper Headgear.");
		clif->message(fd, "- M : To filter Middle Headgear.");
		clif->message(fd, "- L : To filter Lower Headgear.");
		clif->message(fd, "Samples:");
		clif->message(fd, "  @noview UML : To filter 3 chosen options.");
		return true;
	}
	pdb = search_pdb(sd, true);
	if (!strcmpi(message, "on")) {
		pdb->headgearFilter = VHG_UPPER | VHG_MID | VHG_LOW;
		clif->message(fd, "Headgear Filtering ON");
	} else if (!strcmpi(message, "off")) {
		pdb->headgearFilter = 0;
		clif->message(fd, "Headgear Filtering OFF");
	} else {
		char atcmd_output[100];
		if (strstr(message, "U"))
			pdb->headgearFilter |= VHG_UPPER;
		if (strstr(message, "M"))
			pdb->headgearFilter |= VHG_MID;
		if (strstr(message, "L"))
			pdb->headgearFilter |= VHG_LOW;

		clif->message(fd, "Headgear Filtering:");
		sprintf(atcmd_output, "Upper: %s | Middle: %s | Lower: %s",
			(pdb->headgearFilter & VHG_UPPER) ? "ON" : "OFF",
			(pdb->headgearFilter & VHG_MID) ? "ON" : "OFF",
			(pdb->headgearFilter & VHG_LOW) ? "ON" : "OFF");
			clif->message(fd, atcmd_output);
	}
	clif->refresh(sd);
	return true;
}

void clif_viewhg_change(uint16 ou, uint16 om, uint16 ol, uint16* upper, uint16* mid, uint16* low, enum viewhg_e ve, int tid, int sid);
void clif_viewhg_changei(uint16 ou, uint16 om, uint16 ol, int16* upper, int16* mid, int16* low, enum viewhg_e ve, const char *tname, const char* sname) {
	if (strcmp(tname, sname) == 0)
		clif_viewhg_change(ou, om, ol, (uint16*)upper, (uint16*)mid, (uint16*)low, ve, 0, 1);
}

void clif_viewhg_change(uint16 ou, uint16 om, uint16 ol, uint16 *upper, uint16 *mid, uint16 *low, enum viewhg_e ve, int tid, int sid) {
	if (sid == tid)
		return;
	// Change Upper Value
	if (ve & VHG_UPPER) {
		*upper = 0;
	} else {
		*upper = ou;
	}

	// Change Mid Value
	if (ve & VHG_MID) {
		*mid = 0;
	} else {
		*mid = om;
	}

	// Change Lower Value
	if (ve & VHG_LOW) {
		*low = 0;
	} else {
		*low = ol;
	}
	return;
}

void clif_send_helper(struct map_session_data *sd, struct map_session_data *tsd, const void *buf, void *buf_new, int len) {
	struct player_data *pdb = search_pdb(tsd, false);

	if (pdb == NULL || pdb->headgearFilter == 0) {
		WFIFOHEAD(tsd->fd, len);
		memcpy(WFIFOP(tsd->fd, 0), buf, len);
		WFIFOSET(tsd->fd,len);
		return;
	}
	
	WFIFOHEAD(tsd->fd, len);
	unsigned short cmd = RBUFW(buf, 0);
	switch(cmd) {
#if PACKETVER < 20091103
		case idle_unit2Type: {   // packet_idle_unit2
			CONST_PARAMETER struct packet_idle_unit2 *p = (CONST_PARAMETER struct packet_idle_unit2 *)RBUFP(buf, 0);
			CONST_PARAMETER struct packet_idle_unit2 *p2 = (CONST_PARAMETER struct packet_idle_unit2 *)RBUFP(buf_new, 0);
			clif_viewhg_change(p->accessory2, p->accessory3, p->accessory, &(p2->accessory2), &(p2->accessory3), &(p2->accessory), pdb->headgearFilter, tsd->bl.id, p->GID);
			//buf_new = (void*)p2;
			break;
		}
		case spawn_unit2Type: {  // packet_spawn_unit2
			CONST_PARAMETER struct packet_spawn_unit2* p = (CONST_PARAMETER struct packet_spawn_unit2*)RBUFP(buf, 0);
			CONST_PARAMETER struct packet_spawn_unit2* p2 = (CONST_PARAMETER struct packet_spawn_unit2*)RBUFP(buf_new, 0);
			clif_viewhg_change(p->accessory2, p->accessory3, p->accessory, &(p2->accessory2), &(p2->accessory3), &(p2->accessory), pdb->headgearFilter, tsd->bl.id, p->GID);
			//buf_new = (void*)p2;
			break;
		}
#endif
		case idle_unitType: {    // packet_idle_unit
			CONST_PARAMETER struct packet_idle_unit *p = (CONST_PARAMETER struct packet_idle_unit *)RBUFP(buf, 0);
			CONST_PARAMETER struct packet_idle_unit *p2 = (CONST_PARAMETER struct packet_idle_unit *)RBUFP(buf_new, 0);
			clif_viewhg_change(p->accessory2, p->accessory3, p->accessory, &(p2->accessory2), &(p2->accessory3), &(p2->accessory), pdb->headgearFilter, tsd->bl.id, p->GID);
			//buf_new = (void*)p2;
			break;
		}
		case spawn_unitType: {   // packet_spawn_unit
			CONST_PARAMETER struct packet_spawn_unit *p = (CONST_PARAMETER struct packet_spawn_unit *)RBUFP(buf, 0);
			CONST_PARAMETER struct packet_spawn_unit *p2 = (CONST_PARAMETER struct packet_spawn_unit*)RBUFP(buf_new, 0);
			clif_viewhg_change(p->accessory2, p->accessory3, p->accessory, &(p2->accessory2), &(p2->accessory3), &(p2->accessory), pdb->headgearFilter, tsd->bl.id, p->GID);
			//buf_new = (void*)p2;
			break;
		}
		case unit_walkingType: { // packet_unit_walking 
			CONST_PARAMETER struct packet_unit_walking *p = (CONST_PARAMETER struct packet_unit_walking *)RBUFP(buf, 0);
			CONST_PARAMETER struct packet_unit_walking *p2 = (CONST_PARAMETER struct packet_unit_walking *)RBUFP(buf_new, 0);
			clif_viewhg_change(p->accessory2, p->accessory3, p->accessory, &(p2->accessory2), &(p2->accessory3), &(p2->accessory), pdb->headgearFilter, tsd->bl.id, p->GID);
			//buf_new = (void*)p2;
			break;
		}
		case viewequipackType: { // packet_viewequip_ack viewequip_list;
			CONST_PARAMETER struct packet_viewequip_ack *p = (CONST_PARAMETER struct packet_viewequip_ack *)RBUFP(buf, 0);
			CONST_PARAMETER struct packet_viewequip_ack *p2 = (CONST_PARAMETER struct packet_viewequip_ack *)RBUFP(buf_new, 0);
			clif_viewhg_changei(p->accessory2, p->accessory3, p->accessory, &(p2->accessory2), &(p2->accessory3), &(p2->accessory), pdb->headgearFilter, tsd->status.name, p->characterName);
			//buf_new = (void*)p2;
			break;
		}
		case 0xc3: {
			uint8 type = RBUFB(buf, 6);
			int bl_id = RBUFL(buf, 2);
			enum viewhg_e viewhg_eValue = VHG_NONE;
			switch(type) {
				case LOOK_HEAD_BOTTOM:
					viewhg_eValue = VHG_LOW;
					break;
				case LOOK_HEAD_TOP:
					viewhg_eValue = VHG_UPPER;
					break;
				case LOOK_HEAD_MID:
					viewhg_eValue = VHG_MID;
					break;
			}
			if (viewhg_eValue != VHG_NONE && bl_id != tsd->bl.id) {
				if (pdb->headgearFilter & viewhg_eValue) {
					WBUFB(buf_new, 7) = 0;
				} else {
					WBUFB(buf_new, 7) = RBUFB(buf, 7);
				}
			}
			break;
		}
		case 0x1d7: {
			uint8 type = RBUFB(buf, 6);
			int bl_id = RBUFL(buf, 2);
			enum viewhg_e viewhg_eValue = VHG_NONE;
			switch(type) {
				case LOOK_HEAD_BOTTOM:
					viewhg_eValue = VHG_LOW;
					break;
				case LOOK_HEAD_TOP:
					viewhg_eValue = VHG_UPPER;
					break;
				case LOOK_HEAD_MID:
					viewhg_eValue = VHG_MID;
					break;
			}
			if (viewhg_eValue != VHG_NONE && bl_id != tsd->bl.id) {
				if (pdb->headgearFilter & viewhg_eValue) {
					WBUFW(buf_new, 7) = 0;
				} else {
					WBUFW(buf_new, 7) = RBUFW(buf, 7);
				}
			}
			break;
		}
		default:
			ShowError("clif_send_helper: Invalid Packet %d", (int)cmd);
			break;
	}
	memcpy(WFIFOP(tsd->fd, 0), buf_new, len);
	WFIFOSET(tsd->fd, len);
	return;
}

/*==========================================
 * sub process of clif_send
 * Called from a map_foreachinarea (grabs all players in specific area and subjects them to this function)
 * In order to send area-wise packets, such as:
 * - AREA : everyone nearby your area
 * - AREA_WOSC (AREA WITHOUT SAME CHAT) : Not run for people in the same chat as yours
 * - AREA_WOC (AREA WITHOUT CHAT) : Not run for people inside a chat
 * - AREA_WOS (AREA WITHOUT SELF) : Not run for self
 * - AREA_CHAT_WOC : Everyone in the area of your chat without a chat
 *------------------------------------------*/
bool clif_send_pre(const void** buf_, int* len_, struct block_list** bl_, enum send_target* type_);
int clif_send_sub_hook(struct block_list *bl, va_list ap) {
	struct block_list *src_bl;
	struct map_session_data *sd;
	void *buf;
	void *buf_new;
	int len, type, fd;
	int bool_;

	nullpo_ret(bl);
	nullpo_ret(sd = (struct map_session_data *)bl);

	fd = sd->fd;
	if (!fd || sockt->session[fd] == NULL) //Don't send to disconnected clients.
		return 0;

	buf = va_arg(ap,void*);
	len = va_arg(ap,int);
	nullpo_ret(src_bl = va_arg(ap,struct block_list*));
	type = va_arg(ap,int);
	bool_ = va_arg(ap,int);
	buf_new = va_arg(ap,void*);

	switch(type) {
		case AREA_WOS:
			if (bl == src_bl)
				return 0;
		break;
		case AREA_WOC:
			if (sd_chat(sd) || bl == src_bl)
				return 0;
			if (RBUFW(buf, 0) == 0x8d)
				return 0; // Ignore other player's chat
		break;
		case AREA_WOSC: {
			if(src_bl->type == BL_PC){
				struct map_session_data *ssd = (struct map_session_data *)src_bl;
				if (ssd && sd_chat(sd) && (sd_chat(sd) == sd_chat(ssd)))
				return 0;
			} else if(src_bl->type == BL_NPC) {
				struct npc_data *nds = BL_CAST(BL_NPC, src_bl);
				if (nds != NULL && sd_chat(sd) && (sd_chat(sd) == nds->chat_id))
					return 0;
			}
		}
		break;
		case AREA:
/* 0x120 crashes the client when warping for this packetver range [Ind/Hercules], thanks to Judas! */
#if PACKETVER > 20120418 && PACKETVER < 20130000
			if( WBUFW(buf, 0) == 0x120 && sd->state.warping )
				return 0;
#endif
			if (RBUFW(buf, 0) == 0x01c8 && bl != src_bl)
				return 0; // Ignore other player's item usage
			break;
	}

	/* unless visible, hold it here */
	if( clif->ally_only && !sd->sc.data[SC_CLAIRVOYANCE] && !sd->special_state.intravision && battle->check_target( src_bl, &sd->bl, BCT_ENEMY ) > 0 )
		return 0;

	if (bl == src_bl && bl->type == BL_PC) {
		clif_send_helper(sd, sd, buf, buf_new, len);
	} else {
		clif_send_helper(NULL, sd, buf, buf_new, len);
	}

	return clif->send_actual(fd, buf_new, len);
}

bool clif_send_pre(const void** buf_, int* len_, struct block_list** bl_, enum send_target* type_)
{
	const void *buf = *buf_;
	int len = *len_;
	struct block_list *bl = *bl_;
	enum send_target type = *type_;
	struct map_session_data *sd;
	unsigned short cmd = RBUFW(buf, 0);
	bool isStop = false;

	int i;
	struct map_session_data *tsd;
	struct party_data *p = NULL;
	struct guild *g = NULL;
	struct battleground_data *bgd = NULL;
	int x0 = 0, x1 = 0, y0 = 0, y1 = 0, fd;
	struct s_mapiterator* iter;
	void *buf_new = NULL;

	sd = BL_CAST(BL_PC, bl);

	int bool_ = 0;

	switch(cmd) {
#if PACKETVER < 20091103
		case idle_unit2Type:   // packet_idle_unit2
		case spawn_unit2Type:  // packet_spawn_unit2
#endif
		case idle_unitType:    // packet_idle_unit
		case spawn_unitType:   // packet_spawn_unit
		case unit_walkingType: // packet_unit_walking 
		case viewequipackType: // packet_viewequip_ack viewequip_list;
		case 0xc3:
		case 0x1d7:
			isStop = false;
			break;
		default:
			isStop = true;
			break;
	}
	if (isStop)
		return true;

	buf_new = (void*) aMalloc(len);
	memcpy(buf_new, buf, len);	// Copy Original Packet
	switch(type) {
	
		case ALL_CLIENT: //All player clients.
			iter = mapit_getallusers();
			while( (tsd = (TBL_PC*)mapit->next(iter)) != NULL ) {
				clif_send_helper(sd, tsd, buf, buf_new, len);
			}
			mapit->free(iter);
			break;

		case ALL_SAMEMAP: //All players on the same map
			iter = mapit_getallusers();
			while ((tsd = (TBL_PC*)mapit->next(iter)) != NULL) {
				if (bl && bl->m == tsd->bl.m) {
					clif_send_helper(sd, tsd, buf, buf_new, len);
				}
			}
			mapit->free(iter);
			break;

		case AREA:
		case AREA_WOSC:
			if (sd && bl->prev == NULL) //Otherwise source misses the packet.[Skotlex]
				clif->send(buf, len, bl, SELF);
			/* Fall through */
		case AREA_WOC:
		case AREA_WOS:
			nullpo_retr(true, bl);
			map->foreachinarea(clif_send_sub_hook, bl->m, bl->x-AREA_SIZE, bl->y-AREA_SIZE, bl->x+AREA_SIZE, bl->y+AREA_SIZE,
				BL_PC, buf, len, bl, type, bool_, buf_new);
			break;
		case AREA_CHAT_WOC:
			nullpo_retr(true, bl);
			map->foreachinarea(clif_send_sub_hook, bl->m, bl->x-(AREA_SIZE-5), bl->y-(AREA_SIZE-5),
			                   bl->x+(AREA_SIZE-5), bl->y+(AREA_SIZE-5), BL_PC, buf, len, bl, AREA_WOC, 0, buf_new);
			break;

		case CHAT:
		case CHAT_WOS:
			nullpo_retr(true, bl);
			{
				struct chat_data *cd;
				if (sd) {
					cd = (struct chat_data*)map->id2bl(sd_chat(sd));
				} else if (bl->type == BL_CHAT) {
					cd = (struct chat_data*)bl;
				} else break;
				if (cd == NULL)
					break;
				for(i = 0; i < cd->users; i++) {
					if (type == CHAT_WOS && cd->usersd[i] == sd)
						continue;
					if ((fd=cd->usersd[i]->fd) >0 && sockt->session[fd]) { // Added check to see if session exists [PoW]
						clif_send_helper(sd, cd->usersd[i], buf, buf_new, len);
					}
				}
			}
			break;

		case PARTY_AREA:
		case PARTY_AREA_WOS:
			nullpo_retr(true, bl);
			x0 = bl->x - AREA_SIZE;
			y0 = bl->y - AREA_SIZE;
			x1 = bl->x + AREA_SIZE;
			y1 = bl->y + AREA_SIZE;
			/* Fall through */
		case PARTY:
		case PARTY_WOS:
		case PARTY_SAMEMAP:
		case PARTY_SAMEMAP_WOS:
			if (sd && sd->status.party_id)
				p = party->search(sd->status.party_id);

			if (p) {
				for(i=0;i<MAX_PARTY;i++){
					if( (tsd = p->data[i].sd) == NULL )
						continue;

					if( !(fd = tsd->fd) )
						continue;

					if( tsd->bl.id == bl->id && (type == PARTY_WOS || type == PARTY_SAMEMAP_WOS || type == PARTY_AREA_WOS) )
						continue;

					if( type != PARTY && type != PARTY_WOS && bl->m != tsd->bl.m )
						continue;

					if( (type == PARTY_AREA || type == PARTY_AREA_WOS) && (tsd->bl.x < x0 || tsd->bl.y < y0 || tsd->bl.x > x1 || tsd->bl.y > y1) )
						continue;

					clif_send_helper(sd, tsd, buf, buf_new, len);
				}
				if (!map->enable_spy) //Skip unnecessary parsing. [Skotlex]
					break;

				iter = mapit_getallusers();
				while( (tsd = (TBL_PC*)mapit->next(iter)) != NULL ) {
					if( tsd->partyspy == p->party.party_id ) {
						clif_send_helper(sd, tsd, buf, buf_new, len);
					}
				}
				mapit->free(iter);
			}
			break;

		case DUEL:
		case DUEL_WOS:
			if (!sd || !sd->duel_group) break; //Invalid usage.

			iter = mapit_getallusers();
			while( (tsd = (TBL_PC*)mapit->next(iter)) != NULL ) {
				if( type == DUEL_WOS && bl->id == tsd->bl.id )
					continue;
				if( sd->duel_group == tsd->duel_group ) {
					clif_send_helper(sd, tsd, buf, buf_new, len);
				}
			}
			mapit->free(iter);
			break;

		case SELF:
			if (sd && (fd=sd->fd) != 0) {
				clif_send_helper(sd, sd, buf, buf_new, len);
			}
			break;

		// New definitions for guilds [Valaris] - Cleaned up and reorganized by [Skotlex]
		case GUILD_AREA:
		case GUILD_AREA_WOS:
			nullpo_retr(true, bl);
			x0 = bl->x - AREA_SIZE;
			y0 = bl->y - AREA_SIZE;
			x1 = bl->x + AREA_SIZE;
			y1 = bl->y + AREA_SIZE;
			/* Fall through */
		case GUILD_SAMEMAP:
		case GUILD_SAMEMAP_WOS:
		case GUILD:
		case GUILD_WOS:
		case GUILD_NOBG:
			if (sd && sd->status.guild_id)
				g = sd->guild;

			if (g) {
				for(i = 0; i < g->max_member; i++) {
					if( (tsd = g->member[i].sd) != NULL ) {
						if( !(fd = tsd->fd) || fd < 0 ) // Crash Fix
							continue;

						if (!sockt->session_is_valid(fd)) { // Crash Fix
							ShowError("Invalid guild member fd found: %d\n", fd);
							ShowError("Report to devs\n");
							Assert_report(0);
							ShowError("Trying to fix it...\n");
							g->member[i].sd = NULL;
						}

						if( type == GUILD_NOBG && tsd->bg_id )
							continue;

						if( tsd->bl.id == bl->id && (type == GUILD_WOS || type == GUILD_SAMEMAP_WOS || type == GUILD_AREA_WOS) )
							continue;

						if( type != GUILD && type != GUILD_NOBG && type != GUILD_WOS && tsd->bl.m != bl->m )
							continue;

						if( (type == GUILD_AREA || type == GUILD_AREA_WOS) && (tsd->bl.x < x0 || tsd->bl.y < y0 || tsd->bl.x > x1 || tsd->bl.y > y1) )
							continue;

						clif_send_helper(sd, tsd, buf, buf_new, len);
					}
				}
				if (!map->enable_spy) //Skip unnecessary parsing. [Skotlex]
					break;

				iter = mapit_getallusers();
				while( (tsd = (TBL_PC*)mapit->next(iter)) != NULL ) {
					if( tsd->guildspy == g->guild_id ) {
						clif_send_helper(sd, tsd, buf, buf_new, len);
					}
				}
				mapit->free(iter);
			}
			break;

		case BG_AREA:
		case BG_AREA_WOS:
			nullpo_retr(true, bl);
			x0 = bl->x - AREA_SIZE;
			y0 = bl->y - AREA_SIZE;
			x1 = bl->x + AREA_SIZE;
			y1 = bl->y + AREA_SIZE;
			/* Fall through */
		case BG_SAMEMAP:
		case BG_SAMEMAP_WOS:
		case BG:
		case BG_WOS:
			if( sd && sd->bg_id && (bgd = bg->team_search(sd->bg_id)) != NULL ) {
				for( i = 0; i < MAX_BG_MEMBERS; i++ ) {
					if( (tsd = bgd->members[i].sd) == NULL || !(fd = tsd->fd) )
						continue;
					if( tsd->bl.id == bl->id && (type == BG_WOS || type == BG_SAMEMAP_WOS || type == BG_AREA_WOS) )
						continue;
					if( type != BG && type != BG_WOS && tsd->bl.m != bl->m )
						continue;
					if( (type == BG_AREA || type == BG_AREA_WOS) && (tsd->bl.x < x0 || tsd->bl.y < y0 || tsd->bl.x > x1 || tsd->bl.y > y1) )
						continue;
					clif_send_helper(sd, tsd, buf, buf_new, len);
				}
			}
			break;

		case BG_QUEUE:
			if( sd && sd->bg_queue.arena ) {
				struct script_queue *queue = script->queue(sd->bg_queue.arena->queue_id);

				for (i = 0; i < VECTOR_LENGTH(queue->entries); i++) {
					struct map_session_data *qsd = map->id2sd(VECTOR_INDEX(queue->entries, i));

					if (qsd != NULL) {
						clif_send_helper(sd, qsd, buf, buf_new, len);
					}
				}
			}
			break;

		default:
			ShowError("clif_send: Unrecognized type %d\n", (int)type);
			aFree(buf_new);
			hookStop();
			return false;
	}
	aFree(buf_new);
	hookStop();
	return true;
}


HPExport void plugin_init(void)
{
	addAtcommand("noview", noview);
	addHookPre(clif, send, clif_send_pre);
}

HPExport void server_online (void) {
	ShowInfo("'%s' Plugin by Dastgir/Hercules, Version '%s'(Build Date: '%s')\n", pinfo.name, pinfo.version, updated_at);
}
