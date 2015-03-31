#ifndef MNGT_H
#define MNGT_H

void mngt_rx_handle(struct nl80211_data* ctx, u8 *buf, size_t len, int datarate, int ssi_signal);
int  send_deauth(struct nl80211_data *ctx, const u8 *addr, int reason);


#endif