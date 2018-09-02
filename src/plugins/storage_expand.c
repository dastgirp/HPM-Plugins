//===== Hercules Plugin ======================================
//= Storage Expansion
//===== By: ==================================================
//= Dastgir/Hercules
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Expands Storage Size.
//= If this plugin is enabled, Server can have any number of
//= storage size defined.
//===== Instruction: =========================================
//= Enable This Plugin
//= Change MAX_STORAGE from common/mmo.h
//= Ensure storage_expansion = true; in this plugin
//===== Changelog: ===========================================
//= v1.0 - Initial Conversion
//===== Repo Link: ===========================================
//= https://github.com/dastgir/HPM-Plugins
//===== Hercules Topic: ======================================
//= http://herc.ws/board/topic/15636-infinite-storage/
//============================================================
#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common/HPMi.h"
#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/nullpo.h"
#include "common/socket.h"
#include "common/strlib.h"
#include "map/chrif.h"
#include "map/pc.h"
#include "map/map.h"
#include "map/intif.h"
#undef DEFAULT_AUTOSAVE_INTERVAL
#include "char/char.h"
#include "char/mapif.h"
#include "char/int_storage.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"Storage Expansion",
	SERVER_TYPE_MAP|SERVER_TYPE_CHAR,
	"1.0",
	HPM_VERSION,
};

bool storage_expansion = true;

#define inter_fd (chrif->fd)

struct storage_plugin_data {
	struct storage_data p_stor;
	int total_parts, current_part, count;
};

/**
 * Send account storage information for saving.
 * @packet 0x3011 [out] <packet_len>.W <account_id>.L <struct item[]>.P
 * @param  sd     [in]  pointer to session data.
 */
void intif_send_account_storage_pre(struct map_session_data **sd_)
{
	struct map_session_data *sd = *sd_;
	int len = 0, i = 0, c = 0;
	int16 parts = 1;
	int offset = 12, j = 0, len_t = 0;

	if (!storage_expansion)
		return;
	nullpo_retv(sd);

	// Assert that at this point in the code, both flags are true.
	Assert_retv(sd->storage.save == true);
	Assert_retv(sd->storage.received == true);

	if (intif->CheckForCharServer())
		return;

	len = sd->storage.aggregate * sizeof(struct item);
	if (len == 0)
		len = 1;
	parts = ceil(len/65535.0f);

	for (j = 0; j < parts; j++) {
		len_t = 12;
		for (c = 0; i < VECTOR_LENGTH(sd->storage.item); i++) {
			if (VECTOR_INDEX(sd->storage.item, i).nameid == 0)
				continue;
			if (len_t + sizeof(struct item) > 65535)
				break;
			memcpy(WFIFOP(inter_fd, offset + c * sizeof(struct item)), &VECTOR_INDEX(sd->storage.item, i), sizeof(struct item));
			len_t += sizeof(struct item);
			c++;
		}
		WFIFOHEAD(inter_fd, len_t);

		WFIFOW(inter_fd, 0) = 0x3011;
		WFIFOW(inter_fd, 2) = (uint16) len_t;
		WFIFOL(inter_fd, 4) = sd->status.account_id;
		WFIFOW(inter_fd, 8) = j;	// Current Part
		WFIFOW(inter_fd, 10) = parts;	// Total Parts

		WFIFOSET(inter_fd, len_t);
		//ShowInfo("Sent %d(%d) Items (%d/%d), Length: %d(%d), Expected: %d\n", c, i, j, parts, len_t, len, sd->storage.aggregate);
	}
	sd->storage.save = false; // Save Request Sent.
	hookStop();
}

/**
 * Parse the reception of account storage from the inter-server.
 * @packet 0x3805 [in] <packet_len>.W <account_id>.L <struct item[]>.P
 * @param  fd     [in] file/socket descriptor.
 */
void intif_parse_account_storage_pre(int *fd_)
{
	int fd = *fd_;
	int account_id = 0, payload_size = 0, storage_count = 0;
	int i = 0;
	int total_parts;
	struct map_session_data *sd = NULL;
	struct storage_plugin_data *data;

	Assert_retv(fd > 0);

	if (!storage_expansion)
		return;

	payload_size = RFIFOW(fd, 2) - 12;
	total_parts = RFIFOW(fd, 10);
	//parts = RFIFOW(fd, 8);

	if ((account_id = RFIFOL(fd, 4)) == 0 || (sd = map->id2sd(account_id)) == NULL) {
		ShowError("intif_parse_account_storage: Session pointer was null for account id %d!\n", account_id);
		return;
	}

	if (sd->storage.received == true) {
		ShowError("intif_parse_account_storage: Multiple calls from the inter-server received.\n");
		return;
	}

	if (!(data = getFromSession(sockt->session[fd],1))) {
		CREATE(data,struct storage_plugin_data,1);
		addToSession(sockt->session[fd],data,1,true);
		memset(&data->p_stor, 0, sizeof(data->p_stor));
		data->total_parts = total_parts;
		data->current_part = 0;
		data->count = 0;
	}

	//ShowInfo("Received %d/%d, Size: %d. AccountID: %d\n", RFIFOW(fd, 8), total_parts, payload_size, account_id);

	storage_count = (payload_size/sizeof(struct item));
	data->count += storage_count;

	VECTOR_ENSURE(sd->storage.item, data->count, 1);

	sd->storage.aggregate = data->count; // Total items in storage.

	for (i = 0; i < storage_count; i++) {
		const struct item *it = RFIFOP(fd, 12 + i * sizeof(struct item));
		VECTOR_PUSH(sd->storage.item, *it);
	}
	data->current_part += 1;

	if (data->current_part != data->total_parts) {
		hookStop();
		return;
	}

	sd->storage.received = true; // Mark the storage state as received.
	sd->storage.save = false; // Initialize the save flag as false.

	pc->checkitem(sd); // re-check remaining items.

	removeFromSession(sockt->session[fd], 1);
	hookStop();
}

/**
 * Parses an account storage save request from the map server.
 * @packet 0x3011 [in] <packet_len>.W <account_id>.L <struct item[]>.P
 * @param  fd     [in] file/socket descriptor.
 * @return 1 on success, 0 on failure.
 */
int mapif_parse_AccountStorageSave_pre(int *fd_)
{
	int fd = *fd_;
	int payload_size = RFIFOW(fd, 2) - 12, account_id = RFIFOL(fd, 4);
	int total_parts = RFIFOW(fd, 10);
	//int parts = RFIFOW(fd, 8);
	int i = 0, count = 0;
	struct storage_plugin_data *data;

	if (!storage_expansion)
		return 0;

	Assert_ret(fd > 0);
	Assert_ret(account_id > 0);

	if ((data = getFromSession(sockt->session[fd],0)) == NULL) {
		CREATE(data,struct storage_plugin_data,1);
		addToSession(sockt->session[fd],data,0,true);
		memset(&data->p_stor, 0, sizeof(data->p_stor));
		data->total_parts = total_parts;
		data->current_part = 0;
		data->count = 0;
		VECTOR_INIT(data->p_stor.item);
	}

	if (data->total_parts == 0) {
		ShowError("mapif_storage_plugin: Invalid Total Parts.\n");
		hookStop();
		return 0;	
	}


	count = payload_size/sizeof(struct item);
	data->count += count;

	if (count > 0) {
		VECTOR_ENSURE(data->p_stor.item, data->count, 1);

		for (i = 0; i < count; i++) {
			const struct item *it = RFIFOP(fd, 12 + i * sizeof(struct item));

			VECTOR_PUSH(data->p_stor.item, *it);
		}

		data->p_stor.aggregate += count;
		//ShowInfo("Recieved (SR) Part: %d/%d Count: %d/%d Size: %d\n", data->current_part, data->total_parts, count, data->count, payload_size);
		data->current_part += 1;
	}

	if (data->current_part != data->total_parts) {
		hookStop();
		return 1;
	}

	inter_storage->tosql(account_id, &data->p_stor);

	VECTOR_CLEAR(data->p_stor.item);

	mapif->sAccountStorageSaveAck(fd, account_id, true);
	removeFromSession(sockt->session[fd], 0);
	hookStop();

	return 1;
}

/**
 * Loads the account storage and send to the map server.
 * @packet 0x3805     [out] <account_id>.L <struct item[]>.P
 * @param  fd         [in]  file/socket descriptor.
 * @param  account_id [in]  account id of the session.
 * @return 1 on success, 0 on failure.
 */
int mapif_account_storage_load_pre(int *fd_, int *account_id_)
{
	int fd = *fd_, account_id = *account_id_;
	struct storage_data stor = { 0 };
	int count = 0, i = 0, len = 0;
	int len_t = 0, j = 0, c = 0;
	int offset = 12;
	int parts = 0;

	Assert_ret(account_id > 0);

	if (!storage_expansion)
		return 0;

	VECTOR_INIT(stor.item);
	count = inter_storage->fromsql(account_id, &stor);

	len = count * sizeof(struct item);
	if (len == 0)
		len = 1;
	parts = ceil(len/65535.0f);

	for (j = 0; j < parts; j++) {
		len_t = 12;
		for (c = 0; i < count; i++) {
			if (len_t + sizeof(struct item) > 65535)
				break;
			memcpy(WFIFOP(fd, offset + c * sizeof(struct item)), &VECTOR_INDEX(stor.item, i), sizeof(struct item));
			len_t += sizeof(struct item);
			c++;
		}
		WFIFOHEAD(fd, len_t);

		WFIFOW(fd, 0) = 0x3805;
		WFIFOW(fd, 2) = (uint16) len_t;
		WFIFOL(fd, 4) = account_id;
		WFIFOW(fd, 8) = j;	// Current Part
		WFIFOW(fd, 10) = parts;	// Total Parts

		WFIFOSET(fd, len_t);
		//ShowInfo("Sent %d Items(Storage) Part: %d/%d. Length: %d/%d, Items: %d/%d\n", i, j, parts, len_t, len, c, i);
	}


	VECTOR_CLEAR(stor.item);

	hookStop();
	return 1;
}

HPExport void plugin_init(void)
{
	if (SERVER_TYPE == SERVER_TYPE_MAP) {
		addHookPre(intif, send_account_storage, intif_send_account_storage_pre);
		addHookPre(intif, pAccountStorage, intif_parse_account_storage_pre);
	} else {
		addHookPre(mapif, account_storage_load, mapif_account_storage_load_pre);
		addHookPre(mapif, pAccountStorageSave, mapif_parse_AccountStorageSave_pre);
	}
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Dastgir/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}
