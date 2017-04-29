#include "common/hercules.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "common/HPMi.h"
#include "common/nullpo.h"
#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/sql.h"
#include "common/showmsg.h"

#include "char/char.h"

#undef DEFAULT_AUTOSAVE_INTERVAL	//Its defined in map.h too..

#include "map/atcommand.h"
#include "map/chrif.h"
#include "map/map.h"
#include "map/pc.h"
#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"


HPExport struct hplugin_info pinfo = {
	"Plugin",		// Plugin name
	SERVER_TYPE_MAP|SERVER_TYPE_CHAR,// Which server types this plugin works with?
	"1.0",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

int common_value = 0;

#define chrif_check(a) do { if(!chrif->isconnected()) return a; } while(0)

bool map_sendtochar(void)
{
	chrif_check(false);

	WFIFOHEAD(chrif->fd,6);
	WFIFOW(chrif->fd,0) = 0x2b40;
	WFIFOL(chrif->fd,2) = common_value;
	WFIFOSET(chrif->fd,6);

	return true;
}

ACMD(packet_test)
{
	if (!*message) {
		clif->message(fd, "Enter the Value");
		return false;
	}
	
	common_value = atoi(message);
	map_sendtochar();
	ShowDebug("MAP: Common Value Set to %d\n",common_value);
	
	return true;
}

void char_receive_packet(int fd) {
	int value = RFIFOL(fd, 2);
	ShowDebug("Char: CommonValue(Before parsing packet): %d\n",common_value);
	common_value = value;
	ShowDebug("Char: CommonValue(Before parsing packet): %d\n",common_value);
	return;
}

/* Server Startup */
HPExport void plugin_init(void)
{
	switch(SERVER_TYPE){
		case SERVER_TYPE_MAP:
			addAtcommand("packet_test", packet_test);
			break;
		case SERVER_TYPE_CHAR:
			addPacket(0x2b40,6,char_receive_packet,hpParse_FromMap);			
			break;
	}
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
