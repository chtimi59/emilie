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

#include "driver_nl80211_interface.h"


struct wiphy_idx_data {
	int wiphy_idx;
	enum nl80211_iftype nlmode;
	u8 *macaddr;
};


int netdev_info_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct wiphy_idx_data *info = arg;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		genlmsg_attrlen(gnlh, 0), NULL);

	if (tb[NL80211_ATTR_WIPHY])
		info->wiphy_idx = nla_get_u32(tb[NL80211_ATTR_WIPHY]);

	if (tb[NL80211_ATTR_IFTYPE])
		info->nlmode = nla_get_u32(tb[NL80211_ATTR_IFTYPE]);


	if (info->macaddr && tb[NL80211_ATTR_MAC]) {
		// seems to not working :(
		fprintf(stderr, "mac ok\n");
		memcpy(info->macaddr, nla_data(tb[NL80211_ATTR_MAC]), ETH_ALEN);
	}
	
	return NL_SKIP;
}


int nl80211_get_wiphy_index(struct nl80211_data *ctx, int* phyidx)
{
	struct nl_msg *msg;
	struct wiphy_idx_data data = { 0 };
	if (!(msg = nl80211_cmd_msg(ctx, 0, NL80211_CMD_GET_INTERFACE)))
		return -1;
	if (send_and_recv_msgs(ctx, msg, netdev_info_handler, &data))
		return -1;

	if (phyidx) *phyidx = data.wiphy_idx;
	return 0;
}


int nl80211_get_ifmode(struct nl80211_data *ctx, enum nl80211_iftype* nlmode)
{
	struct nl_msg *msg;
	struct wiphy_idx_data data = {
		.nlmode = NL80211_IFTYPE_UNSPECIFIED,
		.macaddr = NULL,
	};

	if (!(msg = nl80211_cmd_msg(ctx, 0, NL80211_CMD_GET_INTERFACE)))
		return NL80211_IFTYPE_UNSPECIFIED;
	if (send_and_recv_msgs(ctx, msg, netdev_info_handler, &data))
		return -1;
	
	if (nlmode) *nlmode = data.nlmode;
	return 0;
}


int nl80211_set_ifmode(struct nl80211_data* ctx, enum nl80211_iftype mode)
{
	struct nl_msg *msg;
	
	msg = nl80211_cmd_msg(ctx, 0, NL80211_CMD_SET_INTERFACE);
	if (!msg) {
		nlmsg_free(msg);
		return -1;
	}

	if (nla_put_u32(msg, NL80211_ATTR_IFTYPE, mode)) {
		nlmsg_free(msg);
		return -1;
	}

	if (send_and_recv_msgs(ctx, msg, NULL, NULL)) {
		fprintf(stderr, "failed?\n");
		return -1;
	}

	return 0;
}


#if 0
int nl80211_get_macaddr(struct nl80211_data *ctx, u8* macaddr)
{
	if (!macaddr) return -1;

	struct nl_msg *msg;
	struct wiphy_idx_data data = { 0 };
	data.macaddr = macaddr;

	if (!(msg = nl80211_cmd_msg(ctx, 0, NL80211_CMD_GET_INTERFACE))) {
		free(data.macaddr);
		return -1;
	}

	if (send_and_recv_msgs(ctx, msg, netdev_info_handler, &data)) {
		free(data.macaddr);
		return -1;
	}

	return 0;
}
#endif

