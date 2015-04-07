// AuraSet (By Dastgir/ Hercules) Plugin v1.2

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../common/HPMi.h"
#include "../common/mmo.h"
#include "../common/socket.h"
#include "../common/malloc.h"
#include "../common/strlib.h"
#include "../common/nullpo.h"
#include "../common/timer.h"


#include "../map/script.h"
#include "../map/pc.h"
#include "../map/clif.h"
#include "../map/map.h"
#include "../map/status.h"
#include "../map/npc.h"
#include "../map/mob.h"

#include "../common/HPMDataCheck.h" /* should always be the last file included! (if you don't make it last, it'll intentionally break compile time) */

HPExport struct hplugin_info pinfo = {	//[Dastgir/Hercules]
	"AuraSet",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.2",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

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

/*==========================================
* Aura [Dastgir/Hercules]
*------------------------------------------*/

BUILDIN(aura){
	int aura =-1, aura1 =-1, aura2 =-1;
	TBL_PC* sd = script->rid2sd(st);

	if (sd == NULL){
		script_pushint(st, 0);
		return false;
	}
	
	aura = script_getnum(st, 2);
	if (script_hasdata(st, 3) && script_isinttype(st,3)){
		aura1 = script_getnum(st, 3);
	}
	if (script_hasdata(st, 4) && script_isinttype(st, 4)){
		aura2 = script_getnum(st, 4);
	}

	if (aura < -1 || aura1 < -1 || aura2 < -1){
		script_pushint(st, 0);
		return false;
	}
	
	if (aura>=0){ pc_setglobalreg(sd, script->add_str("USERAURA"), aura); }
	if (aura1>=0){ pc_setglobalreg(sd, script->add_str("USERAURA1"), aura1); }
	if (aura2>=0){ pc_setglobalreg(sd, script->add_str("USERAURA2"), aura2); }
	pc->setpos(sd, sd->mapindex, sd->bl.x, sd->bl.y, (clr_type)CLR_RESPAWN);
	script_pushint(st, 1);
	return true;
}

/*==========================================
* Aura [Dastgir/Hercules]
*------------------------------------------*/
ACMD(aura){
	int aura =-1, aura1 =-1, aura2 =-1;

	if (!message || !*message || sscanf(message, "%d %d %d", &aura, &aura1, &aura2) < 1){
		clif->message(fd, "Please, enter at least an option (usage: @aura <aura> {<aura1> <aura2>}).");
		return false;
	}
	

	if (aura>=0){ pc_setglobalreg(sd, script->add_str("USERAURA"), aura); }
	if (aura1>=0){ pc_setglobalreg(sd, script->add_str("USERAURA1"), aura1); }
	if (aura2>=0){ pc_setglobalreg(sd, script->add_str("USERAURA2"), aura2); }
	pc->setpos(sd, sd->mapindex, sd->bl.x, sd->bl.y, (clr_type)CLR_RESPAWN);
	clif->message(fd, "Aura has been Set.");

	return true;
}

void clif_sendaurastoone(struct map_session_data *sd, struct map_session_data *dsd)
{
	int effect1, effect2, effect3;

	if (pc_ishiding(sd))
		return;

	effect1 = pc_readglobalreg(sd, script->add_str("USERAURA"));
	effect2 = pc_readglobalreg(sd, script->add_str("USERAURA1"));
	effect3 = pc_readglobalreg(sd, script->add_str("USERAURA2"));

	if (effect1 >= 0)
		clif->specialeffect_single(&sd->bl, effect1, dsd->fd);
	if (effect2 >= 0)
		clif->specialeffect_single(&sd->bl, effect2, dsd->fd);
	if (effect3 >= 0)
		clif->specialeffect_single(&sd->bl, effect3, dsd->fd);
}

void clif_sendauras(struct map_session_data *sd, enum send_target type)
{
	int effect1, effect2, effect3;

	if (pc_ishiding(sd))
		return;

	effect1 = pc_readglobalreg(sd, script->add_str("USERAURA"));
	effect2 = pc_readglobalreg(sd, script->add_str("USERAURA1"));
	effect3 = pc_readglobalreg(sd, script->add_str("USERAURA2"));

	if (effect1 >= 0)
		clif->specialeffect(&sd->bl, effect1, type);
	if (effect2 >= 0)
		clif->specialeffect(&sd->bl, effect2, type);
	if (effect3 >= 0)
		clif->specialeffect(&sd->bl, effect3, type);
}

bool clif_spawn_AuraPost(bool retVal, struct block_list *bl){	//[Dastgir/Hercules]
	struct view_data *vd;

	vd = status->get_viewdata(bl);
	if (retVal == false) { return false; }
	if ((bl->type == BL_NPC
		&& !((TBL_NPC*)bl)->chat_id
		&& (((TBL_NPC*)bl)->option&OPTION_INVISIBLE)) // Hide NPC from maya purple card.
		|| (vd->class_ == INVISIBLE_CLASS)
		)
		return true; // Doesn't need to be spawned, so everything is alright
	if (bl->type == BL_PC){
		clif_sendauras((TBL_PC*)bl, AREA);
	}
	return true;
}

void clif_getareachar_unit_AuraPost(struct map_session_data* sd, struct block_list *bl) {	//[Dastgir/Hercules]
//	struct unit_data *ud;
	struct view_data *vd;

	vd = status->get_viewdata(bl);
	if (!vd || vd->class_ == INVISIBLE_CLASS)
		return;

	/**
	* Hide NPC from maya purple card.
	**/
	if (bl->type == BL_NPC && !((TBL_NPC*)bl)->chat_id && (((TBL_NPC*)bl)->option&OPTION_INVISIBLE))
		return;
	if (bl->type == BL_PC){
		TBL_PC* tsd = (TBL_PC*)bl;
		clif_sendaurastoone(tsd, sd);
		return;
	}
}

int clif_insight2(struct block_list *bl, va_list ap)
{
	struct block_list *tbl;
	struct map_session_data *sd, *tsd;
	int flag;

	tbl = va_arg(ap, struct block_list*);
	flag = va_arg(ap, int);

	if (bl == tbl && !flag)
		return 0;

	sd = BL_CAST(BL_PC, bl);
	tsd = BL_CAST(BL_PC, tbl);

	if (sd && sd->fd)
	{
		if (bl == tbl)
			clif_sendaurastoone(sd, tsd);
		else
			clif->getareachar_unit(sd, tbl);
	}

	return 0;
}

void clif_getareachar_char(struct block_list *bl, short flag)
{
	map->foreachinarea(clif_insight2, bl->m, bl->x - AREA_SIZE, bl->y - AREA_SIZE, bl->x + AREA_SIZE, bl->y + AREA_SIZE, BL_PC, bl, flag);
}

int status_change_start_postAura(int retVal,struct block_list *src, struct block_list *bl, enum sc_type type, int rate, int val1, int val2, int val3, int val4, int tick, int flag) {	//[Dastgir/Hercules]
	struct map_session_data *sd = NULL;
	int effect1, effect2, effect3;

	if (retVal == 0){ return 0; }
	if (bl->type != BL_PC){ return 0; }

	sd = BL_CAST(BL_PC, bl);
	effect1 = pc_readglobalreg(sd, script->add_str("USERAURA"));
	effect2 = pc_readglobalreg(sd, script->add_str("USERAURA1"));
	effect3 = pc_readglobalreg(sd, script->add_str("USERAURA2"));

	if (sd && (effect1 > 0 || effect2>0 || effect3>0) && (type == SC_HIDING || type == SC_CLOAKING || type == SC_CHASEWALK || sd->sc.option == OPTION_INVISIBLE || type == SC_CHASEWALK || type == SC_CHASEWALK2 || type == SC_CAMOUFLAGE)){
		pc_setglobalreg(sd,script->add_str("USERAURA"),effect1*-1);
		pc_setglobalreg(sd,script->add_str("USERAURA1"),effect2*-1);
		pc_setglobalreg(sd,script->add_str("USERAURA2"),effect3*-1);
		clif->clearunit_area(&sd->bl, 0);
		clif_getareachar_char(&sd->bl, 0);
	}
	return 1;
}

int status_change_end_postAura(int retVal, struct block_list* bl, enum sc_type type, int tid, const char* file, int line) {	//[Dastgir/Hercules]
	struct map_session_data *sd;
	struct status_change *sc;
	struct status_change_entry *sce;
	int effect1, effect2, effect3;
	if (retVal == 0){ return 0; }
	if (bl == NULL){ return 0;  }
	sc = status->get_sc(bl);

	if (type < 0 || type >= SC_MAX || !sc || !(sce = sc->data[type]))
		return 0;

	sd = BL_CAST(BL_PC, bl);

	if (sce->timer != tid && tid != INVALID_TIMER)
		return 0;


	effect1 = pc_readglobalreg(sd, script->add_str("USERAURA"));
	effect2 = pc_readglobalreg(sd, script->add_str("USERAURA1"));
	effect3 = pc_readglobalreg(sd, script->add_str("USERAURA2"));

	if (sd && (effect1<0 || effect2<0 || effect3<0) && (type == SC_HIDING || type == SC_CLOAKING || type == SC_CHASEWALK || sd->sc.option==OPTION_INVISIBLE || type==SC_CHASEWALK || type==SC_CHASEWALK2 || type==SC_CAMOUFLAGE)){
		pc_setglobalreg(sd,script->add_str("USERAURA"),effect1*-1);
		pc_setglobalreg(sd,script->add_str("USERAURA1"),effect2*-1);
		pc_setglobalreg(sd,script->add_str("USERAURA2"),effect3*-1);
		clif_sendauras(sd, AREA_WOS);
	}
	return 1;
}

void clif_sendauraself(struct map_session_data *sd){
	clif_sendaurastoone(sd,sd);
}

/* run when server starts */
HPExport void plugin_init(void) {	//[Dastgir/Hercules]

	/* core interfaces */
	iMalloc = GET_SYMBOL("iMalloc");

	/* map-server interfaces */
	script = GET_SYMBOL("script");
	clif = GET_SYMBOL("clif");
	pc = GET_SYMBOL("pc");
	strlib = GET_SYMBOL("strlib");
	map = GET_SYMBOL("map");
	status = GET_SYMBOL("status");
	npc = GET_SYMBOL("npc");
	mob = GET_SYMBOL("mob");

	addAtcommand("aura", aura);
	addScriptCommand("aura", "i??", aura);
	addHookPost("clif->spawn", clif_spawn_AuraPost);
	addHookPost("clif->getareachar_unit", clif_getareachar_unit_AuraPost);
	addHookPost("status->change_end_", status_change_end_postAura);
	addHookPost("status->change_start", status_change_start_postAura);
	addHookPost("clif->refresh",clif_sendauraself);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}