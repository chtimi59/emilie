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
#include "ieee802_11_defs.h"
#include "ieee802_11_common.h"
#include "driver_nl80211_monitor.h"

#include "driver_helper.cxx"



// 80211 callback
static int no_seq_check(struct nl_msg *msg, void *arg) {
    //fprintf(stderr, "no_seq_check\n");
    return NL_OK;
}

// 80211 callback
static int process_global_event(struct nl_msg *msg, void *arg) {
    fprintf(stderr, "process_global_event\n");
    return NL_SKIP;
}

// 80211 callback
static int process_bss_event(struct nl_msg *msg, void *arg) {
    fprintf(stderr, "process_bss_event\n");
    return NL_SKIP;
}


// eloop squeduler event
void wpa_driver_nl80211_event_receive(int sock, void *eloop_ctx, void *handle) {
    fprintf(stderr, "wpa_driver_nl80211_event_receive\n");
    printf("*\n");
}



struct nl80211_data * driver_nl80211_init(struct nl80211_config *cfg)
{
    struct nl80211_data *ctx;

    ctx = zalloc(sizeof(struct nl80211_data));
    if (ctx == NULL)
        return NULL;
    
    ctx->cfg = cfg;
    ctx->ifindex = -1;
    ctx->phyindex = -1;
    ctx->monitor_ifidx = -1;
    ctx->br_ifindex = -1;
    memset(ctx->ifname, 0, IFNAMSIZ);
    snprintf(ctx->ifname, IFNAMSIZ, ctx->cfg->ifname);
    ctx->ifindex  = if_nametoindex(cfg->ifname);
    fprintf(stderr, "nl80211: '%s' index: %d\n", ctx->ifname, ctx->ifindex);



    // NETLINK CALLBACK
    ctx->nl_cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (ctx->nl_cb == NULL) {
        fprintf(stderr,  "nl80211: Failed to allocate netlink callbacks\n");
        return NULL;
    }
    nl_cb_set(ctx->nl_cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    nl_cb_set(ctx->nl_cb, NL_CB_VALID, NL_CB_CUSTOM, process_global_event, ctx);


    // "nl"
    ctx->nl       = nl_create_handle(ctx->nl_cb, "nl");
    if (ctx->nl == NULL)
        goto err;

    // "event"
    ctx->nl_event = nl_create_handle(ctx->nl_cb, "event");
    if (ctx->nl_event == NULL)
        goto err;


    // find "nl80211" familly in "nl"
    ctx->nl80211_id = genl_ctrl_resolve(ctx->nl, "nl80211");
    if (ctx->nl80211_id < 0) {
        fprintf(stderr,  "nl80211: 'nl80211' generic netlink not found\n");
        goto err;
    } else {
        fprintf(stderr, "nl80211: 'nl80211' familly found at %d\n", ctx->nl80211_id);
    }

    
    
    /*ret = nl_get_multicast_id(global, "nl80211", "mlme");
    if (ret >= 0)
        ret = nl_socket_add_membership(global->nl_event, ret);
    if (ret < 0) {
        fprintf(stderr,  "nl80211: Could not add multicast "
            "membership for mlme events: %d (%s)",
            ret, strerror(-ret));
        goto err;
    }*/

    
    nl80211_register_eloop_read(&ctx->nl_event, wpa_driver_nl80211_event_receive, ctx->nl_cb);

    return ctx;

err:
    if (ctx->nl_event) nl_destroy_handles(&ctx->nl_event);
    if (ctx->nl) nl_destroy_handles(&ctx->nl);
    nl_cb_put(ctx->nl_cb);
    ctx->nl_cb = NULL;
    return NULL;
}

void driver_nl80211_deinit(struct nl80211_data *ctx)
{
    if (ctx == NULL)
        return;
    
    if (ctx->phy_info) free(ctx->phy_info);
    if (ctx->nl_event) nl_destroy_handles(&ctx->nl_event);
    if (ctx->nl) nl_destroy_handles(&ctx->nl);
    nl_cb_put(ctx->nl_cb);
    ctx->nl_cb = NULL;
}






int nl80211_init_bss(struct i802_bss *bss) {
    bss->nl_cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!bss->nl_cb)
        return -1;

    nl_cb_set(bss->nl_cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    nl_cb_set(bss->nl_cb, NL_CB_VALID, NL_CB_CUSTOM, process_bss_event, bss);
    return 0;
}


void nl80211_destroy_bss(struct i802_bss *bss) {  
    nl_cb_put(bss->nl_cb);
    bss->nl_cb = NULL;
}


int nl80211_flush(struct nl80211_data *ctx)
{
    fprintf(stderr, "nl80211: flush -> DEL_STATION %s (all)\n", ctx->ifname);
    struct nl_msg *msg;
    int res;
    /*
    * XXX: FIX! this needs to flush all VLANs too
    */

    msg = nlmsg_alloc();
    if (!msg) goto fail;
    if (!nl80211_cmd(ctx, msg, 0, NL80211_CMD_DEL_STATION)) goto fail;
    if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, ctx->ifindex)) goto fail;
    res = send_and_recv_msgs(ctx, msg, NULL, NULL);
    if (res) {
        fprintf(stderr, "nl80211: Station flush failed: ret=%d (%s)\n", res, strerror(-res));
    }
    return res;

fail:
    if (msg) nlmsg_free(msg);
    return -1;
}



int nl80211_deauth(struct nl80211_data *ctx, const u8 *addr, int reason) {
    struct ieee80211_mgmt mgmt;
    memset(&mgmt, 0, sizeof(mgmt));
    mgmt.frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,WLAN_FC_STYPE_DEAUTH);
    memcpy(mgmt.da, addr, ETH_ALEN);
    memcpy(mgmt.sa, ctx->macaddr, ETH_ALEN);
    memcpy(mgmt.bssid, ctx->macaddr, ETH_ALEN);
    mgmt.u.deauth.reason_code = host_to_le16(reason);
    return nl80211_send_mlme(ctx, (u8 *) &mgmt, IEEE80211_HDRLEN + sizeof(mgmt.u.deauth), 0);
}


int nl80211_send_mlme(struct nl80211_data *ctx, const u8 *data, size_t len, int noack)
{
    struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *) data;
    u16 fc = le_to_host16(mgmt->frame_control);

    fprintf(stderr, "nl80211: send_mlme - da=%02x:%02x:%02x:%02x:%02x:%02x noack=%d fc=0x%x (%s)\n",
           mgmt->da[0],mgmt->da[1],mgmt->da[2],mgmt->da[3],mgmt->da[4],mgmt->da[5],
           noack, fc, fc2str(fc));

    return nl80211_send_monitor(ctx, data, len, noack);
}




