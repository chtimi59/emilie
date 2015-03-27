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

#include "driver_nl80211_frag.h"
#include "ieee802_11_defs.h"


static u8 * hostapd_eid_ds_params(u8 *eid)
{
	*eid++ = WLAN_EID_DS_PARAMS;
	*eid++ = 1;
	*eid++ = 6;
	return eid;
}

u8 * hostapd_eid_supp_rates(u8 *eid)
{
	u8 *pos = eid;
	int i, num, count;

	*pos++ = WLAN_EID_SUPP_RATES;
	*pos++ = 8;

	*pos = 10 / 5; *pos |= 0x80; pos++;  // 1MB basic
	*pos = 20 / 5; *pos |= 0x80; pos++;  // 2MB basic
	*pos = 55 / 5; *pos |= 0x80; pos++;  // 5.5MB basic
	*pos = 110 / 5; *pos |= 0x80; pos++; // 11MB basic
	*pos = 60 / 5; pos++;  // 6MB supported
	*pos = 90 / 5; pos++;  // 9MB supported
	*pos = 120 / 5; pos++; // 120MB supported
	*pos = 180 / 5; pos++; // 180MB supported

	return pos;
}

int nl80211_set_ap(struct nl80211_data *ctx)
{
	struct nl_msg *msg = NULL;
	int ret;


	msg = nlmsg_alloc();
	if (!msg)
		goto fail;

	if (!nl80211_cmd(ctx, msg, 0, NL80211_CMD_NEW_BEACON))
		goto fail;
	
	{
		struct ieee80211_mgmt head;
		u8 *pos;

		head.frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT, WLAN_FC_STYPE_BEACON);
		head.duration = host_to_le16(0);
		memcpy(head.da, broadcast_ether_addr, ETH_ALEN);
		memcpy(head.sa, ctx->macaddr, ETH_ALEN);
		memcpy(head.bssid, ctx->macaddr, ETH_ALEN);
		head.seq_ctrl = host_to_le16(0);
		memset(head.u.beacon.timestamp, 0, sizeof(head.u.beacon.timestamp)); //set by PHY ?
		head.u.beacon.beacon_int = host_to_le16(100);
		head.u.beacon.capab_info = host_to_le16(0x401); // TODO: check capabilities

		pos = (u8*)(&head.u.beacon.variable[0]);

		/* SSID */
		#define SSID "test"
		int ssid_len = strlen(SSID);

		*pos++ = WLAN_EID_SSID;
		*pos++ = ssid_len;
		memcpy(pos, SSID, ssid_len);
		pos += ssid_len;

		/* Supported rates */
		pos = hostapd_eid_supp_rates(pos);

		/* DS Params */
		pos = hostapd_eid_ds_params(pos);

		size_t head_len = pos - (u8 *)&head;
		fhexdump(stderr, "nl80211: Beacon head", (const u8*)&head, head_len);


		if (nla_put(msg, NL80211_ATTR_BEACON_HEAD, head_len, &head))
			goto fail;
	}
	goto fail;


	#if 0
	{
		struct ieee80211_mgmt head;
		u8 *pos;

		if (nla_put(msg, NL80211_ATTR_BEACON_TAIL, tail_len, tail))
			goto fail;
	}

	nl80211_put_beacon_int(msg, params->beacon_int) ||
		nla_put_u32(msg, NL80211_ATTR_DTIM_PERIOD, params->dtim_period) ||
		nla_put(msg, NL80211_ATTR_SSID, params->ssid_len, params->ssid))
		goto fail;

	if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, ctx->ifindex)) {
		nlmsg_free(msg);
		return -1;
	}

	if (nla_put_u32(msg, NL80211_ATTR_WIPHY_FRAG_THRESHOLD, val)) {
		nlmsg_free(msg);
		return -1;
	}

	ret = send_and_recv_msgs(ctx, msg, NULL, NULL);
	if (ret != 0) {
		fprintf(stderr, "nl80211: Failed to set fragmentation: %d (%s)\n", ret, strerror(-ret));
		return -1;
	}
#endif

	return 0;

fail:
	if (msg) nlmsg_free(msg);
	return -1;
}

