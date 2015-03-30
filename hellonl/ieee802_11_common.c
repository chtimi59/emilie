/*
 * IEEE 802.11 Common routines
 * Copyright (c) 2002-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "common.h"
#include "ieee802_11_defs.h"
#include "ieee802_11_common.h"



const u8 * get_hdr_bssid(const struct ieee80211_hdr *hdr, size_t len)
{
	u16 fc, type, stype;

	/*
	 * PS-Poll frames are 16 bytes. All other frames are
	 * 24 bytes or longer.
	 */
	if (len < 16)
		return NULL;

	fc = le_to_host16(hdr->frame_control);
	type = WLAN_FC_GET_TYPE(fc);
	stype = WLAN_FC_GET_STYPE(fc);

	switch (type) {
	case WLAN_FC_TYPE_DATA:
		if (len < 24)
			return NULL;
		switch (fc & (WLAN_FC_FROMDS | WLAN_FC_TODS)) {
		case WLAN_FC_FROMDS | WLAN_FC_TODS:
		case WLAN_FC_TODS:
			return hdr->addr1;
		case WLAN_FC_FROMDS:
			return hdr->addr2;
		default:
			return NULL;
		}
	case WLAN_FC_TYPE_CTRL:
		if (stype != WLAN_FC_STYPE_PSPOLL)
			return NULL;
		return hdr->addr1;
	case WLAN_FC_TYPE_MGMT:
		return hdr->addr3;
	default:
		return NULL;
	}
}


const char * fc2str(u16 fc)
{
	u16 stype = WLAN_FC_GET_STYPE(fc);
#define C2S(x) case x: return #x;

	switch (WLAN_FC_GET_TYPE(fc)) {
	case WLAN_FC_TYPE_MGMT:
		switch (stype) {
		C2S(WLAN_FC_STYPE_ASSOC_REQ)
		C2S(WLAN_FC_STYPE_ASSOC_RESP)
		C2S(WLAN_FC_STYPE_REASSOC_REQ)
		C2S(WLAN_FC_STYPE_REASSOC_RESP)
		C2S(WLAN_FC_STYPE_PROBE_REQ)
		C2S(WLAN_FC_STYPE_PROBE_RESP)
		C2S(WLAN_FC_STYPE_BEACON)
		C2S(WLAN_FC_STYPE_ATIM)
		C2S(WLAN_FC_STYPE_DISASSOC)
		C2S(WLAN_FC_STYPE_AUTH)
		C2S(WLAN_FC_STYPE_DEAUTH)
		C2S(WLAN_FC_STYPE_ACTION)
		}
		break;
	case WLAN_FC_TYPE_CTRL:
		switch (stype) {
		C2S(WLAN_FC_STYPE_PSPOLL)
		C2S(WLAN_FC_STYPE_RTS)
		C2S(WLAN_FC_STYPE_CTS)
		C2S(WLAN_FC_STYPE_ACK)
		C2S(WLAN_FC_STYPE_CFEND)
		C2S(WLAN_FC_STYPE_CFENDACK)
		}
		break;
	case WLAN_FC_TYPE_DATA:
		switch (stype) {
		C2S(WLAN_FC_STYPE_DATA)
		C2S(WLAN_FC_STYPE_DATA_CFACK)
		C2S(WLAN_FC_STYPE_DATA_CFPOLL)
		C2S(WLAN_FC_STYPE_DATA_CFACKPOLL)
		C2S(WLAN_FC_STYPE_NULLFUNC)
		C2S(WLAN_FC_STYPE_CFACK)
		C2S(WLAN_FC_STYPE_CFPOLL)
		C2S(WLAN_FC_STYPE_CFACKPOLL)
		C2S(WLAN_FC_STYPE_QOS_DATA)
		C2S(WLAN_FC_STYPE_QOS_DATA_CFACK)
		C2S(WLAN_FC_STYPE_QOS_DATA_CFPOLL)
		C2S(WLAN_FC_STYPE_QOS_DATA_CFACKPOLL)
		C2S(WLAN_FC_STYPE_QOS_NULL)
		C2S(WLAN_FC_STYPE_QOS_CFPOLL)
		C2S(WLAN_FC_STYPE_QOS_CFACKPOLL)
		}
		break;
	}
	return "WLAN_FC_TYPE_UNKNOWN";
#undef C2S
}
