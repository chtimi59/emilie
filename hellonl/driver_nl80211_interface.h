#ifndef DRIVER_NL80211_INTERFACE_H
#define DRIVER_NL80211_INTERFACE_H

int nl80211_get_wiphy_index(struct nl80211_data *ctx, int* phyidx);
int nl80211_get_ifmode(struct nl80211_data *ctx, enum nl80211_iftype* nlmode);
int nl80211_set_ifmode(struct nl80211_data* ctx, enum nl80211_iftype mode);

#endif