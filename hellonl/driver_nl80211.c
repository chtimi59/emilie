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
    int ret;

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





    /* NETLINK CALLBACK :
                                                                        
                   PROCESS X            PROCESS Y            PROCESS Z  
    user            PID55                PID56                PID57     
    space             ^                    ^                   ^        
                      |                    |                   |        (nl_cb)
    netlink         Queue                Queue               Queue      
                      |                    |                   |        
                   -------------------------------         ---------    
    kernel         |         SUBSYSTEM A         |         |SUBSY N|    
                   -------------------------------         ---------    
                                                                        
    */
    ctx->nl_cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (ctx->nl_cb == NULL) {
        fprintf(stderr,  "nl80211: Failed to allocate netlink callbacks\n");
        return NULL;
    }
    nl_cb_set(ctx->nl_cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    nl_cb_set(ctx->nl_cb, NL_CB_VALID, NL_CB_CUSTOM,     process_global_event, ctx);


    // Allocate new socket on generic netlink with custom callbacks
    ctx->nl       = nl_create_handle(ctx->nl_cb, "nl");
    if (ctx->nl == NULL)
        goto err;

    // Find "nl80211" netlink familly id
    ctx->nl80211_id = genl_ctrl_resolve(ctx->nl, "nl80211");
    if (ctx->nl80211_id < 0) {
        fprintf(stderr,  "nl80211: 'nl80211' generic netlink not found\n");
        goto err;
    } else {
        fprintf(stderr, "nl80211: 'nl80211' familly found at %d\n", ctx->nl80211_id);
    }


    // Find "nlctrl" netlink familly
    ctx->nlctrl_id = genl_ctrl_resolve(ctx->nl, "nlctrl");
    if (ctx->nlctrl_id < 0) {
        fprintf(stderr,  "nlctrl: 'nlctrl' generic netlink not found\n");
        goto err;
    } else {
        fprintf(stderr, "nlctrl: 'nlctrl' familly found at %d\n", ctx->nlctrl_id);
    }



    // Allocate another new socket on generic netlink with custom callbacks (used for specific multicast events)
    ctx->nl_event = nl_create_handle(ctx->nl_cb, "event");
    if (ctx->nl_event == NULL)
        goto err;
    // find mlme group in "nl80211" family
    ret = nl_get_multicast_id(ctx, "nl80211", "mlme");
    if (ret >= 0)
        fprintf(stderr,  "nl80211: add multicast membership for 'mlme' group (=%d)\n", ret);
        ret = nl_socket_add_membership(ctx->nl_event, ret);
    if (ret < 0) {
        fprintf(stderr,  "nl80211: Could not add multicast membership for mlme group: %d (%s)\n", ret, strerror(-ret));
        goto err;
    }
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









struct family_data {
    const char *group;
    int id;
};


static int family_handler(struct nl_msg *msg, void *arg)
{
    struct family_data *res = arg;
    struct nlattr *tb[CTRL_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *mcgrp;
    int i;

    nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
    if (!tb[CTRL_ATTR_MCAST_GROUPS])
        return NL_SKIP;

    nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], i)
    {
        struct nlattr *tb2[CTRL_ATTR_MCAST_GRP_MAX + 1];
        nla_parse(tb2, CTRL_ATTR_MCAST_GRP_MAX, nla_data(mcgrp), nla_len(mcgrp), NULL);
        if (!tb2[CTRL_ATTR_MCAST_GRP_NAME] ||
            !tb2[CTRL_ATTR_MCAST_GRP_ID] ||
            strncmp(nla_data(tb2[CTRL_ATTR_MCAST_GRP_NAME]), res->group, nla_len(tb2[CTRL_ATTR_MCAST_GRP_NAME])) != 0)
            continue;
        res->id = nla_get_u32(tb2[CTRL_ATTR_MCAST_GRP_ID]);
        break; 
    };

    return NL_SKIP;
}

int nl_get_multicast_id(struct nl80211_data *ctx, const char *family, const char *group)
{
    struct nl_msg *msg;
    int ret;
    struct family_data res = { group, -ENOENT };

    msg = nlmsg_alloc();
    if (!msg)
        return -ENOMEM;
    
    // Probe a family
    if (!genlmsg_put(msg, 0, 0, ctx->nlctrl_id, 0, 0, CTRL_CMD_GETFAMILY, 0))
        goto failed;
    if (nla_put_string(msg, CTRL_ATTR_FAMILY_NAME, family))
        goto failed;

    ret = send_and_recv_msgs(ctx, msg, family_handler, &res);
    if (ret == 0) ret = res.id;
    return ret;

failed:
    nlmsg_free(msg);
    return -1;
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






