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

HPExport struct hplugin_info pinfo =
{
	"@afk",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

ACMD(afk)
{
	sd->state.autotrade = 1;
	sd->state.monster_ignore = 1;
	pc_setsit(sd);
	skill->sit(sd,1);
	clif->sitting(&sd->bl);
	clif->changelook(&sd->bl,LOOK_HEAD_TOP,471);
	clif->specialeffect(&sd->bl, 234,AREA);
	clif->authfail_fd(fd, 15);
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

	addAtcommand("afk",afk);
}

HPExport void server_online (void) {
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}

