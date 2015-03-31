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



int send_auth(struct nl80211_data *ctx, const u8 *addr) {
    struct ieee80211_mgmt mgmt;
    memset(&mgmt, 0, sizeof(mgmt));
    mgmt.frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,WLAN_FC_STYPE_AUTH);
    memcpy(mgmt.da, addr, ETH_ALEN);
    memcpy(mgmt.sa, ctx->macaddr, ETH_ALEN);
    memcpy(mgmt.bssid, ctx->macaddr, ETH_ALEN);
    
    mgmt.u.auth.auth_alg         = host_to_le16(WLAN_AUTH_OPEN);
    mgmt.u.auth.auth_transaction = host_to_le16(2);
    mgmt.u.auth.status_code      = host_to_le16(WLAN_STATUS_SUCCESS);

    return monitor_tx(ctx, (u8 *) &mgmt, IEEE80211_HDRLEN + sizeof(mgmt.u.auth), 0);
}

int send_asso_resp(struct nl80211_data *ctx, const u8 *addr, int* commonrates) {
    struct ieee80211_mgmt mgmt;
    memset(&mgmt, 0, sizeof(mgmt));
    mgmt.frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,WLAN_FC_STYPE_ASSOC_RESP);
    memcpy(mgmt.da, addr, ETH_ALEN);
    memcpy(mgmt.sa, ctx->macaddr, ETH_ALEN);
    memcpy(mgmt.bssid, ctx->macaddr, ETH_ALEN);

    mgmt.u.assoc_resp.capab_info   = host_to_le16(WLAN_AUTH_OPEN);
    mgmt.u.assoc_resp.status_code  = host_to_le16(2);
    mgmt.u.assoc_resp.aid          = host_to_le16(WLAN_STATUS_SUCCESS);
    //mgmt.u.assoc_resp.variable[0]  = ;

    return monitor_tx(ctx, (u8 *) &mgmt, IEEE80211_HDRLEN + sizeof(mgmt.u.assoc_resp), 0);
}


int send_deauth(struct nl80211_data *ctx, const u8 *addr, int reason) {
    struct ieee80211_mgmt mgmt;
    memset(&mgmt, 0, sizeof(mgmt));
    mgmt.frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,WLAN_FC_STYPE_DEAUTH);
    memcpy(mgmt.da, addr, ETH_ALEN);
    memcpy(mgmt.sa, ctx->macaddr, ETH_ALEN);
    memcpy(mgmt.bssid, ctx->macaddr, ETH_ALEN);
    mgmt.u.deauth.reason_code = host_to_le16(reason);
    return monitor_tx(ctx, (u8 *) &mgmt, IEEE80211_HDRLEN + sizeof(mgmt.u.deauth), 0);
}




static char* hostapd_eid_ssid(u8** buff) {
    int i,j=0;
    char ssid[255] = { 0 };
    u8* pos = *buff;
    u8 eid = *pos; pos++;
    u8 len = *pos; pos++;
    if (eid!=WLAN_EID_SSID) return ssid;
    for(i=0; i<len; i++) {
        if (j<253) ssid[j++] = pos[i];
    }
    ssid[j] = '\0';
    *buff = pos;
    return (char*)ssid;
}

struct rates_t {
    int value;
    int isBasic;
}
static struct rates_t* hostapd_eid_supp_rates(u8** buff, int* out_sz) {
    int i,j=0;
    int rates[255] = { 0 };
    u8* pos = *buff;
    u8 eid = *pos; pos++;
    u8 len = *pos; pos++;
    if (eid!=WLAN_EID_SUPP_RATES) { return (int*)rates; *out_sz=j; }
    for(i=0; i<len; i++) {
        if (j<255) {
            rates[j++] = pos[i];
        }
    }
    
    *buff = pos;
    *out_sz=j;
    return (int*)rates;

    for (j = 0; j<nbBascis; j++) p->d[i++] = BASIC_RATE(BASIC_RATES[j]);
    for (j = 0; j<nbSuppor; j++) p->d[i++] = SUPPORTED_RATE(SUPPORTED_RATES[j]);
    //fhexdump(stderr, "nl80211: Beacon rates", p->d, p->len);
    return p->len;
}


void mngt_rx_handle(struct nl80211_data* ctx, u8 *buf, size_t len, int datarate, int ssi_signal)
{
    u16 fc;
    struct ieee80211_mgmt* mgmt = (struct ieee80211_mgmt*)buf;
    fc = le_to_host16(mgmt->frame_control);
    u16 stype = WLAN_FC_GET_STYPE(fc);

    switch (stype)
    {
       case WLAN_FC_STYPE_AUTH:
            fprintf(stderr, "AUTH from ");
            fprintf(stderr, "%02x:%02x:%02x:%02x:%02x:%02x\n", mgmt->sa[0], mgmt->sa[1], mgmt->sa[2], mgmt->sa[3], mgmt->sa[4], mgmt->sa[5]);
            send_auth(ctx, mgmt->sa);
            break;

        case WLAN_FC_STYPE_DEAUTH:
            fprintf(stderr, "DEAUTH from ");
            fprintf(stderr, "%02x:%02x:%02x:%02x:%02x:%02x\n", mgmt->sa[0], mgmt->sa[1], mgmt->sa[2], mgmt->sa[3], mgmt->sa[4], mgmt->sa[5]);
            break;

        case WLAN_FC_STYPE_ASSOC_REQ:
            fprintf(stderr, "ASSOC_REQ from ");
            fprintf(stderr, "%02x:%02x:%02x:%02x:%02x:%02x\n", mgmt->sa[0], mgmt->sa[1], mgmt->sa[2], mgmt->sa[3], mgmt->sa[4], mgmt->sa[5]);
            if (len<IEEE80211_HDRLEN + sizeof(mgmt->u.assoc_req)) {
                fprintf(stderr,"malformed packet\n");
                return;
            } else {
                u16 capainfo = le_to_host16(mgmt->u.assoc_req.capab_info);
                u16 listen_interval = le_to_host16(mgmt->u.assoc_req.listen_interval);
                u8* sta_varia = mgmt->u.assoc_req.variable;
                char* ssid = hostapd_eid_ssid(&sta_varia);
                int*  rates = hostapd_eid_ssid(&sta_varia);
static int BASIC_RATES[4] = { 10, 20, 55, 110 };
//array of suppported rates in 100 kbps (6MB, 9MB, 12MB, 18MB)
static int SUPPORTED_RATES[4] = { 60, 90, 120, 180 };
//array of suppported rates in 100 kbps (24MB, 36MB, 48MB, 54MB)
static int EXT_RATES[4] = { 240, 360, 480, 540};
                fprintf(stderr, "capa:0x%04X listen_interval:%d ssid:'%s'\n", capainfo, listen_interval, ssid);

                //send_asso_resp(ctx, hdr->addr2);
            }

            break;



        case WLAN_FC_STYPE_ASSOC_RESP:
            break;
        case WLAN_FC_STYPE_REASSOC_REQ:
            break;
        case WLAN_FC_STYPE_REASSOC_RESP:
            break;
        case WLAN_FC_STYPE_PROBE_REQ:
            fprintf(stderr, "PROBE_REQ\n");
            break;
        case WLAN_FC_STYPE_PROBE_RESP:
            break;
        case WLAN_FC_STYPE_BEACON:
            break;
        case WLAN_FC_STYPE_ATIM:
            break;
        case WLAN_FC_STYPE_DISASSOC:
            break;

        case WLAN_FC_STYPE_ACTION:
            break;
    }
    return;
}
