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




