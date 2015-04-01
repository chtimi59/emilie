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




static int hostapd_eid_ssid(u8* buff, char* ssid, int max) {
    int i,j=0;
    u8 eid = buff[0];
    u8 len = buff[1];
    if (WLAN_EID_SSID!=eid) return 0;
    for(i=0; i<len; i++) {
        if (j<(max-1)) ssid[j++] = buff[i+2];
    }
    ssid[j] = '\0';
    return j;
}


typedef struct {
    int value;
    int isBasic;
} rates_t;

static int hostapd_eid_supp_rates(u8* buff, rates_t* rates, int max) {
    int i,j=0;
    u8 eid = buff[0];
    u8 len = buff[1];
    if (WLAN_EID_SUPP_RATES!=eid) return 0;
    for(i=0; i<len; i++) {
        if (j<max) {
            u8 rate =  buff[i+2];
            if (rate & 0x80) {
                rates[j].isBasic = 1;
                rate &= 0x7F;
            }
            rates[j].value = rate * 5;
            j++;
        }
    }
    return j;
}

static int hostapd_eid_ext_rates(u8* buff, rates_t* rates, int max) {
      int i,j=0;
    u8 eid = buff[0];
    u8 len = buff[1];
    if (WLAN_EID_EXT_SUPP_RATES!=eid) return 0;
    for(i=0; i<len; i++) {
        if (j<max) {
            u8 rate =  buff[i+2];
            if (rate & 0x80) {
                rates[j].isBasic = 1;
                rate &= 0x7F;
            }
            rates[j].value = rate * 5;
            j++;
        }
    }
    return j;
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
            fprintf(stderr, "%d %d\n", IEEE80211_HDRLEN, sizeof(mgmt->u.assoc_req));
            if (len<IEEE80211_HDRLEN + sizeof(mgmt->u.assoc_req)) {
                fprintf(stderr,"malformed packet\n");
                return;
            } else {
                u16 capainfo = le_to_host16(mgmt->u.assoc_req.capab_info);
                u16 listen_interval = le_to_host16(mgmt->u.assoc_req.listen_interval);
                u8* sta_varia = mgmt->u.assoc_req.variable;
                int nbrates = 0;
                char ssid[255];
                rates_t rates[255];
                fprintf(stderr,"len = %d\n", len);
                int remain = len - IEEE80211_HDRLEN + sizeof(mgmt->u.assoc_req) /* don't take variable in account */  ;
                fprintf(stderr,"remain = %d\n", remain);
                do {
                   u8 eid              = sta_varia[0];
                   u8 eid_payload_len  = sta_varia[1];
                   u8 eidlen           = eid_payload_len+2;

                   fprintf(stderr,"%02X (%d) (%d/%d)\n", eid, eid,eid_payload_len,remain);
                   switch(eid) {
                        case WLAN_EID_SSID:           hostapd_eid_ssid(sta_varia, ssid, 255); break;
                        case WLAN_EID_SUPP_RATES:     nbrates = hostapd_eid_supp_rates(sta_varia, &rates[nbrates], 255); break;
                        case WLAN_EID_EXT_SUPP_RATES: nbrates = hostapd_eid_ext_rates(sta_varia, &rates[nbrates], 255-nbrates); break;
                   }

                   remain -= eidlen;
                   fprintf(stderr,"remain = %d\n", remain);
                   sta_varia = &sta_varia[eidlen];
                } while(remain>0);

                
                fprintf(stderr, "capa:0x%04X listen_interval:%d ssid:'%s' rates:%d\n", capainfo, listen_interval, ssid, nbrates);
                
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
