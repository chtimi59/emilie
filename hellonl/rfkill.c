/*
 * Linux rfkill helper functions for driver wrappers
 * Copyright (c) 2010, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

//#include "includes.h"
#include <fcntl.h>

#include "common.h"
#include "rfkill.h"
#include "eloop.h"

#define RFKILL_EVENT_SIZE_V1 8

struct rfkill_event {
	u32 idx;
	u8 type;
	u8 op;
	u8 soft;
	u8 hard;
} STRUCT_PACKED;

enum rfkill_operation {
	RFKILL_OP_ADD = 0,
	RFKILL_OP_DEL,
	RFKILL_OP_CHANGE,
	RFKILL_OP_CHANGE_ALL,
};

enum rfkill_type {
	RFKILL_TYPE_ALL = 0,
	RFKILL_TYPE_WLAN,
	RFKILL_TYPE_BLUETOOTH,
	RFKILL_TYPE_UWB,
	RFKILL_TYPE_WIMAX,
	RFKILL_TYPE_WWAN,
	RFKILL_TYPE_GPS,
	RFKILL_TYPE_FM,
	NUM_RFKILL_TYPES,
};


static void rfkill_receive(int sock, void *eloop_ctx, void *sock_ctx)
{
	struct rfkill_data *rfkill = eloop_ctx;
	struct rfkill_event event;
	ssize_t len;
	int new_blocked;

	len = read(rfkill->fd, &event, sizeof(event));
	if (len < 0) {
		fprintf(stderr, "rfkill: Event read failed: %s\n", strerror(errno));
		return;
	}

	if (len != RFKILL_EVENT_SIZE_V1) {
		fprintf(stderr, "rfkill: Unexpected event size %d (expected %d)\n",
			   (int) len, RFKILL_EVENT_SIZE_V1);
		return;
	}


	fprintf(stderr, "rfkill: event: idx=%u type=%d op=%u soft=%u hard=%u\n",
		   event.idx, event.type, event.op, event.soft,
		   event.hard);
	if (event.op != RFKILL_OP_CHANGE || event.type != RFKILL_TYPE_WLAN)
		return;

	if (event.hard) {
		fprintf(stderr, "rfkill: WLAN hard blocked\n");
		new_blocked = 1;
	} else if (event.soft) {
		fprintf(stderr, "rfkill: WLAN soft blocked\n");
		new_blocked = 1;
	} else {
		fprintf(stderr, "rfkill: WLAN unblocked\n");
		new_blocked = 0;
	}

	if (new_blocked != rfkill->blocked) {
		rfkill->blocked = new_blocked;
		if (new_blocked) {
			if (rfkill->cfg->blocked_cb) rfkill->cfg->blocked_cb(rfkill->cfg->ctx);
		} else {
			if (rfkill->cfg->unblocked_cb) rfkill->cfg->unblocked_cb(rfkill->cfg->ctx);
		}
	}
}


struct rfkill_data * rfkill_init(struct rfkill_config *cfg)
{
	struct rfkill_data *rfkill;
	struct rfkill_event event;
	ssize_t len;

	rfkill = zalloc(sizeof(*rfkill));
	if (rfkill == NULL)
		return NULL;

	rfkill->cfg = cfg;
	rfkill->fd = open("/dev/rfkill", O_RDONLY);
	if (rfkill->fd < 0) {
		fprintf(stderr, "rfkill: Cannot open RFKILL control device \n");
		goto fail;
	}

	if (fcntl(rfkill->fd, F_SETFL, O_NONBLOCK) < 0) {
		fprintf(stderr, "rfkill: Cannot set non-blocking mode: %s\n", strerror(errno));
		goto fail2;
	}

	for (;;) {
		len = read(rfkill->fd, &event, sizeof(event));
		if (len < 0) {
			if (errno == EAGAIN)
				break; /* No more entries */
			fprintf(stderr, "rfkill: Event read failed: %s\n", strerror(errno));
			break;
		}
		if (len != RFKILL_EVENT_SIZE_V1) {
			fprintf(stderr, "rfkill: Unexpected event size %d (expected %d)\n",
				   (int) len, RFKILL_EVENT_SIZE_V1);
			continue;
		}
		fprintf(stderr, "rfkill: initial event: idx=%u type=%d op=%u soft=%u hard=%u" "\n",
			   event.idx, event.type, event.op, event.soft,
			   event.hard);

		if (event.op != RFKILL_OP_ADD ||
		    event.type != RFKILL_TYPE_WLAN)
			continue;

		if (event.hard) {
			fprintf(stderr, "rfkill: WLAN hard blocked" "\n");
			rfkill->blocked = 1;
		} else if (event.soft) {
			fprintf(stderr, "rfkill: WLAN soft blocked" "\n");
			rfkill->blocked = 1;
		}
	}

	eloop_register_read_sock(rfkill->fd, rfkill_receive, rfkill, NULL);

	return rfkill;

fail2:
	close(rfkill->fd);
fail:
	free(rfkill);
	return NULL;
}


void rfkill_deinit(struct rfkill_data *rfkill)
{
	if (rfkill == NULL)
		return;

	if (rfkill->fd >= 0) {
		eloop_unregister_read_sock(rfkill->fd);
		close(rfkill->fd);
	}

	free(rfkill);
}


int rfkill_is_blocked(struct rfkill_data *rfkill)
{
	if (rfkill == NULL)
		return 0;

	return rfkill->blocked;
}
