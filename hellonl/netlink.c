/*
* Netlink helper functions for driver wrappers
* Copyright (c) 2002-2014, Jouni Malinen <j@w1.fi>
*
* This software may be distributed under the terms of the BSD license.
* See README for more details.
*/
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <net/if.h>

#include "common.h"
#include "eloop.h"
//#include "priv_netlink.h"
#include "netlink.h"






static void netlink_receive_link(struct netlink_data *netlink, void(*cb)(void *ctx, struct ifinfomsg *ifi, u8 *buf, size_t len), struct nlmsghdr *h) {
    if (cb == NULL || NLMSG_PAYLOAD(h, 0) < sizeof(struct ifinfomsg)) return;
    cb(netlink->cfg->ctx, NLMSG_DATA(h), (u8 *)NLMSG_DATA(h) + NLMSG_ALIGN(sizeof(struct ifinfomsg)), NLMSG_PAYLOAD(h, sizeof(struct ifinfomsg)));
}

static void netlink_receive(int sock, void *eloop_ctx, void *sock_ctx)
{
    struct netlink_data *netlink = eloop_ctx;
    char buf[8192];
    int left;
    struct sockaddr_nl from;
    socklen_t fromlen;
    struct nlmsghdr *h;
    int max_events = 10;

try_again:
    fromlen = sizeof(from);
    left = recvfrom(sock, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr *) &from, &fromlen);
    if (left < 0) {
        if (errno != EINTR && errno != EAGAIN)
            fprintf(stderr, "netlink: recvfrom failed: %s", strerror(errno));
        return;
    }

    h = (struct nlmsghdr *) buf;
    while (NLMSG_OK(h, left)) {
        switch (h->nlmsg_type) {
            
            // NETLINK_ROUTE PROTOCOL, FAMILY LINK, NEW COMMAND
            case RTM_NEWLINK:
                netlink_receive_link(netlink, netlink->cfg->newlink_cb, h);
                break;

            // NETLINK_ROUTE PROTOCOL, FAMILY LINK, DEL COMMAND
            case RTM_DELLINK:
                netlink_receive_link(netlink, netlink->cfg->dellink_cb, h);
                break;

            // NETLINK_ROUTE PROTOCOL, FAMILY LINK, GET COMMAND
            case RTM_GETLINK:
                break;
        }

        h = NLMSG_NEXT(h, left);
    }

    if (left > 0) {
        fprintf(stderr, "netlink: %d extra bytes in the end of netlink message", left);
    }

    if (--max_events > 0) {
        /*
        * Try to receive all events in one eloop call in order to
        * limit race condition on cases where AssocInfo event, Assoc
        * event, and EAPOL frames are received more or less at the
        * same time. We want to process the event messages first
        * before starting EAPOL processing.
        */
        goto try_again;
    }
}

struct netlink_data * netlink_init(struct netlink_config *cfg)
{
    struct netlink_data *netlink;
    struct sockaddr_nl local;

    netlink = zalloc(sizeof(*netlink));
    if (netlink == NULL)
        return NULL;

    
    /* NETLINK_ROUTE PROTOCOL 
    The messages that make up the NETLINK ROUTE protocol can be divided
    into families, each of which control a specific aspect of the Linux kernels
    network routing system.

    Each family consists of three methods; a NEW method, a DEL method, and a GET method.

        LINKS : RTM_NEWLINK, RTM_DELLINK, RTM_SETLINK, RTM_GETLINK
        ADDRESSES (not used here)
        ROUTES (not used here)
        NEIGHBORS (not used here)
        RULES (not used here)
        DISCIPLINES (not used here)
        CLASSES (not used here)
        FILTERS (not used here)

    As each family controls a set of data objects that can be represented as a table,
    these methods allow a user to create table entries, delete table entries, and retrieve one or more of the objects in the table.

    */

    netlink->sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (netlink->sock < 0) {
        fprintf(stderr, "netlink: Failed to open netlink socket: %s", strerror(errno));
        netlink_deinit(netlink);
        return NULL;
    }

    memset(&local, 0, sizeof(local));

    /* This field defines the address family of the message being
        sent over the socket. This should always be set to AF NETLINK. */
    local.nl_family = AF_NETLINK;

    /* This field is unused and should always be set to zero. */
    local.nl_pad = 0;

    /* This field is PID of the process that should receive the frame
       being sent, or the process which sent the frame being received. Set this
       value to the PID of the process you wish to recieve the frame, or to
       zero for a multicast message or to have the kernel process the message.
    */
    local.nl_pid = 0;

    /* This field is used for sending multicast messages over
        netlink. Each netlink protocol family has 32 multicast groups which
        processes can send and receive messages on. To subscribe to a particular
        group, the bind library call is used, and the nl groups field is set to
        the appropriate bitmask. Sending multicast frames works in a simmilar
        fashion, by setting the nl groups field to an appropriate set of values
        when calling sendto or sendmsg. Each protocol uses the multicast
        groups differently, if at all, and their use is defined by convention.
     */
    local.nl_groups = RTMGRP_LINK;
    
    
    if (bind(netlink->sock, (struct sockaddr *) &local, sizeof(local)) < 0)
    {
        fprintf(stderr, "netlink: Failed to bind netlink socket: %s", strerror(errno));
        netlink_deinit(netlink);
        return NULL;
    }

    eloop_register_read_sock(netlink->sock, netlink_receive, netlink, NULL);

    netlink->cfg = cfg;
    
    return netlink;
}


void netlink_deinit(struct netlink_data *netlink)
{
    if (netlink == NULL)
        return;
    if (netlink->sock >= 0) {
        eloop_unregister_read_sock(netlink->sock);
        close(netlink->sock);
    }
    free(netlink);
}





// SEND...
static const char * linkmode_str(int mode)
{
    switch (mode) {
    case -1:
        return "no change";
    case 0:
        return "kernel-control";
    case 1:
        return "userspace-control";
    }
    return "?";
}


static const char * operstate_str(int state)
{
    switch (state) {
    case -1:
        return "no change";
    case IF_OPER_DORMANT:
        return "IF_OPER_DORMANT";
    case IF_OPER_UP:
        return "IF_OPER_UP";
    }
    return "?";
}

int netlink_send_oper_ifla(struct netlink_data *netlink, int ifindex, int linkmode, int operstate)
{
    struct {
        struct nlmsghdr hdr; // The netlink header
        struct ifinfomsg ifinfo;
        char opts[16];
    } req;

    static int nl_seq;
    ssize_t ret;

    memset(&req, 0, sizeof(req));

    // [[ NETLINK HEADER ]]

    /* nlmsg len - Each netlink message header is followed by zero or more bytes of ancilliary data.
       This 4 byte field records the total amount of data in the message, including the header itself. */
    req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));

    /* This 2 byte field defines the format of the data which follows the netlink message header,
      here RTM_SETLINK means  
          for NETLINK_ROUTE protocol
          use LINK familiy
          and SET an interface (not NEW)
    */
    req.hdr.nlmsg_type = RTM_SETLINK;

    /* This 2 byte field or logically OR’ed bits defines various control flags which determine who each message is processed and
       interpreted: ex: NLM_F_REQUEST (most common request), or NLM_F_ACK (an ack)/NLM_F_ECHO (the packet is going to be echoed)  */
    req.hdr.nlmsg_flags = NLM_F_REQUEST;

    /* This 4 byte field is an arbitrary number, and is used by processes that create netlink request messages to correlate those
       requests with thier responses. */
    req.hdr.nlmsg_seq = ++nl_seq;
    req.hdr.nlmsg_pid = 0;


    // [[ NETLINK PAYLOAD FOR RTM_SETLINK, which is network interfaces structure ]]

    /* The address family that this interface belongs to. For interfaces with ipv6 addresses associated this field is AF INET6,
       otherwise it is set to AF UNSPEC */
    req.ifinfo.ifi_family = AF_UNSPEC;
    /* The media type of the interface. Usually set to ARPHRD_ETHER */
    req.ifinfo.ifi_type = 0;
    /* The unique interface number associated with this interface. 
       Note that this number is simply a unique identifier number associated
       with a interface, and is in no way related to the interface name */
    req.ifinfo.ifi_index = ifindex;
    /* The interface flags */
    req.ifinfo.ifi_flags = 0;
    /* Reserved field. Always set this is 0xfffffff */
    req.ifinfo.ifi_change = 0;

    /* The ifinfo data structure may be followed by zero or more attributes, each
       lead by an rtattr structure */
    {
        struct rtattr *rta;

        /* Linux distinguishes between administrative and operational state of an interface.
           Administrative state is the result of "ip link set dev <dev> up or down" and reflects whether
           the administrator wants to use the device for traffic.

           However, an interface is not usable just because the admin enabled it.
           Example: Ethernet requires to be plugged into the switch to be UP

           Both admin and operational state can be queried via the netlink
           operation RTM_GETLINK. It is also possible to subscribe to RTMGRP_LINK
           to be notified of updates. This is important for setting from userspace.
        */

        // IFLA_LINKMODE contains link policy. 
        if (linkmode != -1) {
            rta = aliasing_hide_typecast(((char *)&req + NLMSG_ALIGN(req.hdr.nlmsg_len)), struct rtattr);
            rta->rta_type = IFLA_LINKMODE;
            rta->rta_len = RTA_LENGTH(sizeof(char));
            *((char *)RTA_DATA(rta)) = linkmode;
            req.hdr.nlmsg_len += RTA_SPACE(sizeof(char));
        }

        if (operstate != -1) {
            rta = aliasing_hide_typecast(((char *)&req + NLMSG_ALIGN(req.hdr.nlmsg_len)), struct rtattr);
            rta->rta_type = IFLA_OPERSTATE;
            rta->rta_len = RTA_LENGTH(sizeof(char));
            *((char *)RTA_DATA(rta)) = operstate;
            req.hdr.nlmsg_len += RTA_SPACE(sizeof(char));
        }
    }


    fprintf(stderr, "netlink: Operstate: ifindex=%d linkmode=%d (%s), operstate=%d (%s)\n",
            ifindex, linkmode, linkmode_str(linkmode),
            operstate, operstate_str(operstate));



    ret = send(netlink->sock, &req, req.hdr.nlmsg_len, 0);
    if (ret < 0) {
        fprintf(stderr, "netlink: Sending operstate IFLA failed: %s (assume operstate is not supported)\n", strerror(errno));
    }    
    return ret < 0 ? -1 : 0;
}



void debugPrint(struct ifinfomsg *ifi, u8 *buf, size_t len)
{
    // NETLINK ROUTE message print */
    int attrlen = len;
    struct rtattr *attr = (struct rtattr *) buf;

    char ifname[IFNAMSIZ + 1];
    u32 brid = 0;
    char extra[100], *pos, *end; 

    extra[0] = '\0';
    pos = extra;
    end = pos + sizeof(extra);
    ifname[0] = '\0';

    while (RTA_OK(attr, attrlen))
    {
        switch (attr->rta_type) {
        case IFLA_IFNAME:
            if (RTA_PAYLOAD(attr) >= IFNAMSIZ) break;
            memcpy(ifname, RTA_DATA(attr), RTA_PAYLOAD(attr));
            ifname[RTA_PAYLOAD(attr)] = '\0';
            break;
        case IFLA_MASTER:
            brid = nla_get_u32((struct nlattr *) attr);
            pos += snprintf(pos, end - pos, " master=%u", brid);
            break;
        case IFLA_WIRELESS:
            pos += snprintf(pos, end - pos, " wext");
            break;
        case IFLA_OPERSTATE:
            /* contains RFC2863 state of the interface in numeric representation:
                IF_OPER_UNKNOWN (0):
                 Interface is in unknown state, neither driver nor userspace has set
                 operational state. Interface must be considered for user data as
                 setting operational state has not been implemented in every driver.
                IF_OPER_NOTPRESENT (1):
                 Unused in current kernel (notpresent interfaces normally disappear),
                 just a numerical placeholder.
                IF_OPER_DOWN (2):
                 Interface is unable to transfer data on L1, f.e. ethernet is not
                 plugged or interface is ADMIN down.
                IF_OPER_LOWERLAYERDOWN (3):
                 Interfaces stacked on an interface that is IF_OPER_DOWN show this
                 state (f.e. VLAN).
                IF_OPER_TESTING (4):
                 Unused in current kernel.
                IF_OPER_DORMANT (5):
                 Interface is L1 up, but waiting for an external event, f.e. for a
                 protocol to establish. (802.1X)
                IF_OPER_UP (6):
                 Interface is operational up and can be used.
            */
            pos += snprintf(pos, end - pos, " operstate=%u", nla_get_u32((struct nlattr *) attr));
            break;
        case IFLA_LINKMODE:
            pos += snprintf(pos, end - pos, " linkmode=%u", nla_get_u32((struct nlattr *) attr));
            break;
        }
        attr = RTA_NEXT(attr, attrlen);
    }

    extra[sizeof(extra) - 1] = '\0';
    fprintf(stderr, "RTM_NEWLINK: ifi_index=%d ifname=%s%s ifi_family=%d ifi_flags=0x%x (%s%s%s%s)\n",
            ifi->ifi_index, ifname, extra, ifi->ifi_family,
            ifi->ifi_flags,
            (ifi->ifi_flags & IFF_UP) ? "[UP]" : "",  // Interface is admin up
            (ifi->ifi_flags & IFF_RUNNING) ? "[RUNNING]" : "", /* Interface is in RFC2863 operational state UP */
            (ifi->ifi_flags & IFF_LOWER_UP) ? "[LOWER_UP]" : "",  /* Driver has signaled netif_carrier_on() */
            (ifi->ifi_flags & IFF_DORMANT) ? "[DORMANT]" : ""); /* Driver has signaled netif_dormant_on() */
}