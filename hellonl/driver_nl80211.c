#include "common.h"
#include "eloop.h"
#include <net/if.h>
#include <fcntl.h>

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include "driver_nl80211.h"
#include "netlink.h"
#include "nl80211.h"

#include "driver_helper.cxx"



// 80211 callback
static int no_seq_check(struct nl_msg *msg, void *arg) {
	return NL_OK;
}

// 80211 callback
static int process_global_event(struct nl_msg *msg, void *arg) {
	return NL_SKIP;
}

// 80211 callback
static int process_bss_event(struct nl_msg *msg, void *arg) {
	return NL_SKIP;
}


// eloop squeduler event
void wpa_driver_nl80211_event_receive(int sock, void *eloop_ctx, void *handle) {
	printf("*\n");
}



struct nl80211_data * driver_nl80211_init(struct nl80211_config *cfg)
{
	struct nl80211_data *ctx;

	ctx = zalloc(sizeof(*ctx));
	if (ctx == NULL)
		return NULL;
	
	ctx->cfg = cfg;

	ctx->ifindex  = if_nametoindex(cfg->ifname);
	fprintf(stderr, "nl80211: '%s' index: %d\n", cfg->ifname, ctx->ifindex);

	ctx->nl_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (ctx->nl_cb == NULL) {
		fprintf(stderr,  "nl80211: Failed to allocate netlink callbacks\n");
		return NULL;
	}

	ctx->nl = nl_create_handle(ctx->nl_cb, "nl");
	if (ctx->nl == NULL)
		goto err;
	
	ctx->nl80211_id = genl_ctrl_resolve(ctx->nl, "nl80211");
	if (ctx->nl80211_id < 0) {
		fprintf(stderr,  "nl80211: 'nl80211' generic netlink not found\n");
		goto err;
	} else {
		fprintf(stderr, "nl80211: 'nl80211' familly found at %d\n", ctx->nl80211_id);
	}

	ctx->nl_event = nl_create_handle(ctx->nl_cb, "event");
	if (ctx->nl_event == NULL)
		goto err;
	
	
	/*ret = nl_get_multicast_id(global, "nl80211", "mlme");
	if (ret >= 0)
		ret = nl_socket_add_membership(global->nl_event, ret);
	if (ret < 0) {
		fprintf(stderr,  "nl80211: Could not add multicast "
			"membership for mlme events: %d (%s)",
			ret, strerror(-ret));
		goto err;
	}*/

	
	nl_cb_set(ctx->nl_cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	nl_cb_set(ctx->nl_cb, NL_CB_VALID, NL_CB_CUSTOM, process_global_event, ctx);
	
	nl80211_register_eloop_read(&ctx->nl_event, wpa_driver_nl80211_event_receive, ctx->nl_cb);

	return ctx;

err:
	if (ctx->nl_event) nl_destroy_handles(&ctx->nl_event);
	if (ctx->nl) nl_destroy_handles(&ctx->nl);
	nl_cb_put(ctx->nl_cb);
	ctx->nl_cb = NULL;
	return NULL;
}

void driver_nl80211_deinit(struct nl80211_data *ctx)
{
	if (ctx == NULL)
		return;
	
	if (ctx->nl_event) nl_destroy_handles(&ctx->nl_event);
	if (ctx->nl) nl_destroy_handles(&ctx->nl);
	nl_cb_put(ctx->nl_cb);
	ctx->nl_cb = NULL;
}














int nl80211_init_bss(struct i802_bss *bss) {
	bss->nl_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!bss->nl_cb)
		return -1;

	nl_cb_set(bss->nl_cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	nl_cb_set(bss->nl_cb, NL_CB_VALID, NL_CB_CUSTOM, process_bss_event, bss);
	return 0;
}


void nl80211_destroy_bss(struct i802_bss *bss) {  
	nl_cb_put(bss->nl_cb);
	bss->nl_cb = NULL;
}


