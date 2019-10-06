//===== Hercules Plugin ======================================
//= Colored DispBottom
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
	"Colored dispbottom",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

void clif_displaymessagecolor(struct map_session_data *sd, const char* msg, uint32 color)
{
	int fd;
	unsigned short len = strlen(msg) + 1;
	
	if (sd==NULL)
		return;
	
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

BUILDIN(dispbottom2) // Format : dispbottom2(0xFF00FF,"Message"{,"Player Name"});
{
	struct map_session_data *sd = script->rid2sd(st); // Player Data
	const char *message;             // Message to Display
	uint32 color;             // Color to display
	message = script_getstr(st,3);
	color = script_getnum(st, 2);
	if (script_hasdata(st,4)) {
		const char* player;
		struct map_session_data *tsd;
		player = script_getstr(st, 4);
		tsd = map->nick2sd(player, false);
		if (tsd != NULL)
			clif_displaymessagecolor(tsd, message, color);
		return true;
	}
	if (sd != NULL)
		clif_displaymessagecolor(sd, message, color);
	return true;
}

HPExport void plugin_init(void)
{
	addScriptCommand("dispbottom2", "ss?", dispbottom2);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
