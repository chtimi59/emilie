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

#include "driver_nl80211_freq.h"


int nl80211_set_channel(struct nl80211_data *ctx, int freq)
{
	struct nl_msg *msg;
	int ret;

	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	if (!nl80211_cmd(ctx, msg, 0, NL80211_CMD_SET_CHANNEL)) {
		nlmsg_free(msg);
		return -1;
	}

	if(nla_put_u32(msg, NL80211_ATTR_IFINDEX, ctx->ifindex)) {
		nlmsg_free(msg);
		return -1;
	}

	if (nla_put_u32(msg, NL80211_ATTR_WIPHY_FREQ, freq)) {
		nlmsg_free(msg);
		return -1;
	}

	ret = send_and_recv_msgs(ctx, msg, NULL, NULL);
	if (ret != 0) {
		fprintf(stderr, "nl80211: Failed to set channel: %d (%s)\n", ret, strerror(-ret));
		return -1;
	}
	return 0;
}

