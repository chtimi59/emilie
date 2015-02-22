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

#if 0

static int protocol_feature_handler(struct nl_msg *msg, void *arg)
{
	u32 *feat = arg;
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		genlmsg_attrlen(gnlh, 0), NULL);

	if (tb_msg[NL80211_ATTR_PROTOCOL_FEATURES])
		*feat = nla_get_u32(tb_msg[NL80211_ATTR_PROTOCOL_FEATURES]);

	return NL_SKIP;
}


static u32 get_nl80211_protocol_features(struct nl80211_data *drv)
{
	u32 feat = 0;
	struct nl_msg *msg;

	msg = nlmsg_alloc();
	if (!msg)
		return 0;

	if (!nl80211_cmd(drv, msg, 0, NL80211_CMD_GET_PROTOCOL_FEATURES)) {
		nlmsg_free(msg);
		return 0;
	}

	if (send_and_recv_msgs(drv, msg, protocol_feature_handler, &feat) == 0)
		return feat;

	return 0;
}

 
static int wpa_driver_nl80211_get_info(struct nl80211_data *drv, struct wiphy_info_data *info) {
	return 0;
}


genlmsg_put(msg, 0, 0, nl80211_family, 0, 0, NL80211_CMD_SET_WIPHY, 0);
/* Add data extra attributes in message */
NLA_PUT_U32(msg, NL80211_ATTR_WIPHY, phyidx);  		// NL80211_ATTR_WIPHY =  phyidx
NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_RTS_THRESHOLD, 85); // NL80211_ATTR_WIPHY_RTS_THRESHOLD = 82
#endif