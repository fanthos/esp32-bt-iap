#ifndef _RCIAP_H_
#define _RCIAP_H_

typedef int32_t (*iap_callback_t)(void* param, const char *, const uint32_t);
typedef int32_t (*iap_read_t)(void* param);

typedef struct {
	iap_callback_t callback;
	iap_read_t reader;
	void * param_callback;
	void * param_reader;
	uint8_t ready;
	uint8_t len;
	uint8_t data[140];
}iap_buffer_t;

#endif
