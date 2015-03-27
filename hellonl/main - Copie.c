#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <endian.h>


#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

/* libnl 1.x compatibility code */
#if !defined(CONFIG_LIBNL20) && !defined(CONFIG_LIBNL30)
# define nl_sock nl_handle
static inline struct nl_handle *nl_socket_alloc(void) { return nl_handle_alloc(); }
static inline void nl_socket_free(struct nl_sock *h) { nl_handle_destroy(h); }
static inline int nl_socket_set_buffer_size(struct nl_sock *sk,int rxbuf, int txbuf) { 
	return nl_set_buffer_size(sk, rxbuf, txbuf); }
#endif /* CONFIG_LIBNL20 && CONFIG_LIBNL30 */



#include "nl80211.h"


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

int main(int argc, char **argv)
{
	int err;
	struct nl_sock *sock;
	struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);	

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
	
	// release	
	nl_cb_put(cb);
	nlmsg_free(msg);
	nl_socket_free(sock);
	if (err>=0) {
		fprintf(stderr,"success\n");
	} else {
		fprintf(stderr,"failed\n");
	}
	return err;

nla_put_failure:
	fprintf(stderr,"nla_put_failure\n");
	nl_cb_put(cb);
	nlmsg_free(msg);
	nl_socket_free(sock);
	return 1;
}

