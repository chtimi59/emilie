#ifndef DRIVER_NL80211_H
#define DRIVER_NL80211_H

#include "nl80211.h"

#ifdef CONFIG_LIBNL20
/* libnl 2.0 compatibility code */
#define nl_handle nl_sock
#define nl80211_handle_alloc nl_socket_alloc_cb
#define nl80211_handle_destroy nl_socket_free
#endif /* CONFIG_LIBNL20 */

struct nl80211_data {
	struct nl80211_config *cfg;
	
	struct nl_cb *nl_cb;
	struct nl_handle *nl;
	struct nl_handle *nl_event;
	
	int nl80211_id;				 // netlink nl80211 familly
    int nlctrl_id;				 // netlink nlctrl familly
	char ifname[IFNAMSIZ];		 // wifi interface name (ex: eth0)
	int  ifindex;				 // wifi index
	
	u8 macaddr[ETH_ALEN];		 // mac address
	enum nl80211_iftype mode;	 // should be AP mode

	char monitor_name[IFNAMSIZ]; // monitor name (ex: mon.wlan0)
	int  monitor_ifidx;			 // monitor interface index
	int  monitor_sock;			 // monitor socket

	char phy_name[IFNAMSIZ]; // monitor name (ex: phy0)
	int  phyindex;			 // phy index (ex: phy0)
	struct wiphy_info_data* phy_info; // info from get_capa()

	char brname[IFNAMSIZ]; // bridge name (ex: eth0)
	int  br_ifindex;	   // bridge index
};


struct nl80211_config {
	void *ctx;
	int ioctl_sock;
	struct netlink_data* nl;
	char ifname[32];
};

struct nl80211_data * driver_nl80211_init(struct nl80211_config *);
void driver_nl80211_deinit(struct nl80211_data *);

int nl_get_multicast_id(struct nl80211_data *ctx, const char *family, const char *group);

void * nl80211_cmd(struct nl80211_data *ctx, struct nl_msg *msg, int flags, uint8_t cmd);
int send_and_recv_msgs(struct nl80211_data *ctx, struct nl_msg *msg, int(*valid_handler)(struct nl_msg *, void *), void *valid_data);
struct nl_msg * nl80211_cmd_msg(struct nl80211_data *ctx, int flags, uint8_t cmd);





struct i802_bss {
	struct nl_cb *nl_cb;
};
void nl80211_destroy_bss(struct i802_bss *bss);
int nl80211_init_bss(struct i802_bss *bss);


int nl80211_flush(struct nl80211_data *ctx);
int nl80211_deauth(struct nl80211_data *ctx, const u8 *addr, int reason);
int nl80211_send_mlme(struct nl80211_data *ctx, const u8 *data, size_t data_len, int noack);

#endif /* DRIVER_NL80211_H */