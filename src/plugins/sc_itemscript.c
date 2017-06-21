//===== Hercules Plugin ======================================
//= Status ItemScript
//===== By: ==================================================
//= Dastgir[Hercules]
//===== Current Version: =====================================
//= 1.0
//===== Compatible With: ===================================== 
//= Hercules
//===== Description: =========================================
//= 
//===== Usage: ===============================================
//= sc_start(SC_ITEMSCRIPT, Tick, ItemID);
//============================================================


#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/utils.h"
#include "common/memmgr.h"
#include "common/nullpo.h"
#include "common/random.h"

#include "map/itemdb.h"
#include "map/pc.h"
#include "map/script.h"
#include "map/status.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {	
	"sc itemscript",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

#ifdef SC_MAX
	#define SC_ITEMSCRIPT SC_MAX+100	// Enough place for future new values
	#undef SC_MAX
#else
	#define SC_ITEMSCRIPT SC_DRESS_UP+100	// Enough place for future new values
#endif
#define SC_MAX SC_ITEMSCRIPT+1

void status_calc_pc_additional_post(struct map_session_data *sd, enum e_status_calc_opt opt)
{
	struct status_change *sc;

	if (sd == NULL)
		return;

	sc = &sd->sc;

	if (sc != NULL && sc->count > 0 && sc->data[SC_ITEMSCRIPT] != NULL) {
		struct item_data *data = itemdb->exists(sc->data[SC_ITEMSCRIPT]->val1);
		if (data && data->script)
			script->run_use_script(sd, data, 0);
	}
}

HPExport void plugin_init(void)
{
	addHookPost(status, calc_pc_additional, status_calc_pc_additional_post);

	script->set_constant("SC_ITEMSCRIPT", SC_ITEMSCRIPT, false, false);
	script->set_constant("SC_MAX", SC_MAX, false, false);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
