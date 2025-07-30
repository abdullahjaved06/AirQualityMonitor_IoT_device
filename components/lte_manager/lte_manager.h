#ifndef _LTE_MANAGER_H
#define _LTE_MANAGER_H

extern bool LTE_CONNECTED;

int lte_net_mgmt_connect(void);
int lte_net_mgmt_disconnect(void);
void register_lte_lc_event_handler(void);
int device_sleep_request_edrx(int requested_seconds);

#endif