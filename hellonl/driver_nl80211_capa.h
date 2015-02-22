#ifndef DRIVER_NL80211_CAPA_H
#define DRIVER_NL80211_CAPA_H


/**
* struct wpa_driver_capa - Driver capability information
*/
struct wpa_driver_capa {
#define WPA_DRIVER_CAPA_KEY_MGMT_WPA		0x00000001
#define WPA_DRIVER_CAPA_KEY_MGMT_WPA2		0x00000002
#define WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK	0x00000004
#define WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK	0x00000008
#define WPA_DRIVER_CAPA_KEY_MGMT_WPA_NONE	0x00000010
#define WPA_DRIVER_CAPA_KEY_MGMT_FT		0x00000020
#define WPA_DRIVER_CAPA_KEY_MGMT_FT_PSK		0x00000040
#define WPA_DRIVER_CAPA_KEY_MGMT_WAPI_PSK	0x00000080
#define WPA_DRIVER_CAPA_KEY_MGMT_SUITE_B	0x00000100
#define WPA_DRIVER_CAPA_KEY_MGMT_SUITE_B_192	0x00000200
	/** Bitfield of supported key management suites */
	unsigned int key_mgmt;

#define WPA_DRIVER_CAPA_ENC_WEP40	0x00000001
#define WPA_DRIVER_CAPA_ENC_WEP104	0x00000002
#define WPA_DRIVER_CAPA_ENC_TKIP	0x00000004
#define WPA_DRIVER_CAPA_ENC_CCMP	0x00000008
#define WPA_DRIVER_CAPA_ENC_WEP128	0x00000010
#define WPA_DRIVER_CAPA_ENC_GCMP	0x00000020
#define WPA_DRIVER_CAPA_ENC_GCMP_256	0x00000040
#define WPA_DRIVER_CAPA_ENC_CCMP_256	0x00000080
#define WPA_DRIVER_CAPA_ENC_BIP		0x00000100
#define WPA_DRIVER_CAPA_ENC_BIP_GMAC_128	0x00000200
#define WPA_DRIVER_CAPA_ENC_BIP_GMAC_256	0x00000400
#define WPA_DRIVER_CAPA_ENC_BIP_CMAC_256	0x00000800
#define WPA_DRIVER_CAPA_ENC_GTK_NOT_USED	0x00001000
	/** Bitfield of supported cipher suites */
	unsigned int enc;

#define WPA_DRIVER_AUTH_OPEN		0x00000001
#define WPA_DRIVER_AUTH_SHARED		0x00000002
#define WPA_DRIVER_AUTH_LEAP		0x00000004
	/** Bitfield of supported IEEE 802.11 authentication algorithms */
	unsigned int auth;

	/** Driver generated WPA/RSN IE */
#define WPA_DRIVER_FLAGS_DRIVER_IE	0x00000001
	/** Driver needs static WEP key setup after association command */
#define WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC 0x00000002
	/** Driver takes care of all DFS operations */
#define WPA_DRIVER_FLAGS_DFS_OFFLOAD			0x00000004
	/** Driver takes care of RSN 4-way handshake internally; PMK is configured with
	* struct wpa_driver_ops::set_key using alg = WPA_ALG_PMK */
#define WPA_DRIVER_FLAGS_4WAY_HANDSHAKE 0x00000008
	/** Driver is for a wired Ethernet interface */
#define WPA_DRIVER_FLAGS_WIRED		0x00000010
	/** Driver provides separate commands for authentication and association (SME in
	* wpa_supplicant). */
#define WPA_DRIVER_FLAGS_SME		0x00000020
	/** Driver supports AP mode */
#define WPA_DRIVER_FLAGS_AP		0x00000040
	/** Driver needs static WEP key setup after association has been completed */
#define WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC_DONE	0x00000080
	/** Driver supports dynamic HT 20/40 MHz channel changes during BSS lifetime */
#define WPA_DRIVER_FLAGS_HT_2040_COEX			0x00000100
	/** Driver supports concurrent P2P operations */
#define WPA_DRIVER_FLAGS_P2P_CONCURRENT	0x00000200
	/**
	* Driver uses the initial interface as a dedicated management interface, i.e.,
	* it cannot be used for P2P group operations or non-P2P purposes.
	*/
#define WPA_DRIVER_FLAGS_P2P_DEDICATED_INTERFACE	0x00000400
	/** This interface is P2P capable (P2P GO or P2P Client) */
#define WPA_DRIVER_FLAGS_P2P_CAPABLE	0x00000800
	/** Driver supports station and key removal when stopping an AP */
#define WPA_DRIVER_FLAGS_AP_TEARDOWN_SUPPORT		0x00001000
	/**
	* Driver uses the initial interface for P2P management interface and non-P2P
	* purposes (e.g., connect to infra AP), but this interface cannot be used for
	* P2P group operations.
	*/
#define WPA_DRIVER_FLAGS_P2P_MGMT_AND_NON_P2P		0x00002000
	/**
	* Driver is known to use sane error codes, i.e., when it indicates that
	* something (e.g., association) fails, there was indeed a failure and the
	* operation does not end up getting completed successfully later.
	*/
#define WPA_DRIVER_FLAGS_SANE_ERROR_CODES		0x00004000
	/** Driver supports off-channel TX */
#define WPA_DRIVER_FLAGS_OFFCHANNEL_TX			0x00008000
	/** Driver indicates TX status events for EAPOL Data frames */
#define WPA_DRIVER_FLAGS_EAPOL_TX_STATUS		0x00010000
	/** Driver indicates TX status events for Deauth/Disassoc frames */
#define WPA_DRIVER_FLAGS_DEAUTH_TX_STATUS		0x00020000
	/** Driver supports roaming (BSS selection) in firmware */
#define WPA_DRIVER_FLAGS_BSS_SELECTION			0x00040000
	/** Driver supports operating as a TDLS peer */
#define WPA_DRIVER_FLAGS_TDLS_SUPPORT			0x00080000
	/** Driver requires external TDLS setup/teardown/discovery */
#define WPA_DRIVER_FLAGS_TDLS_EXTERNAL_SETUP		0x00100000
	/** Driver indicates support for Probe Response offloading in AP mode */
#define WPA_DRIVER_FLAGS_PROBE_RESP_OFFLOAD		0x00200000
	/** Driver supports U-APSD in AP mode */
#define WPA_DRIVER_FLAGS_AP_UAPSD			0x00400000
	/** Driver supports inactivity timer in AP mode */
#define WPA_DRIVER_FLAGS_INACTIVITY_TIMER		0x00800000
	/** Driver expects user space implementation of MLME in AP mode */
#define WPA_DRIVER_FLAGS_AP_MLME			0x01000000
	/** Driver supports SAE with user space SME */
#define WPA_DRIVER_FLAGS_SAE				0x02000000
	/** Driver makes use of OBSS scan mechanism in wpa_supplicant */
#define WPA_DRIVER_FLAGS_OBSS_SCAN			0x04000000
	/** Driver supports IBSS (Ad-hoc) mode */
#define WPA_DRIVER_FLAGS_IBSS				0x08000000
	/** Driver supports radar detection */
#define WPA_DRIVER_FLAGS_RADAR				0x10000000
	/** Driver supports a dedicated interface for P2P Device */
#define WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE		0x20000000
	/** Driver supports QoS Mapping */
#define WPA_DRIVER_FLAGS_QOS_MAPPING			0x40000000
	/** Driver supports CSA in AP mode */
#define WPA_DRIVER_FLAGS_AP_CSA				0x80000000
	/** Driver supports mesh */
#define WPA_DRIVER_FLAGS_MESH			0x0000000100000000ULL
	/** Driver support ACS offload */
#define WPA_DRIVER_FLAGS_ACS_OFFLOAD		0x0000000200000000ULL
	/** Driver supports key management offload */
#define WPA_DRIVER_FLAGS_KEY_MGMT_OFFLOAD	0x0000000400000000ULL
	/** Driver supports TDLS channel switching */
#define WPA_DRIVER_FLAGS_TDLS_CHANNEL_SWITCH	0x0000000800000000ULL
	/** Driver supports IBSS with HT datarates */
#define WPA_DRIVER_FLAGS_HT_IBSS		0x0000001000000000ULL
	u64 flags;

#define WPA_DRIVER_SMPS_MODE_STATIC			0x00000001
#define WPA_DRIVER_SMPS_MODE_DYNAMIC			0x00000002
	unsigned int smps_modes;

	unsigned int wmm_ac_supported : 1;

	unsigned int mac_addr_rand_scan_supported : 1;
	unsigned int mac_addr_rand_sched_scan_supported : 1;

	/** Maximum number of supported active probe SSIDs */
	int max_scan_ssids;

	/** Maximum number of supported active probe SSIDs for sched_scan */
	int max_sched_scan_ssids;

	/** Whether sched_scan (offloaded scanning) is supported */
	int sched_scan_supported;

	/** Maximum number of supported match sets for sched_scan */
	int max_match_sets;

	/**
	* max_remain_on_chan - Maximum remain-on-channel duration in msec
	*/
	unsigned int max_remain_on_chan;

	/**
	* max_stations - Maximum number of associated stations the driver
	* supports in AP mode
	*/
	unsigned int max_stations;

	/**
	* probe_resp_offloads - Bitmap of supported protocols by the driver
	* for Probe Response offloading.
	*/
	/** Driver Probe Response offloading support for WPS ver. 1 */
#define WPA_DRIVER_PROBE_RESP_OFFLOAD_WPS		0x00000001
	/** Driver Probe Response offloading support for WPS ver. 2 */
#define WPA_DRIVER_PROBE_RESP_OFFLOAD_WPS2		0x00000002
	/** Driver Probe Response offloading support for P2P */
#define WPA_DRIVER_PROBE_RESP_OFFLOAD_P2P		0x00000004
	/** Driver Probe Response offloading support for IEEE 802.11u (Interworking) */
#define WPA_DRIVER_PROBE_RESP_OFFLOAD_INTERWORKING	0x00000008
	unsigned int probe_resp_offloads;

	unsigned int max_acl_mac_addrs;

	/**
	* Number of supported concurrent channels
	*/
	unsigned int num_multichan_concurrent;

	/**
	* extended_capa - extended capabilities in driver/device
	*
	* Must be allocated and freed by driver and the pointers must be
	* valid for the lifetime of the driver, i.e., freed in deinit()
	*/
	const u8 *extended_capa, *extended_capa_mask;
	unsigned int extended_capa_len;

	struct wowlan_triggers wowlan_triggers;

	/** Driver adds the DS Params Set IE in Probe Request frames */
#define WPA_DRIVER_FLAGS_DS_PARAM_SET_IE_IN_PROBES	0x00000001
	/** Driver adds the WFA TPC IE in Probe Request frames */
#define WPA_DRIVER_FLAGS_WFA_TPC_IE_IN_PROBES		0x00000002
	/** Driver handles quiet period requests */
#define WPA_DRIVER_FLAGS_QUIET				0x00000004
	/**
	* Driver is capable of inserting the current TX power value into the body of
	* transmitted frames.
	* Background: Some Action frames include a TPC Report IE. This IE contains a
	* TX power field, which has to be updated by lower layers. One such Action
	* frame is Link Measurement Report (part of RRM). Another is TPC Report (part
	* of spectrum management). Note that this insertion takes place at a fixed
	* offset, namely the 6th byte in the Action frame body.
	*/
#define WPA_DRIVER_FLAGS_TX_POWER_INSERTION		0x00000008
	u32 rrm_flags;
};


struct wiphy_info_data {
	char phyname[32];
	struct wpa_driver_capa capa;

	unsigned int num_multichan_concurrent;
	unsigned int error : 1;
	unsigned int device_ap_sme : 1;
	unsigned int poll_command_supported : 1;
	unsigned int data_tx_status : 1;
	unsigned int monitor_supported : 1;
	unsigned int auth_supported : 1;
	unsigned int connect_supported : 1;
	unsigned int p2p_go_supported : 1;
	unsigned int p2p_client_supported : 1;
	unsigned int p2p_concurrent : 1;
	unsigned int channel_switch_supported : 1;
	unsigned int set_qos_map_supported : 1;
	unsigned int have_low_prio_scan : 1;
	unsigned int wmm_ac_supported : 1;
	unsigned int mac_addr_rand_scan_supported : 1;
	unsigned int mac_addr_rand_sched_scan_supported : 1;
};

int nl80211_get_capa(struct nl80211_data *, struct wiphy_info_data*);

#endif