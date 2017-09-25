#include <stdint.h>
#include <string.h>

#include "rciap.h"
#include "uart_rc.h"
#include "play_ctrl.h"

#define WRITERET(x, b, l) l += sizeof(x);memcpy(b,x,l);
#define R(x) retdata ## x
#define RL(x) sizeof(retdata ## x)

uint8_t playstatus = 0;

uint16_t STATUS_PUSH = 0x0001;

#define IAP_BAUDRATE 19200
#define IAP_BYTEPS IAP_BAUDRATE / 10
#define IAP_MAXWAIT 256L * 1000L / IAP_BYTEPS + 1
#define IAP_MILLISBUF 1

enum {
	SCM_BASIC = 0x0001,
	SCM_EXT = 0x0002,
	SCM_TRACK_INDEX = 0x0004,
	SCM_TRACK_INDEX_MS = 0x0008,
	SCM_TRACK_INDEX_S = 0x0010,
	SCM_CHAPTER_INDEX = 0x0020,
	SCM_CHAPTER_INDEX_MS = 0x0040,
	SCM_CHAPTER_INDEX_S = 0x0080,
	SCM_TRACK_UID = 0x0100,
	SCM_TRACK_TYPE = 0x0200,
	SCM_TRACK_LYRIC = 0x0400
};

enum {
	SCI_STOP = 0x00,
	SCI_FFW = 0x02,
	SCI_REW = 0x03,
	SCI_EXT = 0x06,
	SCI_TRACK_INDEX = 0x01,
	SCI_TRACK_INDEX_MS = 0x04,
	SCI_TRACK_INDEX_S = 0x07,
	SCI_CHAPTER_INDEX = 0x05,
	SCI_CHAPTER_INDEX_MS = 0x08,
	SCI_CHAPTER_INDEX_S = 0x09,
	SCI_TRACK_UID = 0x0a,
	SCI_TRACK_TYPE = 0x0b,
	SCI_TRACK_LYRIC = 0x0c
};

static uint32_t iap_event_mask = 0;

static inline uint32_t uc2ul(const uint8_t * a) {
	return (*a << 24) | (*(a + 1) << 16) | (*(a + 2) << 8) | *(a + 3);
}

static inline void ul2uc(uint32_t a, uint8_t *buf) {
	buf[0] = (a >> 24) & 0xff;
	buf[1] = (a >> 16) & 0xff;
	buf[2] = (a >> 8) & 0xff;
	buf[3] = a & 0xff;
}

static void calcsum(uint8_t * const buf, uint16_t len) {
	uint8_t sum = 0;
	const uint8_t *buf1 = buf + len + 2;
	buf[2] = (uint8_t)len;
	while(buf1 >= buf + 2) {
		sum -= *buf1;
		buf1--;
	}
	buf[len + 3] = sum;
}

static void procunkcmd(const uint16_t len,
		const uint8_t *const recv, const uint8_t lbl[]) {
	//SerialDbg.print(lbl);
	for(uint16_t _i = 0; _i < len; _i++){
		//SerialDbg.print(recv[_i], HEX);
		//SerialDbg.print(" ");
	}
	//SerialDbg.println("");
}

void iap_event_notify(uint8_t status, iap_callback_t cb, void *cb_param) {
	uint8_t sendbuf[20] = {0xff, 0x55, 0x00, 0x04, 0x00, 0x27};
	if (status == PC_SEEK) {
		if (iap_event_mask | SCM_TRACK_INDEX_MS) {
			uint32_t playtime = play_get_time();
			sendbuf[6] = SCI_TRACK_INDEX_MS;
			ul2uc(playtime, sendbuf + 7);
			calcsum(sendbuf, 8);
			cb(sendbuf, 12, cb_param);
			return;
		} else if (iap_event_mask | SCM_TRACK_INDEX_S) {
			uint32_t playtime = play_get_time() / 1000;
			sendbuf[6] = SCI_TRACK_INDEX_MS;
			ul2uc(playtime, sendbuf + 7);
			calcsum(sendbuf, 8);
			cb(sendbuf, 12, cb_param);
			return;
		}
	} else if (iap_event_mask | SCM_EXT) {
		sendbuf[6] = SCI_EXT;
		switch (status) {
		case PC_PLAY:
			sendbuf[7] = 0x0a;
			break;
		case PC_STOP:
			sendbuf[7] = 0x02;
			break;
		case PC_PAUSE:
			sendbuf[7] = 0x0b;
			break;
		}
		calcsum(sendbuf, 5);
		cb(sendbuf, 9, cb_param);
		return;
	}
}

uint16_t iap_proc_msg(const uint8_t recv[], const uint16_t datalen,
		iap_callback_t cb, void *cb_param) {
	const uint8_t
		//retdata00ack[] = {0,0},
		retdata_0007[] = "MYBT1",
		retdata_0009[] = {8,1,1},
		retdata_000B[] = "MYBT1ABCDEFG",
		retdata_000F[] = {0,1,15},
		retdata_04ack[] = {0,0,0},
		retdata_04ackunk[] = {5,0,0},
		retdata_str[] = "BTAUDIO",
		//retdata04_0[] = {0},
		retdata_04str[] = "\0\0\0\0MY888",
		retdata_00[] = {0,0,0,0},
		retdata_01[] = {0,0,0,1},
		retdata_02[] = {0,0,0,2},
		retdata_03[] = {0,0,0,3},
		retdata_0101[] = {0,0,0,1,0,0,0,1},
		retdata_041C[] = {0,0,0xea,0x60,0,0,0x27,0x10,1},
		retdata_ary0028[] = {1,4,4,4,5,6,7},
		retdata_buf0013[] = {0x00, 0x27, 0x00},
		retdata_d0027[] = {1, 0, 0, 0, 1};
	uint16_t retlen;
	uint8_t len;
	uint8_t data[140] = {0xff, 0x55};
	uint8_t * const sendbuf = data + 3;

	if(recv[0] == 0 && datalen > 1) {
		uint8_t *rbuf = sendbuf + 2;
		sendbuf[0] = 0;
		sendbuf[1] = recv[1] + 1;
		len = 0;
		const uint16_t headersize = 2;
		switch(recv[1]) {
			case 0x01: // Identify
			case 0x02: // !ACK
				return 0;
			case 0x03: // RequesetRemoteUIMode
				rbuf[0] = 1;
				len = headersize + 1;
				break;
			case 0x05: // switch remote ui
			case 0x06: // Exit remote ui
				rbuf[0] = 0;
				rbuf[1] = recv[1];
				sendbuf[1] = 2;
				len = headersize + 2;
				break;
			case 0x07: // get ipod name
				len = headersize;
				WRITERET(R(_0007),rbuf, len);
				break;
			case 0x09: // get ipod version
				len = headersize;
				WRITERET(R(_0009),rbuf, len);
				break;
			case 0x0B: // get serial
				len = headersize;
				WRITERET(R(_000B),rbuf, len);
				break;
			case 0x0D: // Get Device Type
				len = headersize;
				WRITERET(R(_04str),rbuf, len);
				rbuf[1] = 3;
				break;
			case 0x0F: // req lingo protocol version
				len = headersize;
				WRITERET(R(_000F),rbuf, len);
				rbuf[0] = recv[2];
				break;
			case 0x13: // Identify Device Lingoes
				sendbuf[0] = 0;
				sendbuf[1] = 2;
				sendbuf[2] = 0;
				sendbuf[3] = 0x13;
				len = headersize + 2;
				calcsum(data, len);
				cb(data, len + 2, cb_param);

				sendbuf[0] = 0;
				sendbuf[1] = 0x27;
				sendbuf[2] = 0;
				len = headersize + 1;
				break;
			case 0x28: // Device RetAccessoryInfo
				rbuf[0] = R(_ary0028)[recv[2]];
				if(rbuf[0] > RL(_ary0028)){
					return 0;
				}
				len = headersize + 1;
				sendbuf[1] = 0x27;
				break;
			default:
				rbuf[0] = 5;
				rbuf[1] = recv[1];
				sendbuf[1] = 2;
				len = headersize + 2;
				procunkcmd(datalen, recv, (uint8_t*)"!!!Unk: ");
				break;
		}
	} else if (recv[0] == 4 && recv[1] == 0 && datalen > 2) {
		uint8_t *rbuf = sendbuf + 3;
		sendbuf[0] = 4;
		sendbuf[1] = 0;
		sendbuf[2] = recv[2] + 1;
		const uint16_t headersize = 3;
		switch(recv[2]) {
			uint8_t action;
			case 0x04: // Set current chapter
			case 0x0B: // Set audiobook speed
			case 0x16: // Rest db selection
			case 0x17: // Select db record
			case 0x28: // Play Current Selection
			case 0x2E: // Set Shuffle
			case 0x31: // Set Repeat
			case 0x38: // SelectSortDbRecord
				len = headersize;
				WRITERET(R(_04ack),rbuf, len);
				rbuf[2] = recv[2];
				sendbuf[2] = 1;
				break;
			case 0x02:  // Get current playing info
				len = headersize;
				WRITERET(R(_0101),rbuf, len);
				break;
			case 0x05: // Get current play status
				len = headersize;
				WRITERET(R(_02),rbuf, len);
				break;
			case 0x07: // Get chapter name
			case 0x20: // Get Track Title
			case 0x22: // Get Artist Name
			case 0x24: // Get Albun name
				len = headersize;
				WRITERET(R(_str),rbuf, len);
				break;
			case 0x09: // Get audiobook speed
			case 0x2C: // Get Shuffle
			case 0x2F: // Get Repeat
				len = headersize + 1;
				rbuf[0] = 0;
				break;
			case 0x18: // Get num of categorized db records
				len = headersize;
				WRITERET(R(_03),rbuf, len);
				break;
			case 0x1C: // Get PlayStatus
				len = headersize;
				WRITERET(R(_041C),rbuf, len);
				break;
			case 0x1E: // Get current playing track index
				len = headersize;
				WRITERET(R(_01),rbuf, len);
				break;
			case 0x35: // Get num playing tracks
				len = headersize;
				WRITERET(R(_03),rbuf, len);
				break;
			case 0x1A: // Retrieve categorized db records
				{
					uint32_t id_s, id_e;
					id_e = uc2ul(recv + 7);
					if(id_e == 0xffffffff) {
						id_s = 0;
						id_e = 4;
					} else {
						id_s = uc2ul(recv + 3);
						id_e = id_s + id_e;
						if(id_s > 3)id_s = 3;
						if(id_e > 4)id_e = 4;
					}
					memcpy(rbuf+4, R(_str), RL(_str));
					len = headersize;
					len += RL(_str) + 4;
					while(id_s < id_e) {
						rbuf[0] = id_s >> 24;
						rbuf[1] = id_s >> 16;
						rbuf[2] = id_s >> 8;
						rbuf[3] = id_s;
						calcsum(data, len);
						cb(data, len, cb_param);
						id_s ++;
					}
					return 0;
				}
			case 0x26: // Set PlayStatusChange notification
				if(datalen < 4) {
					if(recv[3] == 0) {
						iap_event_mask = 0x0000;
					} else {
						iap_event_mask = 0x002d;
					}
				} else {
					iap_event_mask = uc2ul(recv + 3);
				}
			case 0x29: // Play control
				switch(recv[3]) {
					case 0x01:
						action = PC_TOGGLE;
						break;
					case 0x02:
						action = PC_STOP;
						break;
					case 0x0A:
						action = PC_PLAY;
						break;
					case 0x0B:
						action = PC_PAUSE;
						break;
					case 0x03:
					case 0x08:
					case 0x0C:
						action = PC_NEXT;
						break;
					case 0x04:
					case 0x09:
					case 0x0D:
						action = PC_PREV;
						break;
					default:
						action = PC_NONE;
						break;
				}
				if(action != PC_NONE) {
					play_control(action);
				}
				len = headersize;
				WRITERET(R(_04ack),rbuf, len);
				rbuf[2] = recv[2];
				sendbuf[2] = 1;

				calcsum(data, len);
				cb(data, len, cb_param);

				sendbuf[2] = 0x27;
				len = headersize;
				WRITERET(R(_d0027),rbuf, len);

				break;
			case 0x37: // Set play track
				switch(recv[6]) {
					case 0:
						action = PC_PREV;
						break;
					case 2:
						action = PC_NEXT;
						break;
					default:
						action = PC_NONE;
						break;
				}
				if(action != PC_NONE) {
					play_control(action);
				}
				len = headersize;
				WRITERET(R(_04ack),rbuf, len);
				rbuf[2] = recv[2];
				sendbuf[2] = 1;
				calcsum(data, len);
				cb(data, len, cb_param);

				sendbuf[2] = 0x27;
				len = headersize;
				WRITERET(R(_d0027),rbuf, len);
				break;
			default:
				len = headersize;
				WRITERET(R(_04ackunk),rbuf, len);
				rbuf[2] = recv[2];
				sendbuf[2] = 1;
				procunkcmd(datalen, recv, (uint8_t*)"!!!Unk: ");
				break;
		}
	} else {
		procunkcmd(datalen, recv, (uint8_t*)"!!!Unk: ");
		return 0;
	}

	if(len > 0) {
		calcsum(data, len);
		cb(data, len, cb_param);
	}
	return retlen;
}
