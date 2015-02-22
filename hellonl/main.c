#include "common.h"

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include "driver_nl80211.h"
#include "netlink.h"
#include "nl80211.h"

#include "rfkill.h"
#include "netlink.h"
#include "driver_nl80211.h"
#include "ioctl.h"



#if 0


// nl callback
static int ack_handler(struct nl_msg *msg, void *arg) {
	*((int *)arg) = 0; // pass no error code !
	return NL_STOP;
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg) {
	fprintf(stderr,"NL : Message [%d, from %d, type %d] meets error:%d\n",
		err->msg.nlmsg_seq,
		err->msg.nlmsg_pid,
		err->msg.nlmsg_type,
		err->error);

	*((int *)arg) = err->error; // pass error code
	return NL_STOP;
}

// 80211 callback
int process_bss_event(struct nl_msg *msg, void *arg)
{
	return NL_SKIP;
}

#endif

void set_rts_threshold(struct netlink_data *drv, int rts)
{
	/* wireless driver id */
	int phyidx = 0; // see here /sys/class/ieee80211/%s/index

	/**
	Documentation Overview - libnl Suite
	http://www.infradead.org/~tgr/libnl/doc/
	*/

	/* CREATE A NETLINK MESSAGE */
	struct nl_msg *msg;

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "set_rts_threshold: err 0");
		return;
	}

	if (!nl80211_cmd(drv, msg, 0, NL80211_CMD_SET_WIPHY)) {
		fprintf(stderr, "set_rts_threshold: err 1");
		nlmsg_free(msg);
		return;
	}

	if (nla_put_u32(msg, NL80211_ATTR_WIPHY, phyidx)) {
		fprintf(stderr, "set_rts_threshold: err 2");
		nlmsg_free(msg);
		return;
	}

	if (nla_put_u32(msg, NL80211_ATTR_WIPHY_RTS_THRESHOLD, rts)) {
		fprintf(stderr, "set_rts_threshold: err 3");
		nlmsg_free(msg);
		return;
	}

	if (send_and_recv_msgs(drv, msg, NULL, NULL)) {
		fprintf(stderr, "set_rts_threshold: err 4");
		return;
	}

	return;
}

#if 0
struct nl_msg* set_rts_threshold(int nl80211_family, int rts)
{
	/* wireless driver id */
	int phyidx = 0; // see here /sys/class/ieee80211/%s/index


	/**
	Documentation Overview - libnl Suite
	http://www.infradead.org/~tgr/libnl/doc/
	*/

	/* CREATE A NETLINK MESSAGE */
	struct nl_msg *msg = nlmsg_alloc();

	/* 
	msg	    : Netlink message object
	port    : Netlink port or NL_AUTO_PORT (0)
	seq     : Sequence number of message or NL_AUTO_SEQ (0)
	family  : Numeric family identifier (nl80211)
	hdrlen  : Length of user header = 0
	flags   : Additional Netlink message flags (optional) = 0
	cmd     : Numeric command identifier = NL80211_CMD_SET_WIPHY
	version : Interface version = 0
	*/
	genlmsg_put(msg, 0, 0, nl80211_family, 0, 0, NL80211_CMD_SET_WIPHY, 0);

	/* Add data extra attributes in message */
	nla_put_u32(msg, NL80211_ATTR_WIPHY, phyidx);  		     // NL80211_ATTR_WIPHY =  phyidx
	nla_put_u32(msg, NL80211_ATTR_WIPHY_RTS_THRESHOLD, rts); // NL80211_ATTR_WIPHY_RTS_THRESHOLD = 82
	return msg;
}
#endif


void OnProperClose(int sig, void *signal_ctx) {
	fprintf(stderr, "CTRL-C hit !\n");
	eloop_terminate();
}

void OnTime(void *eloop_data, void *user_ctx) {
	fprintf(stderr, "BEEP !\n");
	eloop_register_timeout(1, 0, &OnTime, NULL, NULL);
}


#define EXIT_ERROR(_str_) do { err=-1; fprintf(stderr, _str_"\n"); goto exit_label; } while(0)

int main(int argc, char **argv)
{
	int err = 0;
	//struct nl_sock *nlsock = NULL;
	//struct nl_cb   *nlcb = nl_cb_alloc(NL_CB_DEFAULT);
	//struct nl_msg  *nlmsg = NULL;

	struct rfkill_config  rfkill_cfg = {0};
	struct rfkill_data    *rfkill_data = NULL;

	struct netlink_config netlink_cfg = {0};
	struct netlink_data   *netlink_data = NULL;

	struct nl80211_config  nl80211_cfg = {0};
	struct nl80211_data    *nl80211_data = NULL;

	struct i802_bss		   bss = {0};
	int ioctl_socket;

	// init squeduler
	eloop_init();

	// catch CTRL-C signal
	eloop_register_signal_terminate(&OnProperClose, NULL);

	// timer test
	eloop_register_timeout(1, 0, &OnTime, NULL, NULL);
	
#if 0
	/** CREATE NETLINK SOCKET 
	 * Netlink socket family is a Linux kernel interface used for inter-process communication (IPC) 
	 * between the kernel and userspace processes,
	 */
	netlink_data = netlink_init(&netlink_cfg);
	if (netlink_data == NULL)
		EXIT_ERROR("netlink: init failed");

	/** CREATE A BASIC SOCKET FORT IOCTL */
	ioctl_socket = linux_create_socket();
	if (ioctl_socket < 0)
		EXIT_ERROR("ioctl socket: init failed");

	/* ATTACH NL80211 family */
	nl80211_cfg.ioctl_sock = ioctl_socket;
	nl80211_cfg.nl = netlink_data;
	nl80211_data = driver_nl80211_init(&nl80211_cfg);
	if (nl80211_data == NULL)
		EXIT_ERROR("nl80211: init failed");

	// Init BSS
	/*if (nl80211_init_bss(&bss)) 
		EXIT_ERROR("nl80211: init bss failed");*/

	// RF-KILL (usefull ?)
	/*rfkill_data = rfkill_init(&rfkill_cfg);
	if (rfkill_data == NULL) {
		if (rfkill_data) free(rfkill_data);
		EXIT_ERROR("nl80211: RFKILL status not available");
	}*/

	// INTERFACE IS UP ?
	if (linux_iface_up(ioctl_socket, "wlan0")<=0)
		EXIT_ERROR("wlan0: is down\n");

	// TEST
	//set_rts_threshold(nl80211_data, 82);
	
#endif

	// -------- SCHEDULER -----------
	eloop_run();
	// ------------------------------

	fprintf(stderr, "Closing...\n");

#if 0
	/* init netlink socket */
	sock = nl_socket_alloc();
	if (!sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return 1;
	}

	/* connect to socket */
	nl_socket_set_buffer_size(sock, 8192, 8192);
	if (genl_connect(sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		nl_socket_free(sock);
		return 1;
	}

	/* search nl80211 family id */
	int nl80211_family = genl_ctrl_resolve(sock, "nl80211");
	if (nl80211_family < 0) {
		fprintf(stderr, "nl80211 not found.\n");
		nl_socket_free(sock);
		return 1;
	} else {
		fprintf(stderr, "nl80211 found at ID%i\n",nl80211_family);
	}

	/* ?? */
	/*struct nl_cb *socket_cb = nl_cb_alloc(NL_CB_DEFAULT);
	nl_socket_set_cb(sock, socket_cb);*/



	/* wireless driver id */
	int phyidx = 0; // see here /sys/class/ieee80211/%s/index

	
	/**
	 Documentation Overview - libnl Suite
	 http://www.infradead.org/~tgr/libnl/doc/
	*/

	/* 1. CREATE A NETLINK MESSAGE */
	struct nl_msg *msg = nlmsg_alloc();

	/* msg	   : Netlink message object
	   port	   : Netlink port or NL_AUTO_PORT (0)
	   seq     : Sequence number of message or NL_AUTO_SEQ (0)
	   family  : Numeric family identifier (nl80211)
	   hdrlen  : Length of user header = 0
	   flags   : Additional Netlink message flags (optional) = 0
	   cmd     : Numeric command identifier = NL80211_CMD_SET_WIPHY
	   version : Interface version = 0
	*/
	genlmsg_put(msg, 0, 0, nl80211_family, 0, 0, NL80211_CMD_SET_WIPHY, 0);
	/* Add data extra attributes in message */
	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY, phyidx);  		// NL80211_ATTR_WIPHY =  phyidx
	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_RTS_THRESHOLD, 85); // NL80211_ATTR_WIPHY_RTS_THRESHOLD = 82
	

	// 2. SEND THE MESSAGE
	nl_send_auto_complete(sock, msg);


	// 3. WAIT FOR ACK (based on callback)
	err = 1;	
	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	while (err > 0) {
		nl_recvmsgs(sock, cb);
	}
#endif	



	
exit_label:
	fprintf(stderr, (err<0) ? "failed\n" : "success\n");

	//rfkill_deinit(rfkill_data);
	//nl80211_destroy_bss(&bss);
	/*driver_nl80211_deinit(nl80211_data);
	netlink_deinit(netlink_data);*/
	
	
	/*
	nl_cb_put(nlcb);
	nlmsg_free(nlmsg);
	nl_socket_free(nlsock);
	

	nl_cb_put(nlcb);
	if (netlink_cfg) free(netlink_cfg);
	netlink_deinit(netlink_cfg);
	if (netlink_data == NULL) {
		goto failure;


	nlmsg_free(nlmsg);
	nl_socket_free(nlsock);
	*/
	fprintf(stderr, "bye\n");
	return err;
}

