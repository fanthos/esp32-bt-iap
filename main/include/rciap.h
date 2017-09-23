#ifndef _RCIAP_H_
#define _RCIAP_H_

typedef int32_t (*iap_callback_t)(const uint8_t *, const uint32_t, void* param);
typedef int32_t (*iap_read_t)(void* param);

uint16_t iap_proc_msg(const uint8_t recv[], const uint16_t datalen,
	iap_callback_t cb, void *cb_param);

#endif
