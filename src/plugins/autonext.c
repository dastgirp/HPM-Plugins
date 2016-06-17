//===== Hercules Plugin ======================================
//= autonext
//===== By: ==================================================
//= Dastgir/Hercules
//= original by Shikazu
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Clears the NPC dialog box and acts as if next button has
//= been pressed
//===== Changelog: ===========================================
//= v1.0 - Initial Conversion
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
#include "common/socket.h"
#include "common/strlib.h"
#include "map/clif.h"
#include "map/script.h"
#include "map/skill.h"
#include "map/pc.h"
#include "map/map.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"autonext",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

BUILDIN(autonext) 
{
	struct map_session_data *sd;
	int timeout = script_getnum(st, 2);
	
	sd = script->rid2sd(st);
	if (sd == NULL)
		return true;

#ifdef SECURE_NPCTIMEOUT
	sd->npc_idle_type = NPCT_WAIT;
#endif

	if(st->sleep.tick == 0) {
		st->state = RERUNLINE;
		st->sleep.tick = timeout;
	} else { // sleep time is over
		st->state = RUN;
		st->sleep.tick = 0;
	}

	clif->scriptnext(sd, st->oid);
	return true;
}

HPExport void plugin_init (void) 
{
	addScriptCommand("autonext", "i", autonext);
}

HPExport void server_online (void)
{
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}