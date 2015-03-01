#include "common.h"
#include "eloop.h"
#include <net/if.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netpacket/packet.h>
#include <linux/filter.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <arpa/inet.h>

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include "driver_nl80211.h"
#include "netlink.h"
#include "nl80211.h"

#include "ioctl.h"
#include "driver_nl80211_monitor.h"


static void handle_monitor_read(int sock, void *eloop_ctx, void *sock_ctx)
{

}

static int nl80211_create_iface_once(struct nl80211_data* ctx, int(*handler)(struct nl_msg *, void *), void *arg)
{
	int ret = -ENOBUFS;
	struct nl_msg *msg;
	struct nlattr *flags;
	int ifidx;
	const char *ifname = ctx->monitor_name;
	fprintf(stderr, "nl80211: Create monitor interface %s\n", ifname);

	msg = nl80211_cmd_msg(ctx, 0, NL80211_CMD_NEW_INTERFACE);
	if (!msg)
		goto fail;

	if (nla_put_string(msg, NL80211_ATTR_IFNAME, ifname))
		goto fail;
	
	if (nla_put_u32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR))
		goto fail;

	flags = nla_nest_start(msg, NL80211_ATTR_MNTR_FLAGS);
	if (!flags)
		goto fail;

	if (nla_put_flag(msg, NL80211_MNTR_FLAG_COOK_FRAMES))
		goto fail;

	nla_nest_end(msg, flags);

	/*
	* Tell cfg80211 that the interface belongs to the socket that created
	* it, and the interface should be deleted when the socket is closed.
	*/
	if (nla_put_flag(msg, NL80211_ATTR_IFACE_SOCKET_OWNER))
		goto fail;

	ret = send_and_recv_msgs(ctx, msg, handler, arg);
	msg = NULL;


	if (ret) {
	fail:
		nlmsg_free(msg);
		fprintf(stderr, "nl80211: Failed to create Monitor interface: %d (%s)\n", ret, strerror(-ret));
		return ret;
	}


	ifidx = if_nametoindex(ifname);
	fprintf(stderr, "nl80211: New interface %s created at ifindex=%d\n", ifname, ifidx);
	if (ifidx <= 0)
		return -1;

	return ifidx;
}

void nl80211_remove_iface(struct nl80211_data *ctx, int ifidx)
{
	struct nl_msg *msg = NULL;

	fprintf(stderr, "nl80211: Remove interface ifindex=%d\n", ifidx);

	msg = nlmsg_alloc();
	if (!msg)
		goto failed;

	if (!nl80211_cmd(ctx, msg, 0, NL80211_CMD_DEL_INTERFACE)) 
		goto failed;

	if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifidx))
		goto failed;

	if (send_and_recv_msgs(ctx, msg, NULL, NULL) == 0)
		return;

failed:
	if (msg) nlmsg_free(msg);
	fprintf(stderr, "failed to remove interface (ifidx=%d)\n", ifidx);
}





/*
* we post-process the filter code later and rewrite
* this to the offset to the last instruction
*/
#define PASS	0xFF
#define FAIL	0xFE

static struct sock_filter msock_filter_insns[] = {
	/*
	* do a little-endian load of the radiotap length field
	*/
	/* load lower byte into A */
	BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 2),
	/* put it into X (== index register) */
	BPF_STMT(BPF_MISC | BPF_TAX, 0),
	/* load upper byte into A */
	BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 3),
	/* left-shift it by 8 */
	BPF_STMT(BPF_ALU | BPF_LSH | BPF_K, 8),
	/* or with X */
	BPF_STMT(BPF_ALU | BPF_OR | BPF_X, 0),
	/* put result into X */
	BPF_STMT(BPF_MISC | BPF_TAX, 0),

	/*
	* Allow management frames through, this also gives us those
	* management frames that we sent ourselves with status
	*/
	/* load the lower byte of the IEEE 802.11 frame control field */
	BPF_STMT(BPF_LD | BPF_B | BPF_IND, 0),
	/* mask off frame type and version */
	BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0xF),
	/* accept frame if it's both 0, fall through otherwise */
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0, PASS, 0),

	/*
	* TODO: add a bit to radiotap RX flags that indicates
	* that the sending station is not associated, then
	* add a filter here that filters on our DA and that flag
	* to allow us to deauth frames to that bad station.
	*
	* For now allow all To DS data frames through.
	*/
	/* load the IEEE 802.11 frame control field */
	BPF_STMT(BPF_LD | BPF_H | BPF_IND, 0),
	/* mask off frame type, version and DS status */
	BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0x0F03),
	/* accept frame if version 0, type 2 and To DS, fall through otherwise
	*/
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x0801, PASS, 0),

#if 0
	/*
	* drop non-data frames
	*/
	/* load the lower byte of the frame control field */
	BPF_STMT(BPF_LD | BPF_B | BPF_IND, 0),
	/* mask off QoS bit */
	BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0x0c),
	/* drop non-data frames */
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 8, 0, FAIL),
#endif
	/* load the upper byte of the frame control field */
	BPF_STMT(BPF_LD | BPF_B | BPF_IND, 1),
	/* mask off toDS/fromDS */
	BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0x03),
	/* accept WDS frames */
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 3, PASS, 0),

	/*
	* add header length to index
	*/
	/* load the lower byte of the frame control field */
	BPF_STMT(BPF_LD | BPF_B | BPF_IND, 0),
	/* mask off QoS bit */
	BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0x80),
	/* right shift it by 6 to give 0 or 2 */
	BPF_STMT(BPF_ALU | BPF_RSH | BPF_K, 6),
	/* add data frame header length */
	BPF_STMT(BPF_ALU | BPF_ADD | BPF_K, 24),
	/* add index, was start of 802.11 header */
	BPF_STMT(BPF_ALU | BPF_ADD | BPF_X, 0),
	/* move to index, now start of LL header */
	BPF_STMT(BPF_MISC | BPF_TAX, 0),

	/*
	* Accept empty data frames, we use those for
	* polling activity.
	*/
	BPF_STMT(BPF_LD | BPF_W | BPF_LEN, 0),
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_X, 0, PASS, 0),

	/*
	* Accept EAPOL frames
	*/
	BPF_STMT(BPF_LD | BPF_W | BPF_IND, 0),
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0xAAAA0300, 0, FAIL),
	BPF_STMT(BPF_LD | BPF_W | BPF_IND, 4),
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x0000888E, PASS, FAIL),

	/* keep these last two statements or change the code below */
	/* return 0 == "DROP" */
	BPF_STMT(BPF_RET | BPF_K, 0),
	/* return ~0 == "keep all" */
	BPF_STMT(BPF_RET | BPF_K, ~0),
};

static struct sock_fprog msock_filter = {
	.len = ARRAY_SIZE(msock_filter_insns),
	.filter = msock_filter_insns,
};


static int add_monitor_filter(int s)
{
	int idx;

	/* rewrite all PASS/FAIL jump offsets */
	for (idx = 0; idx < msock_filter.len; idx++)
	{
		struct sock_filter *insn = &msock_filter_insns[idx];

		if (BPF_CLASS(insn->code) == BPF_JMP) {
			if (insn->code == (BPF_JMP | BPF_JA)) {
				if (insn->k == PASS)
					insn->k = msock_filter.len - idx - 2;
				else if (insn->k == FAIL)
					insn->k = msock_filter.len - idx - 3;
			}

			if (insn->jt == PASS)
				insn->jt = msock_filter.len - idx - 2;
			else if (insn->jt == FAIL)
				insn->jt = msock_filter.len - idx - 3;

			if (insn->jf == PASS)
				insn->jf = msock_filter.len - idx - 2;
			else if (insn->jf == FAIL)
				insn->jf = msock_filter.len - idx - 3;
		}
	}

	if (setsockopt(s, SOL_SOCKET, SO_ATTACH_FILTER,
		&msock_filter, sizeof(msock_filter))) {
		fprintf(stderr, "nl80211: setsockopt(SO_ATTACH_FILTER) failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int nl80211_create_monitor_interface(struct nl80211_data* ctx)
{
	memset(ctx->monitor_name, 0, IFNAMSIZ);
	snprintf(ctx->monitor_name, IFNAMSIZ, "mon.%s", ctx->ifname);
	ctx->monitor_ifidx = nl80211_create_iface_once(ctx, NULL, NULL);

	if (ctx->monitor_ifidx == -EOPNOTSUPP) {
		/*
		* This is backward compatibility for a few versions of
		* the kernel only that didn't advertise the right
		* attributes for the only driver that then supported
		* AP mode w/o monitor -- ath6kl.
		*/
		fprintf(stderr, "nl80211: Driver does not support monitor interface type - try to run without it\n");
	}

	if (ctx->monitor_ifidx < 0)
		return -1;

	// interface up!
	if (linux_set_iface_flags(ctx->cfg->ioctl_sock, ctx->monitor_name, 1))
		goto error;

	// create socket on monitor interface
	{
		struct sockaddr_ll ll;
		int optval;
		socklen_t optlen;

		memset(&ll, 0, sizeof(ll));
		ll.sll_family = AF_PACKET;
		ll.sll_ifindex = ctx->monitor_ifidx;
		ctx->monitor_sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

		if (ctx->monitor_sock < 0) {
			fprintf(stderr, "nl80211: socket[PF_PACKET,SOCK_RAW] failed: %s\n", strerror(errno));
			goto error;
		}

		if (add_monitor_filter(ctx->monitor_sock)) {
			fprintf(stderr, "Failed to set socket filter for monitor interface; do filtering in user space\n");
			/* This works, but will cost in performance. */
		}

		if (bind(ctx->monitor_sock, (struct sockaddr *) &ll, sizeof(ll)) < 0) {
			fprintf(stderr, "nl80211: monitor socket bind failed: %s\n", strerror(errno));
			goto error;
		}

		optlen = sizeof(optval);
		optval = 20;
		if (setsockopt(ctx->monitor_sock, SOL_SOCKET, SO_PRIORITY, &optval, optlen)) {
			fprintf(stderr, "nl80211: Failed to set socket priority: %s\n", strerror(errno));
			goto error;
		}

		if (eloop_register_read_sock(ctx->monitor_sock, handle_monitor_read, ctx, NULL)) {
			fprintf(stderr, "nl80211: Could not register monitor read socket\n");
			goto error;
		}
	}

	return 0;

error:
	nl80211_remove_monitor_interface(ctx);
	return -1;
}


void nl80211_remove_monitor_interface(struct nl80211_data* ctx)
{
	if (ctx->monitor_ifidx >= 0) {
		fprintf(stderr, "nl80211: Remove monitor interface\n");
		nl80211_remove_iface(ctx, ctx->monitor_ifidx);
		ctx->monitor_ifidx = -1;
	}

	if (ctx->monitor_sock >= 0) {
		eloop_unregister_read_sock(ctx->monitor_sock);
		close(ctx->monitor_sock);
		ctx->monitor_sock = -1;
	}
}