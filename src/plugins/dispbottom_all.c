//===== Hercules Plugin ======================================
//= DispBottom All
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Adds color to dispbottom
//===== Changelog: ===========================================
//= v1.0 - Initial Release [Dastgir]
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

#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"dispbottom All",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

BUILDIN(dispbottom_all) // Format : dispbottom_all("Message");
{
	struct map_session_data *sd = script->rid2sd(st);
	const char *message = script_getstr(st,2);

	if (sd == NULL)
		return true;

	clif->disp_message(&(sd->bl), message, ALL_CLIENT);

	return true;
}

HPExport void plugin_init (void) 
{
	addScriptCommand("dispbottom_all","s",dispbottom_all);
}

HPExport void server_online (void)
{
	ShowInfo ("'%s' Plugin by Dastgir/Hercules. Version '%s'\n",pinfo.name,pinfo.version);
}
