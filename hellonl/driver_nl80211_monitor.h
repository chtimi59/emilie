#ifndef DRIVER_NL80211_MONITOR_H
#define DRIVER_NL80211_MONITOR_H

int nl80211_create_monitor_interface(struct nl80211_data* ctx);
void nl80211_remove_monitor_interface(struct nl80211_data* ctx);

#endif