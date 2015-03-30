/*
 * IEEE 802.11 Common routines
 * Copyright (c) 2002-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef IEEE802_11_COMMON_H
#define IEEE802_11_COMMON_H

const u8 * get_hdr_bssid(const struct ieee80211_hdr *hdr, size_t len);
const char * fc2str(u16 fc);

#endif /* IEEE802_11_COMMON_H */
