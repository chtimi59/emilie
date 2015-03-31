#include "common.h"
#include "eloop.h"
#include <net/if.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netpacket/packet.h>
#include <linux/filter.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <arpa/inet.h>

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include "driver_nl80211.h"
#include "netlink.h"
#include "nl80211.h"

#include "ioctl.h"
#include "driver_nl80211_monitor.h"
#include "radiotap_iter.h"
#include "ieee802_11_defs.h"
#include "ieee802_11_common.h"
#include "mngt.h"

// handle_tx_callback
static void handle_tx_frame(void *ctx, u8 *buf, size_t len, int ok)
{
    /*struct ieee80211_hdr *hdr;
    u16 fc;
    union wpa_event_data event;

    hdr = (struct ieee80211_hdr *) buf;
    fc = le_to_host16(hdr->frame_control);

    os_memset(&event, 0, sizeof(event));
    event.tx_status.type = WLAN_FC_GET_TYPE(fc);
    event.tx_status.stype = WLAN_FC_GET_STYPE(fc);
    event.tx_status.dst = hdr->addr1;
    event.tx_status.data = buf;
    event.tx_status.data_len = len;
    event.tx_status.ack = ok;
    wpa_supplicant_event(ctx, EVENT_TX_STATUS, &event);*/
    fprintf(stderr, "TX_STATUS %d\n", ok);
}

static void handle_rx_frame(struct nl80211_data* ctx, u8 *buf, size_t len, int datarate, int ssi_signal)
{
    struct ieee80211_hdr *hdr;
    u16 fc;
    hdr = (struct ieee80211_hdr *) buf;
    fc = le_to_host16(hdr->frame_control);

    if (len<IEEE80211_HDRLEN) {
        fprintf(stderr,"malformed packet\n");
        return;
    }

    switch (WLAN_FC_GET_TYPE(fc))
    {
        case WLAN_FC_TYPE_MGMT:
        {
            const u8 *bssid = get_hdr_bssid(hdr, len);
            if (memcmp(bssid,ctx->macaddr,6)==0) {
                //fprintf(stderr,"RECEIVED 802.11 MNGT FRAME %s\n", fc2str(fc));
                mngt_rx_handle(ctx, buf, len, datarate, ssi_signal);
                break;
            }

            if (memcmp(bssid,broadcast_ether_addr,6)==0) {
                //fprintf(stderr,"RECEIVED 802.11 MNGT FRAME (BROADCAST) %s\n",fc2str(fc));
                /*fprintf(stderr, "%02x:%02x:%02x:%02x:%02x:%02x\n", 
                    bssid[0], bssid[1], bssid[2],
                    bssid[3], bssid[4], bssid[5]);*/
                break;
            };
            break;
        }

        case WLAN_FC_TYPE_CTRL:
            /* can only get here with PS-Poll frames */
            /*wpa_printf(MSG_DEBUG, "CTRL");
            from_unknown_sta(drv, buf, len);*/
            break;
        case WLAN_FC_TYPE_DATA:
            /*from_unknown_sta(drv, buf, len);*/
            break;
    }
}

static void handle_monitor_read(int sock, void *eloop_ctx, void *sock_ctx)
{
    struct nl80211_data* ctx = eloop_ctx;
    
    int len;
    unsigned char buf[3000];
    struct ieee80211_radiotap_iterator iter;
    int ret;
    int datarate = 0, ssi_signal = 0;
    int injected = 0, failed = 0, rxflags = 0;

    len = recv(sock, buf, sizeof(buf), 0);
    if (len < 0) {
        fprintf(stderr, "nl80211: Monitor socket recv failed: %s\n", strerror(errno));
        return;
    }

    if (ieee80211_radiotap_iterator_init(&iter, (void *) buf, len, NULL)) {
        fprintf(stderr, "nl80211: received invalid radiotap frame\n");
        return;
    }

    while (1) {
        ret = ieee80211_radiotap_iterator_next(&iter);
        if (ret == -ENOENT)
            break;
        if (ret) {
            fprintf(stderr, "nl80211: received invalid radiotap frame (%d)\n", ret);
            return;
        }
        switch (iter.this_arg_index) {
        case IEEE80211_RADIOTAP_FLAGS:
            if (*iter.this_arg & IEEE80211_RADIOTAP_F_FCS)
                len -= 4;
            break;
        case IEEE80211_RADIOTAP_RX_FLAGS:
            rxflags = 1;
            break;
        case IEEE80211_RADIOTAP_TX_FLAGS:
            injected = 1;
            failed = le_to_host16((*(uint16_t *) iter.this_arg)) &IEEE80211_RADIOTAP_F_TX_FAIL;
            break;
        case IEEE80211_RADIOTAP_DATA_RETRIES:
            break;
        case IEEE80211_RADIOTAP_CHANNEL:
            /* TODO: convert from freq/flags to channel number */
            break;
        case IEEE80211_RADIOTAP_RATE:
            datarate = *iter.this_arg * 5;
            break;
        case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
            ssi_signal = (s8) *iter.this_arg;
            break;
        }
    }

    if (rxflags && injected) return;
    if (!injected)
        handle_rx_frame(ctx, buf + iter._max_length, len - iter._max_length, datarate, ssi_signal);
    else
        handle_tx_frame(ctx, buf + iter._max_length, len - iter._max_length, !failed);
}

static int nl80211_create_iface_once(struct nl80211_data* ctx, int(*handler)(struct nl_msg *, void *), void *arg)
{
    int ret = -ENOBUFS;
    struct nl_msg *msg;
    struct nlattr *flags;
    int ifidx;
    const char *ifname = ctx->monitor_name;
    fprintf(stderr, "nl80211: Create monitor interface %s\n", ifname);

    msg = nl80211_cmd_msg(ctx, 0, NL80211_CMD_NEW_INTERFACE);
    if (!msg)
        goto fail;

    if (nla_put_string(msg, NL80211_ATTR_IFNAME, ifname))
        goto fail;
    
    if (nla_put_u32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR))
        goto fail;

    flags = nla_nest_start(msg, NL80211_ATTR_MNTR_FLAGS);
    if (!flags)
        goto fail;

    if (nla_put_flag(msg, NL80211_MNTR_FLAG_COOK_FRAMES))
        goto fail;

    nla_nest_end(msg, flags);

    /*
    * Tell cfg80211 that the interface belongs to the socket that created
    * it, and the interface should be deleted when the socket is closed.
    */
    if (nla_put_flag(msg, NL80211_ATTR_IFACE_SOCKET_OWNER))
        goto fail;

    ret = send_and_recv_msgs(ctx, msg, handler, arg);
    msg = NULL;


    if (ret) {
    fail:
        nlmsg_free(msg);
        fprintf(stderr, "nl80211: Failed to create Monitor interface: %d (%s)\n", ret, strerror(-ret));
        return -1;
    }

    ifidx = if_nametoindex(ifname);
    fprintf(stderr, "nl80211: New interface %s created at ifindex=%d\n", ifname, ifidx);
    if (ifidx <= 0)
        return -1;

    return ifidx;
}

void nl80211_remove_iface(struct nl80211_data *ctx, int ifidx)
{
    struct nl_msg *msg = NULL;

    fprintf(stderr, "nl80211: Remove interface ifindex=%d\n", ifidx);

    msg = nlmsg_alloc();
    if (!msg)
        goto failed;

    if (!nl80211_cmd(ctx, msg, 0, NL80211_CMD_DEL_INTERFACE)) 
        goto failed;

    if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifidx))
        goto failed;

    if (send_and_recv_msgs(ctx, msg, NULL, NULL) == 0)
        return;

failed:
    if (msg) nlmsg_free(msg);
    fprintf(stderr, "failed to remove interface (ifidx=%d)\n", ifidx);
}





/*
* we post-process the filter code later and rewrite
* this to the offset to the last instruction
*/
#define PASS    0xFF
#define FAIL    0xFE

static struct sock_filter msock_filter_insns[] = {
    /*
    * do a little-endian load of the radiotap length field
    */
    /* load lower byte into A */
    BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 2),
    /* put it into X (== index register) */
    BPF_STMT(BPF_MISC | BPF_TAX, 0),
    /* load upper byte into A */
    BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 3),
    /* left-shift it by 8 */
    BPF_STMT(BPF_ALU | BPF_LSH | BPF_K, 8),
    /* or with X */
    BPF_STMT(BPF_ALU | BPF_OR | BPF_X, 0),
    /* put result into X */
    BPF_STMT(BPF_MISC | BPF_TAX, 0),

    /*
    * Allow management frames through, this also gives us those
    * management frames that we sent ourselves with status
    */
    /* load the lower byte of the IEEE 802.11 frame control field */
    BPF_STMT(BPF_LD | BPF_B | BPF_IND, 0),
    /* mask off frame type and version */
    BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0xF),
    /* accept frame if it's both 0, fall through otherwise */
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0, PASS, 0),

    /*
    * TODO: add a bit to radiotap RX flags that indicates
    * that the sending station is not associated, then
    * add a filter here that filters on our DA and that flag
    * to allow us to deauth frames to that bad station.
    *
    * For now allow all To DS data frames through.
    */
    /* load the IEEE 802.11 frame control field */
    BPF_STMT(BPF_LD | BPF_H | BPF_IND, 0),
    /* mask off frame type, version and DS status */
    BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0x0F03),
    /* accept frame if version 0, type 2 and To DS, fall through otherwise
    */
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x0801, PASS, 0),

#if 0
    /*
    * drop non-data frames
    */
    /* load the lower byte of the frame control field */
    BPF_STMT(BPF_LD | BPF_B | BPF_IND, 0),
    /* mask off QoS bit */
    BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0x0c),
    /* drop non-data frames */
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 8, 0, FAIL),
#endif
    /* load the upper byte of the frame control field */
    BPF_STMT(BPF_LD | BPF_B | BPF_IND, 1),
    /* mask off toDS/fromDS */
    BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0x03),
    /* accept WDS frames */
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 3, PASS, 0),

    /*
    * add header length to index
    */
    /* load the lower byte of the frame control field */
    BPF_STMT(BPF_LD | BPF_B | BPF_IND, 0),
    /* mask off QoS bit */
    BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0x80),
    /* right shift it by 6 to give 0 or 2 */
    BPF_STMT(BPF_ALU | BPF_RSH | BPF_K, 6),
    /* add data frame header length */
    BPF_STMT(BPF_ALU | BPF_ADD | BPF_K, 24),
    /* add index, was start of 802.11 header */
    BPF_STMT(BPF_ALU | BPF_ADD | BPF_X, 0),
    /* move to index, now start of LL header */
    BPF_STMT(BPF_MISC | BPF_TAX, 0),

    /*
    * Accept empty data frames, we use those for
    * polling activity.
    */
    BPF_STMT(BPF_LD | BPF_W | BPF_LEN, 0),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_X, 0, PASS, 0),

    /*
    * Accept EAPOL frames
    */
    BPF_STMT(BPF_LD | BPF_W | BPF_IND, 0),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0xAAAA0300, 0, FAIL),
    BPF_STMT(BPF_LD | BPF_W | BPF_IND, 4),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x0000888E, PASS, FAIL),

    /* keep these last two statements or change the code below */
    /* return 0 == "DROP" */
    BPF_STMT(BPF_RET | BPF_K, 0),
    /* return ~0 == "keep all" */
    BPF_STMT(BPF_RET | BPF_K, ~0),
};

static struct sock_fprog msock_filter = {
    .len = ARRAY_SIZE(msock_filter_insns),
    .filter = msock_filter_insns,
};


static int add_monitor_filter(int s)
{
    int idx;

    /* rewrite all PASS/FAIL jump offsets */
    for (idx = 0; idx < msock_filter.len; idx++)
    {
        struct sock_filter *insn = &msock_filter_insns[idx];

        if (BPF_CLASS(insn->code) == BPF_JMP) {
            if (insn->code == (BPF_JMP | BPF_JA)) {
                if (insn->k == PASS)
                    insn->k = msock_filter.len - idx - 2;
                else if (insn->k == FAIL)
                    insn->k = msock_filter.len - idx - 3;
            }

            if (insn->jt == PASS)
                insn->jt = msock_filter.len - idx - 2;
            else if (insn->jt == FAIL)
                insn->jt = msock_filter.len - idx - 3;

            if (insn->jf == PASS)
                insn->jf = msock_filter.len - idx - 2;
            else if (insn->jf == FAIL)
                insn->jf = msock_filter.len - idx - 3;
        }
    }

    if (setsockopt(s, SOL_SOCKET, SO_ATTACH_FILTER,
        &msock_filter, sizeof(msock_filter))) {
        fprintf(stderr, "nl80211: setsockopt(SO_ATTACH_FILTER) failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int nl80211_create_monitor_interface(struct nl80211_data* ctx)
{
    if (ctx->monitor_ifidx < 0) {
        // create monitor
        ctx->monitor_ifidx = nl80211_create_iface_once(ctx, NULL, NULL);
        if (ctx->monitor_ifidx == -EOPNOTSUPP) {
            /*
            * This is backward compatibility for a few versions of
            * the kernel only that didn't advertise the right
            * attributes for the only driver that then supported
            * AP mode w/o monitor -- ath6kl.
            */
            fprintf(stderr, "nl80211: Driver does not support monitor interface type - try to run without it\n");
        }
    } else {
        fprintf(stderr, "nl80211: re-use monitor '%s' index: %d\n",ctx->monitor_name, ctx->monitor_ifidx);
    }

    if (ctx->monitor_ifidx < 0)
        return -1;

    // interface up!    
    if (linux_set_iface_flags(ctx->cfg->ioctl_sock, ctx->monitor_name, 1))
        goto error;

    // create socket on monitor interface
    {
        struct sockaddr_ll ll;
        int optval;
        socklen_t optlen;

        memset(&ll, 0, sizeof(ll));
        ll.sll_family = AF_PACKET;
        ll.sll_ifindex = ctx->monitor_ifidx;
        ctx->monitor_sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

        if (ctx->monitor_sock < 0) {
            fprintf(stderr, "nl80211: socket[PF_PACKET,SOCK_RAW] failed: %s\n", strerror(errno));
            goto error;
        }

        if (add_monitor_filter(ctx->monitor_sock)) {
            fprintf(stderr, "Failed to set socket filter for monitor interface; do filtering in user space\n");
            /* This works, but will cost in performance. */
        }

        if (bind(ctx->monitor_sock, (struct sockaddr *) &ll, sizeof(ll)) < 0) {
            fprintf(stderr, "nl80211: monitor socket bind failed: %s\n", strerror(errno));
            goto error;
        }

        optlen = sizeof(optval);
        optval = 20;
        if (setsockopt(ctx->monitor_sock, SOL_SOCKET, SO_PRIORITY, &optval, optlen)) {
            fprintf(stderr, "nl80211: Failed to set socket priority: %s\n", strerror(errno));
            goto error;
        }

        if (eloop_register_read_sock(ctx->monitor_sock, handle_monitor_read, ctx, NULL)) {
            fprintf(stderr, "nl80211: Could not register monitor read socket\n");
            goto error;
        }
    }
    
    return 0;
    
error:
    nl80211_remove_monitor_interface(ctx);
    return -1;
}


void nl80211_remove_monitor_interface(struct nl80211_data* ctx)
{
    if (ctx->monitor_ifidx >= 0) {
        nl80211_remove_iface(ctx, ctx->monitor_ifidx);
        ctx->monitor_ifidx = -1;
    }

    if (ctx->monitor_sock >= 0) {
        eloop_unregister_read_sock(ctx->monitor_sock);
        close(ctx->monitor_sock);
        ctx->monitor_sock = -1;
    }
}



#define WPA_PUT_LE16(a, val)                    \
      do {                                    \
           (a)[1] = ((u16) (val)) >> 8;    \
           (a)[0] = ((u16) (val)) & 0xff;  \
      } while (0)



int monitor_tx(struct nl80211_data* ctx, const void *data, size_t len, int noack)
{
    __u8 rtap_hdr[] = {
        0x00, 0x00, /* radiotap version */
        0x0e, 0x00, /* radiotap length */
        0x02, 0xc0, 0x00, 0x00, /* bmap: flags, tx and rx flags */
        IEEE80211_RADIOTAP_F_FRAG, /* F_FRAG (fragment if required) */
        0x00,       /* padding */
        0x00, 0x00, /* RX and TX flags to indicate that */
        0x00, 0x00, /* this is the injected frame directly */
    };

    struct iovec iov[2] = {
        {
            .iov_base = &rtap_hdr,
            .iov_len = sizeof(rtap_hdr),
        },
        {
            .iov_base = (void *) data,
            .iov_len = len,
        }
    };

    struct msghdr msg = {
        .msg_name = NULL,
        .msg_namelen = 0,
        .msg_iov = iov,
        .msg_iovlen = 2,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = 0,
    };
    int res;
    
    u16 txflags = 0;
    if (noack) txflags |= IEEE80211_RADIOTAP_F_TX_NOACK;
    WPA_PUT_LE16(&rtap_hdr[12], txflags);

    if (ctx->monitor_sock < 0) {
        fprintf(stderr, "nl80211: No monitor socket available for %s\n", __func__);
        return -1;
    }


    // DEBUG
    {
        struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *) data;
        u16 fc = le_to_host16(mgmt->frame_control);
        fprintf(stderr, "monitor: send_mlme - da=%02x:%02x:%02x:%02x:%02x:%02x noack=%d fc=0x%x (%s)\n",
               mgmt->da[0],mgmt->da[1],mgmt->da[2],mgmt->da[3],mgmt->da[4],mgmt->da[5],
               noack, fc, fc2str(fc));
    }

    res = sendmsg(ctx->monitor_sock, &msg, 0);
    if (res < 0) {
        fprintf(stderr, "nl80211: sendmsg: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}