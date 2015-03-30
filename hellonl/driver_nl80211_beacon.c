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



typedef struct {
	u8* d;
	u8 len;
} packet_element_t;

inline u8* packet_element_concatnfree(u8* pos, packet_element_t pkt) {
	memcpy((void*)pos, (void*)pkt.d, pkt.len);
	free(pkt.d);
	pos += pkt.len;
	return pos;
}



static int hostapd_header_beacon(packet_element_t* p, u8* macaddr, u16 beacon_period)
{
	struct ieee80211_mgmt* head=NULL;
	p->len = (u8)&(head->u.beacon.variable[0]);
	head = (struct ieee80211_mgmt*)malloc(p->len);

	// FRAME CONTROL (2) : PROTOCOL=0 + TYPE=MNGT + SUBTYPE=BEACON, FROMDS, TODS....
	head->frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT, WLAN_FC_STYPE_BEACON);
	// FRAME DURATION (2) : 0 (typic for broadcast/multicast)
	head->duration = host_to_le16(0);
	// ADDR1 (6) = DA ( destination = broadcast)
	memcpy(head->da, broadcast_ether_addr, ETH_ALEN);
	// ADDR2 (6) = SA ( source = me)
	memcpy(head->sa, macaddr, ETH_ALEN);
	// ADDR3 (6) = BSSID ( me)
	memcpy(head->bssid, macaddr, ETH_ALEN);
	// SEQUENCE-CTRL (2)
	head->seq_ctrl = host_to_le16(0);

	// FRAME BODY FIXED ELEMENTS FOR BEACON
	// BEACON TIMESTAMP (8)
	memset(head->u.beacon.timestamp, 0, sizeof(head->u.beacon.timestamp)); //set by PHY ?
	// BEACON INTERVAL (2)
	head->u.beacon.beacon_int = host_to_le16(beacon_period);
	// BEACON CAPA (2)
	head->u.beacon.capab_info = host_to_le16(0x401); // TODO: check capabilities
	p->d = (u8*)head;
	return p->len;
}

static int hostapd_eid_ssid(packet_element_t* p, const char* ssid) {
	u8 len = strlen(ssid);
	p->len += len + 2;
	p->d = malloc(p->len);
	p->d[0] = WLAN_EID_SSID;
	p->d[1] = len;
	memcpy(&(p->d[2]), ssid, len);
	//fhexdump(stderr, "nl80211: Beacon SSID", p->d, p->len);
	return p->len;
}

inline u8 SUPPORTED_RATE(float rate) { return ((u8)(rate * 10)) / 5; }
inline u8 BASIC_RATE(float rate) { u8 r = SUPPORTED_RATE(rate); r |= 0x80; return r; }
static int hostapd_eid_supp_rates(packet_element_t* p) {
	u8 len = 8;
	p->len += len + 2;
	p->d = malloc(p->len);
	p->d[0] = WLAN_EID_SUPP_RATES;
	p->d[1] = len;
	p->d[2] = BASIC_RATE(1);   // 1MB basic
	p->d[3] = BASIC_RATE(2);   // 2MB basic
	p->d[4] = BASIC_RATE(5.5); // 5.5MB basic
	p->d[5] = BASIC_RATE(11);  // 11MB basic
	p->d[6] = SUPPORTED_RATE(6);  // 6MB supported
	p->d[7] = SUPPORTED_RATE(9);  // 9MB supported
	p->d[8] = SUPPORTED_RATE(12); // 120MB supported
	p->d[9] = SUPPORTED_RATE(18); // 180MB supported
	//fhexdump(stderr, "nl80211: Beacon Rates", p->d, p->len);
	return p->len;
}

static int hostapd_eid_ds_params(packet_element_t* p) {
	u8 len = 1;
	p->len += len + 2;
	p->d = malloc(p->len);
	p->d[0] = WLAN_EID_DS_PARAMS;
	p->d[1] = len;
	p->d[2] = 6;
	//fhexdump(stderr, "nl80211: Beacon DS-PARAM", p->d, p->len);
	return p->len;
}

static int hostapd_eid_emilie(packet_element_t* p) {
	u8 len = 1;
	p->len += len + 2;
	p->d = malloc(p->len);
	p->d[0] = WLAN_EID_VENDOR_SPECIFIC;
	p->d[1] = len;
	p->d[2] = 0xFF;
	//fhexdump(stderr, "nl80211: Beacon DS-PARAM", p->d, p->len);
	return p->len;
}




int nl80211_set_ap(struct nl80211_data *ctx)
{
	struct nl_msg *msg = NULL;
	int ret;

	u16   beacon_period = 100;
	u8    beacon_DTIM = 1;
	char* ssid_name = "TEST";
	u8    ssid_sz = strlen(ssid_name);

	//portion of the beacon before the TIM IE
	u8 * head = NULL;
	int  head_sz = 0;
	
	//portion of the beacon after the TIM IE
	u8 * tail = NULL;
	int  tail_sz = 0;

	{
		u8 * pos = NULL;		
		packet_element_t macheader = { 0 };
		packet_element_t ssid = { 0 };
		packet_element_t rates = { 0 };
		packet_element_t ds = { 0 };

		head_sz += hostapd_header_beacon(&macheader, ctx->macaddr, beacon_period);
		head_sz += hostapd_eid_ssid(&ssid, ssid_name);
		head_sz += hostapd_eid_supp_rates(&rates);
		head_sz += hostapd_eid_ds_params(&ds);

		head = (u8*)malloc(head_sz); pos = head;
		pos = packet_element_concatnfree(pos, macheader);
		pos = packet_element_concatnfree(pos, ssid);
		pos = packet_element_concatnfree(pos, rates);
		pos = packet_element_concatnfree(pos, ds);

		//fprintf(stderr, "expected Beacon head          (len=55): 80 00 00 00 ff ff ff ff ff ff e8 4e 06 22 a6 82 e8 4e 06 22 a6 82 00 00 00 00 00 00 00 00 00 00 64 00 01 04 00 04 54 45 53 54 01 08 82 84 8b 96 0c 12 18 24 03 01 06\n");		
		fhexdump(stderr, "nl80211: Beacon head", head, head_sz);		
	}

	{
		u8 * pos = NULL;
		packet_element_t emilie = { 0 }; // l
		tail_sz += hostapd_eid_emilie(&emilie);

		tail = (u8*)malloc(tail_sz); pos = tail;
		pos = packet_element_concatnfree(pos, emilie);

		fhexdump(stderr, "nl80211: Beacon tail", tail, tail_sz);
	}

	// NL80211 BEACON SETTING
	msg = nlmsg_alloc();
	if (!msg) goto fail;
	if (!nl80211_cmd(ctx, msg, 0, NL80211_CMD_SET_BSS)) goto fail;
	if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, ctx->ifindex)) goto fail;
	if (nla_put(msg, NL80211_ATTR_BEACON_HEAD, head_sz, head)) goto fail;
	if (nla_put(msg, NL80211_ATTR_BEACON_TAIL, tail_sz, tail)) goto fail;
	if (nla_put_u32(msg, NL80211_ATTR_BEACON_INTERVAL, beacon_period)) goto fail;
	if (nla_put_u32(msg, NL80211_ATTR_DTIM_PERIOD, beacon_DTIM)) goto fail;	
	if (nla_put(msg, NL80211_ATTR_SSID, ssid_sz, ssid_name)) goto fail;
	if (nla_put_u32(msg, NL80211_ATTR_HIDDEN_SSID, NL80211_HIDDEN_SSID_NOT_IN_USE)) goto fail;
	if (nla_put_flag(msg, NL80211_ATTR_PRIVACY)) goto fail;
	if (nla_put_u32(msg, NL80211_ATTR_AUTH_TYPE, NL80211_AUTHTYPE_OPEN_SYSTEM)) goto fail;
	if (nla_put_u32(msg, NL80211_ATTR_SMPS_MODE, NL80211_SMPS_OFF)) goto fail;
	ret = send_and_recv_msgs(ctx, msg, NULL, NULL);
	if (ret) {
		fprintf(stderr, "nl80211: Beacon set failed: %d (%s)\n", ret, strerror(-ret));
		goto fail;
	}

#if 0
	// NL80211 BSS ADVDANCED SETTING
	msg = nlmsg_alloc();
	if (!msg)
		goto fail;
	if (!nl80211_cmd(ctx, msg, 0, NL80211_CMD_SET_BSS))
		goto fail;
	if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, ctx->ifindex))
		goto fail;
	if (nla_put_u8(msg, NL80211_ATTR_BSS_CTS_PROT, cts))
		goto fail;
	if (nla_put_u8(msg, NL80211_ATTR_BSS_SHORT_PREAMBLE, preamble))
		goto fail;
	if (nla_put_u8(msg, NL80211_ATTR_BSS_SHORT_SLOT_TIME, slot))
		goto fail;
	if (nla_put_u16(msg, NL80211_ATTR_BSS_HT_OPMODE, ht_opmode))
		goto fail;
	if (nla_put_u8(msg, NL80211_ATTR_AP_ISOLATE, ap_isolate))
		goto fail;
	if (nl80211_put_basic_rates(msg, basic_rates))
		goto fail;
	ret = send_and_recv_msgs(ctx, msg, NULL, NULL);
	if (ret) {
		fprintf(stderr, "nl80211: advanced settings failed: %d (%s)\n", ret, strerror(-ret));
		goto fail;
	}
#endif


	if (head) free(head);
	if (tail) free(tail);
	return 0;
fail:
	if (msg) nlmsg_free(msg);
	if (head) free(head);
	if (tail) free(tail);
	return -1;
}

