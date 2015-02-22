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

	struct rfkill_config  rfkill_cfg = {0};
	struct rfkill_data    *rfkill_data = NULL;

	struct netlink_config netlink_cfg = {0};
	struct netlink_data   *netlink_data = NULL;

	struct nl80211_config  nl80211_cfg = {0};
	struct nl80211_data    *nl80211_data = NULL;
	struct wiphy_info_data phy_info = {0};

	struct i802_bss		   bss = {0};

	// init squeduler
	eloop_init();

	// catch CTRL-C signal
	eloop_register_signal_terminate(&OnProperClose, NULL);

	// timer test
	eloop_register_timeout(1, 0, &OnTime, NULL, NULL);
	

	/** CREATE NETLINK SOCKET 
	 * Netlink socket family is a Linux kernel interface used for inter-process communication (IPC) 
	 * between the kernel and userspace processes,
	 */
	netlink_data = netlink_init(&netlink_cfg);
	if (netlink_data == NULL)
		EXIT_ERROR("netlink: init failed");

	/** CREATE A BASIC SOCKET FOR IOCTL */
	ioctl_socket = linux_create_socket();
	if (ioctl_socket < 0)
		EXIT_ERROR("ioctl socket: init failed");

	/* ATTACH NL80211 family */
	nl80211_cfg.ioctl_sock = ioctl_socket;
	nl80211_cfg.nl = netlink_data;
	strlcpy(nl80211_cfg.ifname, "wlan0", 6);
	if (linux_iface_up(ioctl_socket, nl80211_cfg.ifname) <= 0)
		EXIT_ERROR("error: interface is down\n");
	nl80211_data = driver_nl80211_init(&nl80211_cfg);
	if (nl80211_data == NULL)
		EXIT_ERROR("nl80211: init failed");

	/* Get Capabilitlies/Information */
	if (nl80211_get_capa(nl80211_data, &phy_info))
		EXIT_ERROR("nl80211: get capabilities failed\n");
	fprintf(stderr, "nl80211: '%s' detected!\n", phy_info.phyname);

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






	// TEST
	set_nl80211_rts_threshold(nl80211_data, 85);
	int v = 0;
	get_nl80211_rts_threshold(nl80211_data, &v);
	fprintf(stderr, "test : RTS = %d\n",v);



	// -------- SCHEDULER -----------
	eloop_run();
	// ------------------------------

	fprintf(stderr, "Closing...\n");


	
exit_label:
	fprintf(stderr, (err<0) ? "failed\n" : "success\n");


	//nl80211_destroy_bss(&bss);

	rfkill_deinit(rfkill_data);
	driver_nl80211_deinit(nl80211_data);
	linux_free_socket(ioctl_socket);
	netlink_deinit(netlink_data);
	
	fprintf(stderr, "bye\n");
	return err;
}

