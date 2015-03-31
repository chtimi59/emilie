#ifndef DRIVER_NL80211_MONITOR_H
#define DRIVER_NL80211_MONITOR_H

int nl80211_create_monitor_interface(struct nl80211_data* ctx);
void nl80211_remove_monitor_interface(struct nl80211_data* ctx);

int monitor_tx(struct nl80211_data* ctx, const void *data, size_t len, int noack);

#endif