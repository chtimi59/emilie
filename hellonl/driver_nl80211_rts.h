#ifndef DRIVER_NL80211_RST_H
#define DRIVER_NL80211_RST_H

int get_nl80211_rts_threshold(struct nl80211_data *drv, int* rts);
int set_nl80211_rts_threshold(struct nl80211_data *drv, int rts);

#endif