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

	ctx->nl_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (ctx->nl_cb == NULL) {
		fprintf(stderr,  "nl80211: Failed to allocate netlink callbacks");
		return NULL;
	}

	ctx->nl = nl_create_handle(ctx->nl_cb, "nl");
	if (ctx->nl == NULL)
		goto err;

	ctx->nl80211_id = genl_ctrl_resolve(ctx->nl, "nl80211");
	if (ctx->nl80211_id < 0) {
		fprintf(stderr,  "nl80211: 'nl80211' generic netlink not found");
		goto err;
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
	nl_destroy_handles(&ctx->nl_event);
	nl_destroy_handles(&ctx->nl);
	nl_cb_put(ctx->nl_cb);
	ctx->nl_cb = NULL;
	return NULL;
}

void driver_nl80211_deinit(struct nl80211_data *ctx)
{
	if (ctx == NULL)
		return;
	
	nl_destroy_handles(&ctx->nl_event);
	nl_destroy_handles(&ctx->nl);	
	nl_cb_put(ctx->nl_cb);
	ctx->nl_cb = NULL;
}





/* nl80211 code */
int ack_handler(struct nl_msg *msg, void *arg)
{
	int *err = arg;
	*err = 0;
	return NL_STOP;
}

int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
	void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_SKIP;
}

void * nl80211_cmd(struct nl80211_data *ctx, struct nl_msg *msg, int flags, uint8_t cmd) {
	return genlmsg_put(msg, 0, 0, ctx->nl80211_id, 0, flags, cmd, 0);
}


void nl80211_nlmsg_clear(struct nl_msg *msg)
{
	/*
	* Clear nlmsg data, e.g., to make sure key material is not left in
	* heap memory for unnecessarily long time.
	*/
	if (msg) {
		struct nlmsghdr *hdr = nlmsg_hdr(msg);
		void *data = nlmsg_data(hdr);
		/*
		* This would use nlmsg_datalen() or the older nlmsg_len() if
		* only libnl were to maintain a stable API.. Neither will work
		* with all released versions, so just calculate the length
		* here.
		*/
		int len = hdr->nlmsg_len - NLMSG_HDRLEN;

		memset(data, 0, len);
	}
}

int send_and_recv(struct nl80211_data *ctx, struct nl_handle *nl_handle, struct nl_msg *msg,
	int(*valid_handler)(struct nl_msg *, void *), void *valid_data)
{
	struct nl_cb *cb;
	int err = -ENOMEM;

	if (!msg)
		return -ENOMEM;

	cb = nl_cb_clone(ctx->nl_cb);
	if (!cb)
		goto out;

	err = nl_send_auto_complete(nl_handle, msg);
	if (err < 0)
		goto out;

	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

	if (valid_handler)
		nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, valid_data);

	while (err > 0) {
		int res = nl_recvmsgs(nl_handle, cb);
		if (res < 0) {
			fprintf(stderr, "nl80211: %s->nl_recvmsgs failed: %d\n", __func__, res);
		}
	}

out:
	nl_cb_put(cb);
	if (!valid_handler && valid_data == (void *)-1) {
		nl80211_nlmsg_clear(msg);
	}
	nlmsg_free(msg);
	return err;
}


int send_and_recv_msgs(struct nl80211_data *ctx, struct nl_msg *msg, int(*valid_handler)(struct nl_msg *, void *), void *valid_data) {
	return send_and_recv(ctx, ctx->nl, msg, valid_handler, valid_data);
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


