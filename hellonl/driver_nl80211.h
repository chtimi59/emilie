#ifndef DRIVER_NL80211_H
#define DRIVER_NL80211_H

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
	int ifindex;
	int nl80211_id;
};

struct nl80211_config {
	void *ctx;
	int ioctl_sock;
	struct netlink_data* nl;
	char ifname[32];
};

struct nl80211_data * driver_nl80211_init(struct nl80211_config *);
void driver_nl80211_deinit(struct nl80211_data *);


void * nl80211_cmd(struct nl80211_data *ctx, struct nl_msg *msg, int flags, uint8_t cmd);
int send_and_recv_msgs(struct nl80211_data *ctx, struct nl_msg *msg, int(*valid_handler)(struct nl_msg *, void *), void *valid_data);
struct nl_msg * nl80211_cmd_msg(struct nl80211_data *ctx, int flags, uint8_t cmd);





struct i802_bss {
	struct nl_cb *nl_cb;
};
void nl80211_destroy_bss(struct i802_bss *bss);
int nl80211_init_bss(struct i802_bss *bss);


#endif /* DRIVER_NL80211_H */