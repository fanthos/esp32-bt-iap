#ifndef _RCIAP_H_
#define _RCIAP_H_

typedef int32_t (*iap_callback_t)(const char *, const int32_t, void* param);

uint16_t iap_proc_msg(const char recv[], const uint16_t datalen,
	iap_callback_t cb, void *cb_param);
void iap_event_notify(uint8_t status, iap_callback_t cb, void *cb_param);

#endif
