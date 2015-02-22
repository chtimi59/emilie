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




void * nl80211_cmd(struct nl80211_data *ctx, struct nl_msg *msg, int flags, uint8_t cmd) {
	return genlmsg_put(msg, 0, 0, ctx->nl80211_id, 0, flags, cmd, 0);
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
