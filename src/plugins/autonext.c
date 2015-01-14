/*
	By Shikazu
	Edited by Dastgir/Hercules
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/HPMi.h"
#include "../common/malloc.h"
#include "../common/mmo.h"
#include "../common/socket.h"
#include "../common/strlib.h"
#include "../map/clif.h"
#include "../map/script.h"
#include "../map/skill.h"
#include "../map/pc.h"
#include "../map/map.h"

#include "../common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"AutoNext Script Command",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

BUILDIN(autonext) 
{
	TBL_PC* sd;
	int timeout;
	
	sd = script->rid2sd(st);
	if( sd == NULL )
		return true;
#ifdef SECURE_NPCTIMEOUT
	sd->npc_idle_type = NPCT_WAIT;
#endif
	//script_detach_rid(st);

	timeout=script_getnum(st,2);

	if(st->sleep.tick == 0)
	{
		st->state = RERUNLINE;
		st->sleep.tick = timeout;
	}
	else
	{// sleep time is over
		st->state = RUN;
		st->sleep.tick = 0;
	}

	clif->scriptnext(sd, st->oid);
	return true;
}

/* Server Startup */
HPExport void plugin_init (void) 
{
	clif = GET_SYMBOL("clif");
	script = GET_SYMBOL("script");
	skill = GET_SYMBOL("skill");
	pc = GET_SYMBOL("pc");
	strlib = GET_SYMBOL("strlib");

	addScriptCommand("autonext","i",autonext);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}