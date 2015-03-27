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

#include "driver_nl80211_rts.h"

static int wiphy_info_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	int *rts = arg;	           
	if (tb_msg[NL80211_ATTR_WIPHY_RTS_THRESHOLD]) {}
	*rts = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_RTS_THRESHOLD]);

	return NL_SKIP;
}


int get_nl80211_rts_threshold(struct nl80211_data *ctx, int* rts)
{
	/* wireless driver id */
	//int phyidx = 0; // see here /sys/class/ieee80211/%s/index
	int phyidx = ctx->phyindex;

	/**
	Documentation Overview - libnl Suite
	http://www.infradead.org/~tgr/libnl/doc/
	*/

	/* CREATE A NETLINK MESSAGE */
	struct nl_msg *msg;

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "get_rts_threshold: memory error\n");
		return -1;
	}

	if (!nl80211_cmd(ctx, msg, 0, NL80211_CMD_GET_WIPHY)) {
		fprintf(stderr, "get_rts_threshold: command error \n");
		nlmsg_free(msg);
		return -1;
	}

	if (nla_put_u32(msg, NL80211_ATTR_WIPHY, phyidx)) {
		fprintf(stderr, "get_rts_threshold: command attribute error\n");
		nlmsg_free(msg);
		return -1;
	}

	if (send_and_recv_msgs(ctx, msg, wiphy_info_handler, rts)) {
		fprintf(stderr, "get_rts_threshold: send error (privilege?)\n");
		return -1;
	}

	return 0;
}




int set_nl80211_rts_threshold(struct nl80211_data *ctx, int rts)
{
	/* wireless driver id */
	//int phyidx = 0; // see here /sys/class/ieee80211/%s/index
	int phyidx = ctx->phyindex;


	u32 val;

	if (rts >= 2347)
		val = (u32)-1;
	else
		val = rts;


	/**
	Documentation Overview - libnl Suite
	http://www.infradead.org/~tgr/libnl/doc/
	*/

	/* CREATE A NETLINK MESSAGE */
	struct nl_msg *msg;

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "set_rts_threshold: memory error\n");
		return -1;
	}

	if (!nl80211_cmd(ctx, msg, 0, NL80211_CMD_SET_WIPHY)) {
		fprintf(stderr, "set_rts_threshold: command error \n");
		nlmsg_free(msg);
		return -1;
	}

	if (nla_put_u32(msg, NL80211_ATTR_WIPHY, phyidx)) {
		fprintf(stderr, "set_rts_threshold: command attribute error\n");
		nlmsg_free(msg);
		return -1;
	}

	if (nla_put_u32(msg, NL80211_ATTR_WIPHY_RTS_THRESHOLD, val)) {
		fprintf(stderr, "set_rts_threshold: command attribute error\n");
		nlmsg_free(msg);
		return -1;
	}
	
	if (send_and_recv_msgs(ctx, msg, NULL, NULL)) {
		fprintf(stderr, "set_rts_threshold: send error (privilege?)\n");
		return -1;
	}

	return 0;
}

