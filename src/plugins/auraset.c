//===== Hercules Plugin ======================================
//= AuraSet
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.5
//===== Description: =========================================
//= Set's an effect for infinite duration
//===== Changelog: ===========================================
//= v1.0 - Initial Conversion
//= v1.1 - Dead Person cannot @afk.
//= v1.2 - Added afk_timeout option and battle_config for it
//= v1.3 - Added noafk mapflag.
//= v1.4 - Compatible with new Hercules.
//= v1.5 - Added MAX_AURA option(ability to have x Aura's)
//===== Repo Link: ===========================================
//= https://github.com/dastgir/HPM-Plugins
//============================================================

#include "common/hercules.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "common/HPMi.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/memmgr.h"
#include "common/strlib.h"
#include "common/nullpo.h"
#include "common/timer.h"

#include "map/battle.h"
#include "map/script.h"
#include "map/pc.h"
#include "map/clif.h"
#include "map/status.h"
#include "map/npc.h"
#include "map/mob.h"
#include "map/map.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {   // [Dastgir/Hercules]
	"AuraSet",
	SERVER_TYPE_MAP,
	"1.5",
	HPM_VERSION,
};

struct hide_data {
	bool hidden;         // Is Player hidden?
};

#define MAX_AURA 3
#define AURA_VARIABLE "USERAURA"
// OUPUT_LENGTH should be Length Of (AURA_VARIABLE + Digits in MAX_AURA + 1)
#define OUTPUT_LENGTH 8+1+1

struct hide_data* hd_search(struct map_session_data* sd)
{
	struct hide_data *data;
	if ((data = getFromMSD(sd,0)) == NULL) {
		CREATE(data,struct hide_data,1);
		data->hidden = false;
		addToMSD(sd,data,0,true);
	}
	return data;
}

/*==========================================
* Aura [Dastgir/Hercules]
*------------------------------------------*/

BUILDIN(aura)
{
	int aura[MAX_AURA] = { -1 };
	int i;
	char output[OUTPUT_LENGTH];
	struct map_session_data *sd = script->rid2sd(st);

	if (sd == NULL) {
		script_pushint(st, 0);
		return false;
	}
	
	// Set the Aura
	for (i = 0; i < MAX_AURA; ++i) {
		if (i == 0 ||
			(script_hasdata(st, 2+i) && script_isinttype(st, 2+i))
		) {
			aura[i] = script_getnum(st, 2+i);
		}
		if (aura[i] < -1) {
			script_pushint(st, 0);
			return false;
		}
		if (aura[i] >= 0) {
			sprintf(output, "%s%d", AURA_VARIABLE, i+1);
			pc_setglobalreg(sd, script->add_str(output), aura[i]);
		}
	}

	// Refresh
	pc->setpos(sd, sd->mapindex, sd->bl.x, sd->bl.y, (clr_type)CLR_RESPAWN);
	script_pushint(st, 1);
	return true;
}

/**
 * Aura [Dastgir/Hercules]
 * Limited to 3 Aura's
 */
ACMD(aura)
{
	int aura[3] = { -1 };
	int i;
	char output[OUTPUT_LENGTH];

	if (!message || !*message || sscanf(message, "%d %d %d", &aura[0], &aura[1], &aura[2]) < 1){
		clif->message(fd, "Please, enter at least an option (usage: @aura <aura1> {<aura2> <aura3>}).");
		return false;
	}
	
	for (i = 0; i < 3; ++i){
		if (aura[i] >= 0) {
			sprintf(output, "%s%d", AURA_VARIABLE, i+1);
			pc_setglobalreg(sd, script->add_str(output), aura[i]);
		}
	}
	
	// Respawn
	pc->setpos(sd, sd->mapindex, sd->bl.x, sd->bl.y, (clr_type)CLR_RESPAWN);
	clif->message(fd, "Aura has been Set.");

	return true;
}

void clif_sendaurastoone(struct map_session_data *sd, struct map_session_data *dsd)
{
	int effect, i;
	char output[OUTPUT_LENGTH];

	if (pc_ishiding(sd))
		return;
	
	for (i = 0; i < MAX_AURA; ++i) {
		sprintf(output, "%s%d", AURA_VARIABLE, i+1);
		effect = pc_readglobalreg(sd, script->add_str(output));
		if (effect > 0)
			clif->specialeffect_single(&sd->bl, effect, dsd->fd);
	}

	return;
}

void clif_sendauras(struct map_session_data *sd, enum send_target type, bool is_hidden)
{
	int effect, i;
	char output[OUTPUT_LENGTH];

	if (pc_ishiding(sd) && is_hidden==true)
		return;
	
	for (i = 0; i < MAX_AURA; ++i) {
		sprintf(output, "%s%d", AURA_VARIABLE, i+1);
		effect = pc_readglobalreg(sd, script->add_str(output));
		if (effect > 0)
			clif->specialeffect(&sd->bl, effect, type);
	}
	
	return;
}

// [Dastgir/Hercules]
bool clif_spawn_post(bool retVal, struct block_list *bl)
{
	struct view_data *vd;
	vd = status->get_viewdata(bl);
	if (retVal == false)
		return retVal;
	if (vd->class_ == INVISIBLE_CLASS)
		return true; // Doesn't need to be spawned, so everything is alright
	
	if (bl->type == BL_PC)
		clif_sendauras((TBL_PC*)bl, AREA, true);

	return true;
}

// [Dastgir/Hercules]
void clif_getareachar_unit_post(struct map_session_data *sd, struct block_list *bl)
{
	
	struct view_data *vd;

	vd = status->get_viewdata(bl);
	if (vd == NULL)
		return;

	if (bl->type == BL_PC) {
		struct map_session_data *tsd = BL_CAST(BL_PC, bl);
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

	if (sd && sd->fd) {
		if (bl == tbl)
			clif_sendaurastoone(sd, tsd);
		else
			clif->getareachar_unit(sd, tbl);
	}

	return 0;
}

void clif_getareachar_char(struct block_list *bl, short flag)
{
	map->foreachinarea( clif_insight2,
						bl->m,
						bl->x - AREA_SIZE,
						bl->y - AREA_SIZE,
						bl->x + AREA_SIZE,
						bl->y + AREA_SIZE,
						BL_PC,
						bl,
						flag);
}

// [Dastgir/Hercules]
int status_change_start_post(int retVal, struct block_list *src, struct block_list *bl, enum sc_type type, int rate, int val1, int val2, int val3, int val4, int tick, int flag)
{
	struct map_session_data *sd = NULL;
	struct hide_data *data;

	if (retVal == 0 || bl->type != BL_PC)
		return retVal;

	sd = BL_CAST(BL_PC, bl);
	data = hd_search(sd);

	if (sd &&
		data->hidden==false &&
		(type == SC_HIDING || type == SC_CLOAKING || type == SC_CHASEWALK || sd->sc.option == OPTION_INVISIBLE || type == SC_CHASEWALK || type == SC_CHASEWALK2 || type == SC_CAMOUFLAGE)
		){
		data->hidden = true;
		clif->clearunit_area(&sd->bl, 0);
		clif_getareachar_char(&sd->bl, 0);
	}
	return retVal;
}

// [Dastgir/Hercules]
int status_change_end_pre(struct block_list **bl, enum sc_type *type_, int *tid_, const char **file, int *line)
{
	struct map_session_data *sd;
	struct status_change *sc;
	struct status_change_entry *sce;
	enum sc_type type = *type_;
	struct hide_data* data;
	int tid = *tid_;

	if (*bl == NULL)
		return 0;
	
	sc = status->get_sc(*bl);
	
	if (type < 0 || type >= SC_MAX || !sc || !(sce = sc->data[type]))
		return 0;

	sd = BL_CAST(BL_PC, *bl);
	
	if (sce->timer != tid && tid != INVALID_TIMER && sce->timer != INVALID_TIMER)
		return 0;

	if (sd) {
		data = hd_search(sd);
		if (data->hidden==true &&
			(type == SC_HIDING || type == SC_CLOAKING || type == SC_CHASEWALK || sd->sc.option==OPTION_INVISIBLE || type==SC_CHASEWALK || type==SC_CHASEWALK2 || type==SC_CAMOUFLAGE)
			){
			data->hidden = false;
			clif_sendauras(sd, AREA, false);
		}
	}
	return 1;
}

void clif_refresh_post(struct map_session_data *sd)
{
	clif_sendaurastoone(sd, sd);
}


HPExport void plugin_init(void)
{
	char output[MAX_AURA+1] = "i";
	int i;
	for (i = 1; i < MAX_AURA; ++i)
		sprintf(output, "%s?", output);
	
	addAtcommand("aura", aura);
	addScriptCommand("aura", output, aura);

	addHookPre(status, change_end_, status_change_end_pre);	
	addHookPost(clif, spawn, clif_spawn_post);
	addHookPost(clif, getareachar_unit, clif_getareachar_unit_post);
	addHookPost(clif, refresh, clif_refresh_post);
	addHookPost(status, change_start, status_change_start_post);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}
