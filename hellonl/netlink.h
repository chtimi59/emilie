/*
* Netlink helper functions for driver wrappers
* Copyright (c) 2002-2009, Jouni Malinen <j@w1.fi>
*
* This software may be distributed under the terms of the BSD license.
* See README for more details.
*/

#ifndef NETLINK_H
#define NETLINK_H

struct ifinfomsg;

struct netlink_data {
	struct netlink_config *cfg;
	int sock;
};

struct netlink_config {
	void *ctx;
	void(*newlink_cb)(void *ctx, struct ifinfomsg *ifi, u8 *buf, size_t len);
	void(*dellink_cb)(void *ctx, struct ifinfomsg *ifi, u8 *buf, size_t len);
};

struct netlink_data * netlink_init(struct netlink_config *cfg);
void netlink_deinit(struct netlink_data *netlink);
int netlink_send_oper_ifla(struct netlink_data *netlink, int ifindex, int linkmode, int operstate);

#ifndef IFLA_LINKMODE
#define IFLA_LINKMODE 17
#define IF_OPER_DORMANT 5
#define IF_OPER_UP 6
#endif

#endif /* NETLINK_H */