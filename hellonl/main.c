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

#include "driver_nl80211_rts.h"
#include "driver_nl80211_capa.h"
#include "driver_nl80211_interface.h"
#include "driver_nl80211_monitor.h"


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
	int ioctl_socket;

	struct rfkill_config  rfkill_cfg = { 0 };
	struct rfkill_data    *rfkill_data = NULL;

	struct netlink_config netlink_cfg = { 0 };
	struct netlink_data   *netlink_data = NULL;

	struct nl80211_config  nl80211_cfg = { 0 };
	struct nl80211_data    *nl80211_data = NULL;
	
	//struct i802_bss		   bss = { 0 };

	// init squeduler
	eloop_init();

	// catch CTRL-C signal
	eloop_register_signal_terminate(&OnProperClose, NULL);

	// timer test
	eloop_register_timeout(1, 0, &OnTime, NULL, NULL);

	/** CREATE A BASIC SOCKET FOR IOCTL */
	ioctl_socket = linux_create_socket();
	if (ioctl_socket < 0)
		EXIT_ERROR("ioctl socket: init failed");



	/** CREATE NETLINK SOCKET
	 * Netlink socket family is a Linux kernel interface used for inter-process communication (IPC)
	 * between the kernel and userspace processes,
	 */
	netlink_data = netlink_init(&netlink_cfg);
	if (netlink_data == NULL)
		EXIT_ERROR("netlink: init failed");

	/* ATTACH NL80211 family */
	nl80211_cfg.ioctl_sock = ioctl_socket;
	nl80211_cfg.nl = netlink_data;
	strlcpy(nl80211_cfg.ifname, "wlan0", 6);

	// set interface down (seems safe for configuration)
	fprintf(stderr, "set interface %s down\n", nl80211_cfg.ifname);
	if (linux_set_iface_flags(ioctl_socket, nl80211_cfg.ifname, false))
		EXIT_ERROR("error: could'nt set interface down\n");

	nl80211_data = driver_nl80211_init(&nl80211_cfg);
	if (nl80211_data == NULL)
		EXIT_ERROR("nl80211: init failed");



	/* Get Physical interface associate to nl80211  +Some Capabilitlies */

	if (nl80211_feed_capa(nl80211_data))
		EXIT_ERROR("nl80211: get capabilities failed\n");
	if (nl80211_get_wiphy_index(nl80211_data, &nl80211_data->phyindex))
		EXIT_ERROR("nl80211: could'nt find phy index\n");
	fprintf(stderr, "nl80211: '%s' index: %d\n", nl80211_data->phy_name, nl80211_data->phyindex);


#if 1
	// TEST
	set_nl80211_rts_threshold(nl80211_data, 85);
	int v = 0;
	get_nl80211_rts_threshold(nl80211_data, &v);
	fprintf(stderr, "test : RTS = %d\n", v);
#endif


	// Change ifmode if needed
	if (nl80211_get_ifmode(nl80211_data, &nl80211_data->mode))
		EXIT_ERROR("nl80211: could'nt find phy mode\n");	
	if (nl80211_data->mode != NL80211_IFTYPE_AP)
	{
		// change mode		
		fprintf(stderr, "nl80211: change mode from %d to %d\n", nl80211_data->mode, NL80211_IFTYPE_AP);
		if (nl80211_set_ifmode(nl80211_data, NL80211_IFTYPE_AP))
			EXIT_ERROR("nl80211: could'nt change phy mode\n");
		if (nl80211_get_ifmode(nl80211_data, &nl80211_data->mode))
			EXIT_ERROR("nl80211: could'nt find phy mode\n");
	}
	if (nl80211_data->mode != NL80211_IFTYPE_AP)
		EXIT_ERROR("nl80211: can't use wireless device as an AP\n");
	fprintf(stderr, "nl80211: phy mode %d\n", nl80211_data->mode);


	//get interface mac address
	if (linux_get_ifhwaddr(ioctl_socket, nl80211_cfg.ifname, nl80211_data->macaddr))
		EXIT_ERROR("error: could'nt get mac address\n");
	fprintf(stderr, "nl80211: mac address %02x:%02x:%02x:%02x:%02x:%02x\n", 
		nl80211_data->macaddr[0], nl80211_data->macaddr[1], nl80211_data->macaddr[2],
		nl80211_data->macaddr[3], nl80211_data->macaddr[4], nl80211_data->macaddr[5]);

	// Add monitor interface
	nl80211_create_monitor_interface(nl80211_data);

	// Set interface up
	fprintf(stderr, "set interface %s up\n", nl80211_cfg.ifname);
	if (linux_set_iface_flags(ioctl_socket, nl80211_cfg.ifname, true))
		EXIT_ERROR("error: could'nt set interface up\n");
	


	/*
	if (linux_br_get(nl80211_data->brname, nl80211_cfg.ifname, &nl80211_data->br_ifindex) == 0) {
		fprintf(stderr, "nl80211: '%s' bridge index :%d\n", nl80211_data->brname, nl80211_data->br_ifindex);
	} else {
		fprintf(stderr, "nl80211: add bridge\n");
		ifindex = if_nametoindex(params->bridge[i]);
		if (ifindex)
			add_ifidx(drv, ifindex);
		if (ifindex == br_ifindex)
			br_added = 1;
	}
	*/

	// RF-KILL (usefull ?)
	rfkill_data = rfkill_init(&rfkill_cfg);
	if (rfkill_data == NULL) {
		if (rfkill_data) free(rfkill_data);
		EXIT_ERROR("nl80211: RFKILL status not available");
	}



	// Init BSS
	/*if (nl80211_init_bss(&bss)) 
		EXIT_ERROR("nl80211: init bss failed");
	*/










	// -------- SCHEDULER -----------
	eloop_run();
	// ------------------------------

	fprintf(stderr, "Closing...\n");


	
exit_label:
	fprintf(stderr, (err<0) ? "failed\n" : "success\n");

	nl80211_remove_monitor_interface(nl80211_data);
	//nl80211_destroy_bss(&bss);

	rfkill_deinit(rfkill_data);
	driver_nl80211_deinit(nl80211_data);
	netlink_deinit(netlink_data);
	linux_free_socket(ioctl_socket);

	fprintf(stderr, "bye\n");
	return err;
}

