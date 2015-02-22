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

#include "driver_nl80211_capa.h"

static int protocol_feature_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	u32 *feat = arg;
	if (tb_msg[NL80211_ATTR_PROTOCOL_FEATURES]) {
		*feat = nla_get_u32(tb_msg[NL80211_ATTR_PROTOCOL_FEATURES]);
	}

	return NL_SKIP;
}

static u32 get_nl80211_protocol_features(struct nl80211_data *drv)
{
	u32 feat = 0;
	struct nl_msg *msg;

	msg = nlmsg_alloc();
	if (!msg)
		return 0;

	if (!nl80211_cmd(drv, msg, 0, NL80211_CMD_GET_PROTOCOL_FEATURES)) {
		nlmsg_free(msg);
		return 0;
	}

	if (send_and_recv_msgs(drv, msg, protocol_feature_handler, &feat) == 0)
		return feat;

	return 0;
}




static int wiphy_info_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);


	struct wiphy_info_data *info = arg;
	
	if (tb[NL80211_ATTR_WIPHY_NAME]) {
		strlcpy(info->phyname, nla_get_string(tb[NL80211_ATTR_WIPHY_NAME]), sizeof(info->phyname));
	}


	#if 0
	if (tb[NL80211_ATTR_MAX_NUM_SCAN_SSIDS])
		info->capa.max_scan_ssids = nla_get_u8(tb[NL80211_ATTR_MAX_NUM_SCAN_SSIDS]);

	if (tb[NL80211_ATTR_MAX_NUM_SCHED_SCAN_SSIDS])
		info->capa.max_sched_scan_ssids = nla_get_u8(tb[NL80211_ATTR_MAX_NUM_SCHED_SCAN_SSIDS]);

	if (tb[NL80211_ATTR_MAX_MATCH_SETS])
		info->capa.max_match_sets = nla_get_u8(tb[NL80211_ATTR_MAX_MATCH_SETS]);

	if (tb[NL80211_ATTR_MAC_ACL_MAX])
		info->capa.max_acl_mac_addrs = nla_get_u8(tb[NL80211_ATTR_MAC_ACL_MAX]);

	/*
	wiphy_info_supported_iftypes(info, tb[NL80211_ATTR_SUPPORTED_IFTYPES]);
	wiphy_info_iface_comb(info, tb[NL80211_ATTR_INTERFACE_COMBINATIONS]);
	wiphy_info_supp_cmds(info, tb[NL80211_ATTR_SUPPORTED_COMMANDS]);
	wiphy_info_cipher_suites(info, tb[NL80211_ATTR_CIPHER_SUITES]);
	*/

	if (tb[NL80211_ATTR_OFFCHANNEL_TX_OK]) {
		fprintf(stderr, "nl80211: Using driver-based off-channel TX\n");
		info->capa.flags |= WPA_DRIVER_FLAGS_OFFCHANNEL_TX;
	}

	if (tb[NL80211_ATTR_ROAM_SUPPORT]) {
		fprintf(stderr, "nl80211: Using driver-based roaming\n");
		info->capa.flags |= WPA_DRIVER_FLAGS_BSS_SELECTION;
	}

	/*
	wiphy_info_max_roc(capa, tb[NL80211_ATTR_MAX_REMAIN_ON_CHANNEL_DURATION]);
	*/

	if (tb[NL80211_ATTR_SUPPORT_AP_UAPSD])
		info->capa.flags |= WPA_DRIVER_FLAGS_AP_UAPSD;

	/*
	wiphy_info_tdls(capa, tb[NL80211_ATTR_TDLS_SUPPORT],
		tb[NL80211_ATTR_TDLS_EXTERNAL_SETUP]);
	*/

	if (tb[NL80211_ATTR_DEVICE_AP_SME])
		info->device_ap_sme = 1;
	/*
	wiphy_info_feature_flags(info, tb[NL80211_ATTR_FEATURE_FLAGS]);
	wiphy_info_probe_resp_offload(capa,
		tb[NL80211_ATTR_PROBE_RESP_OFFLOAD]);
	*/

	if (tb[NL80211_ATTR_EXT_CAPA] && tb[NL80211_ATTR_EXT_CAPA_MASK] && drv->extended_capa == NULL)
	{
		drv->extended_capa = malloc(nla_len(tb[NL80211_ATTR_EXT_CAPA]));
		if (drv->extended_capa) {
			memcpy(drv->extended_capa, nla_data(tb[NL80211_ATTR_EXT_CAPA]), nla_len(tb[NL80211_ATTR_EXT_CAPA]));
			drv->extended_capa_len = nla_len(tb[NL80211_ATTR_EXT_CAPA]);
		}
		drv->extended_capa_mask = malloc(nla_len(tb[NL80211_ATTR_EXT_CAPA_MASK]));
		if (drv->extended_capa_mask) {
			memcpy(drv->extended_capa_mask, nla_data(tb[NL80211_ATTR_EXT_CAPA_MASK]), nla_len(tb[NL80211_ATTR_EXT_CAPA_MASK]));
		}
		else
		{
			free(drv->extended_capa);
			drv->extended_capa = NULL;
			drv->extended_capa_len = 0;
		}
	}

	if (tb[NL80211_ATTR_VENDOR_DATA]) {
		struct nlattr *nl;
		int rem;

		nla_for_each_nested(nl, tb[NL80211_ATTR_VENDOR_DATA], rem)
		{
			struct nl80211_vendor_cmd_info *vinfo;
			if (nla_len(nl) != sizeof(*vinfo)) {
				fprintf(stderr, "nl80211: Unexpected vendor data info\n");
				continue;
			}

			vinfo = nla_data(nl);
			switch (vinfo->subcmd) {
				case QCA_NL80211_VENDOR_SUBCMD_ROAMING:
					drv->roaming_vendor_cmd_avail = 1;
					break;
				case QCA_NL80211_VENDOR_SUBCMD_DFS_CAPABILITY:
					drv->dfs_vendor_cmd_avail = 1;
					break;
				case QCA_NL80211_VENDOR_SUBCMD_GET_FEATURES:
					drv->get_features_vendor_cmd_avail = 1;
					break;
				case QCA_NL80211_VENDOR_SUBCMD_DO_ACS:
					drv->capa.flags |= WPA_DRIVER_FLAGS_ACS_OFFLOAD;
					break;
			}

			fprintf(stderr, "nl80211: Supported vendor command: vendor_id=0x%x subcmd=%u\n", vinfo->vendor_id, vinfo->subcmd);
		}
	}

	if (tb[NL80211_ATTR_VENDOR_EVENTS]) {
		struct nlattr *nl;
		int rem;

		nla_for_each_nested(nl, tb[NL80211_ATTR_VENDOR_EVENTS], rem) {
			struct nl80211_vendor_cmd_info *vinfo;
			if (nla_len(nl) != sizeof(*vinfo)) {
				fprintf(stderr, "nl80211: Unexpected vendor data info\n");
				continue;
			}
			vinfo = nla_data(nl);
			fprintf(stderr, "nl80211: Supported vendor event: vendor_id=0x%x subcmd=%u\n",vinfo->vendor_id, vinfo->subcmd);
		}
	}

	wiphy_info_wowlan_triggers(capa, tb[NL80211_ATTR_WOWLAN_TRIGGERS_SUPPORTED]);

	if (tb[NL80211_ATTR_MAX_AP_ASSOC_STA])
		capa->max_stations = nla_get_u32(tb[NL80211_ATTR_MAX_AP_ASSOC_STA]);
	
	#endif 

	return NL_SKIP;
}



 
int nl80211_get_capa(struct nl80211_data *drv, struct wiphy_info_data *info)
{
	u32 feat;
	int flags = 0;
	feat = get_nl80211_protocol_features(drv);
	if (feat & NL80211_PROTOCOL_FEATURE_SPLIT_WIPHY_DUMP) {
		flags = NLM_F_DUMP;
	} 		

	memset(info, 0, sizeof(*info));

	/* CREATE A NETLINK MESSAGE */
	struct nl_msg *msg = nl80211_cmd_msg(drv, flags, NL80211_CMD_GET_WIPHY);
	if (!msg || nla_put_flag(msg, NL80211_ATTR_SPLIT_WIPHY_DUMP)) {
		nlmsg_free(msg);
		return -1;
	}

	if (send_and_recv_msgs(drv, msg, wiphy_info_handler, info))
		return -1;

	return 0;
}

