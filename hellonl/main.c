#include "common.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#pragma message "LITTLE Endian"
#elif __BYTE_ORDER == __BIG_ENDIAN
#pragma message "BIG Endian"
#endif


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
#include "mngt.h"

#include "ieee802_11_defs.h"

#include "driver_nl80211_rts.h"
#include "driver_nl80211_capa.h"
#include "driver_nl80211_interface.h"
#include "driver_nl80211_monitor.h"
#include "driver_nl80211_freq.h"
#include "driver_nl80211_frag.h"
#include "driver_nl80211_beacon.h"


void OnProperClose(int sig, void *signal_ctx) {
    fprintf(stderr, "CTRL-C hit !\n");
    eloop_terminate();
}

void OnTime(void *eloop_data, void *user_ctx) {
    fprintf(stderr, "BEEP !\n");
    eloop_register_timeout(1, 0, &OnTime, NULL, NULL);
}

// wpa_driver_nl80211_event_rtm_newlink -> wpa_driver_nl80211_event_newlink
void OnNetlink_event_newlink(void *ctx, struct ifinfomsg *ifi, u8 *buf, size_t len)
{
    struct nl80211_data    *global = ctx;
    if ((global->monitor_ifidx != ifi->ifi_index) && (global->ifindex != ifi->ifi_index)) {
        fprintf(stderr, "nl80211: Ignore RTM_NEWLINK event for foreign ifindex %d", ifi->ifi_index);
        return;
    }

    debugPrint(ifi, buf, len);
}

// wpa_driver_nl80211_event_rtm_dellink -> wpa_driver_nl80211_event_dellink
void OnNetlink_event_dellink(void *ctx, struct ifinfomsg *ifi, u8 *buf, size_t len)
{
    struct nl80211_data    *global = ctx;
    if ((global->monitor_ifidx != ifi->ifi_index) && (global->ifindex != ifi->ifi_index)) {
        fprintf(stderr, "nl80211: Ignore RTM_NEWLINK event for foreign ifindex %d", ifi->ifi_index);
        return;
    }

    debugPrint(ifi, buf, len);
}




#define EXIT_ERROR(_str_) do { err=-1; fprintf(stderr, _str_"\n"); goto exit_label; } while(0)

int main(int argc, char **argv)
{
    int err = 0;
    int ioctl_socket = -1;

    struct rfkill_config  rfkill_cfg = { 0 };
    struct rfkill_data    *rfkill_data = NULL;

    struct netlink_config netlink_cfg = { 0 };
    struct netlink_data   *netlink_data = NULL;

    struct nl80211_config  nl80211_cfg = { 0 };
    struct nl80211_data    *nl80211_data = NULL;
    
    //struct i802_bss           bss = { 0 };

    // init squeduler
    eloop_init();

    // catch CTRL-C signal
    eloop_register_signal_terminate(&OnProperClose, NULL);

    // timer test
    eloop_register_timeout(1, 0, &OnTime, NULL, NULL);


    // RF-KILL (usefull ?)
    rfkill_data = rfkill_init(&rfkill_cfg);
    if (rfkill_data == NULL) {
        if (rfkill_data) free(rfkill_data);
        EXIT_ERROR("nl80211: RFKILL status not available");
    }



    /** CREATE A BASIC SOCKET FOR IOCTL */
    ioctl_socket = linux_create_socket();
    if (ioctl_socket < 0)
        EXIT_ERROR("ioctl socket: init failed");
    nl80211_cfg.ioctl_sock = ioctl_socket;

    // test wlan0-5 interfaces    
    do {        
        strlcpy(nl80211_cfg.ifname, "wlan0", 6);
        if (-1!=linux_get_if_ifidx(ioctl_socket, nl80211_cfg.ifname)) break;
        strlcpy(nl80211_cfg.ifname, "wlan1", 6);
        if (-1!=linux_get_if_ifidx(ioctl_socket, nl80211_cfg.ifname)) break;
        strlcpy(nl80211_cfg.ifname, "wlan2", 6);
        if (-1!=linux_get_if_ifidx(ioctl_socket, nl80211_cfg.ifname)) break;
        strlcpy(nl80211_cfg.ifname, "wlan3", 6);
        if (-1!=linux_get_if_ifidx(ioctl_socket, nl80211_cfg.ifname)) break;
        strlcpy(nl80211_cfg.ifname, "wlan4", 6);
        if (-1!=linux_get_if_ifidx(ioctl_socket, nl80211_cfg.ifname)) break;
        strlcpy(nl80211_cfg.ifname, "wlan5", 6);
        if (-1!=linux_get_if_ifidx(ioctl_socket, nl80211_cfg.ifname)) break;
        EXIT_ERROR("error: no wlan interface found (tested: wlan0 -> wlan5)\n");
    } while(0);
    


    /* NETLINK

    http://www.carisma.slowglass.com/~tgr/libnl/doc/core.html
    Netlink socket family is a Linux kernel interface used for inter-process communication (IPC) between the kernel and userspace processes,
    as well as between user processes in a way similar to the Unix domain sockets.
    However, unlike INET sockets, Netlink communication cannot traverse host boundaries because Netlink processes addresses by process identifiers (PIDs),
    which are inherently local.

    A non-exhaustive list of the supported protocol entries follows:
    NETLINK_ROUTE: provide routing an link information
    NETLINK_FIREWALL
    NETLINK_NFLOG
    NETLINK_ARPD
    NETLINK_AUDIT
    [...]
    and User-defined Netlink protocol
    */
    
    /** NETLINK : NL80211 */
    fprintf(stderr, "\n** NETLINK : NL80211 init **\n");
    nl80211_data = driver_nl80211_init(&nl80211_cfg);
    if (nl80211_data == NULL)
        EXIT_ERROR("nl80211: init failed");    
    fprintf(stderr, "nl80211: init success\n");

    if (nl80211_get_ifmode(nl80211_data, &nl80211_data->mode))
        EXIT_ERROR("nl80211: could'nt find phy mode\n");    

    /** NETLINK : NETLINK_ROUTE / LINK FAMILIY (used to change link operation state) */
    fprintf(stderr, "\n** NETLINK : NETLINK_ROUTE init **\n");
    netlink_cfg.ctx = nl80211_data;
    netlink_cfg.newlink_cb = OnNetlink_event_newlink;
    netlink_cfg.dellink_cb = OnNetlink_event_dellink;
    netlink_data = netlink_init(&netlink_cfg);
    if (netlink_data == NULL)
        EXIT_ERROR("netlink: init failed");
    nl80211_cfg.nl = netlink_data; // share reference
    fprintf(stderr, "netlink: init success\n\n");


    // Ask nl80211 to create a monitor interface and create a socket on it
    // Monitor is used for RX and TX
    memset(nl80211_data->monitor_name, 0, IFNAMSIZ);
    snprintf(nl80211_data->monitor_name, IFNAMSIZ, "mon.%s", nl80211_data->ifname);
    nl80211_data->monitor_ifidx = linux_get_if_ifidx(ioctl_socket, nl80211_data->monitor_name);
    if (nl80211_create_monitor_interface(nl80211_data))
        EXIT_ERROR("nl80211: Monitor interface error\n");






#if 1
    //Get wlan0 interface mac address
    if (linux_get_ifhwaddr(ioctl_socket, nl80211_cfg.ifname, nl80211_data->macaddr))
        EXIT_ERROR("error: could'nt get mac address\n");
    fprintf(stderr, "nl80211: mac address %02x:%02x:%02x:%02x:%02x:%02x\n", 
        nl80211_data->macaddr[0], nl80211_data->macaddr[1], nl80211_data->macaddr[2],
        nl80211_data->macaddr[3], nl80211_data->macaddr[4], nl80211_data->macaddr[5]);
#endif


#if 1
    /* Get Physical interface associated to nl80211  +Some Capabilitlies */
    if (nl80211_feed_capa(nl80211_data))
        EXIT_ERROR("nl80211: get capabilities failed\n");
    if (nl80211_get_wiphy_index(nl80211_data, &nl80211_data->phyindex))
        EXIT_ERROR("nl80211: could'nt find phy index\n");
    fprintf(stderr, "nl80211: '%s' index: %d\n", nl80211_data->phy_name, nl80211_data->phyindex);
#endif


#if 1
{
    // Set RTS threshold
    int rts = 2347;
    set_nl80211_rts_threshold(nl80211_data, rts);
    get_nl80211_rts_threshold(nl80211_data, &rts);
    if (rts == -1) fprintf(stderr, "nl80211: RTS off\n");
    if (rts != -1) fprintf(stderr, "nl80211: RTS = %d B\n", rts);
}
#endif

#if 1
{
    // Set Fragmentation threshold
    int frag = 2346;
    fprintf(stderr, "nl80211: set fragmentation %d B\n", frag);
    if (set_nl80211_frag_threshold(nl80211_data, frag))
        EXIT_ERROR("error: could'nt set fragmentation\n");

}
#endif


#if 1
    // Change ifmode if needed
    if (nl80211_get_ifmode(nl80211_data, &nl80211_data->mode))
        EXIT_ERROR("nl80211: could'nt find phy mode\n");    
    if (nl80211_data->mode != NL80211_IFTYPE_AP)
    {
        // safe for reconfiguration
        fprintf(stderr, "set interface %s down\n", nl80211_cfg.ifname);
        if (linux_set_iface_flags(ioctl_socket, nl80211_cfg.ifname, false))
            EXIT_ERROR("error: could'nt set interface down\n");

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

    // Set wlan interface up
    fprintf(stderr, "set interface %s up\n", nl80211_cfg.ifname);
    if (linux_set_iface_flags(ioctl_socket, nl80211_cfg.ifname, true))
        EXIT_ERROR("error: could'nt set interface up\n");
#endif


#if 1
    // Set frequency
    int freq = 2437;
    fprintf(stderr, "nl80211: set frequency %dMHZ\n", freq);
    if (nl80211_set_channel(nl80211_data, freq))
        EXIT_ERROR("error: could'nt set frequency\n");
#endif


#if 1
    // flush stations
    nl80211_flush(nl80211_data);
    send_deauth(nl80211_data, broadcast_ether_addr, WLAN_REASON_DEAUTH_LEAVING);
#endif


#if 1
    // Set Beacons
    if (nl80211_set_ap(nl80211_data))
        EXIT_ERROR("error: could'nt set beacon\n");
#endif


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







    // Init BSS
    /*if (nl80211_init_bss(&bss)) 
        EXIT_ERROR("nl80211: init bss failed");
    */




    // NETLINK:  LINK UP !
    if (netlink_send_oper_ifla(netlink_data, nl80211_data->ifindex, -1, IF_OPER_UP))
        EXIT_ERROR("netlink: init failed");


    // -------- SCHEDULER -----------
    eloop_run();
    // ------------------------------

    fprintf(stderr, "Closing...\n");


    
exit_label:
    fprintf(stderr, (err<0) ? "failed\n" : "success\n");

    if (nl80211_data->monitor_ifidx >= 0) nl80211_remove_monitor_interface(nl80211_data);
    //nl80211_destroy_bss(&bss);

    rfkill_deinit(rfkill_data);
    driver_nl80211_deinit(nl80211_data);
    netlink_deinit(netlink_data);
    if (ioctl_socket!=-1) linux_free_socket(ioctl_socket);

    fprintf(stderr, "bye\n");
    return err;
}

