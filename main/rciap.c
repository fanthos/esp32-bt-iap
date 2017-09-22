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

const unsigned char CTRLPIN_PLAY = 2;
const unsigned char CTRLPIN_NEXT = 3;
const unsigned char CTRLPIN_PREV = 4;

static inline void setstatus(uint8_t flag) {
	playstatus |= flag;
}

static inline void clearstatus(uint8_t flag) {
	playstatus &= ~flag;
}

static inline uint8_t getstatus(uint8_t flag) {
	return (playstatus & flag);
}

static inline void mydelay(const uint32_t t) {
	//SerialDbg.print("mydelay: ");
	//SerialDbg.println(t);
	// delay(t);
}

static inline uint32_t uc2ul(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
	uint32_t t = a;
	t = t << 8 | b;
	t = t << 8 | c;
	t = t << 8 | d;
	return t;
}

static uint8_t calcsum(const uint8_t *buf, uint16_t len) {
	uint8_t sum = 0;
	const uint8_t *buf1 = buf + len;
	while(buf < buf1) {
		sum -= *buf;
		buf++;
	}
	return sum;
}

static void procunkcmd(iap_buffer_t *buffer, const uint16_t len, const uint8_t recv[], const char lbl[]) {
	//SerialDbg.print(lbl);
	for(uint16_t _i = 0; _i < len; _i++){
		//SerialDbg.print(recv[_i], HEX);
		//SerialDbg.print(" ");
	}
	//SerialDbg.println("");
}

static void serialwrite(iap_buffer_t *buffer, const uint8_t buf[], const uint16_t len) {
	static uint32_t lastsent = 0;
	uint32_t newsent;
	const uint32_t sendinterval = 10;
	uint32_t t;
	uint32_t sendtime = len * 1000 / IAP_BYTEPS + IAP_MILLISBUF;
	procunkcmd(buffer, len - 2, buf + 2, "send: ");
	newsent = xTaskGetTickCount() * 10;
	if(lastsent > newsent) {
		t = lastsent - newsent;
		if(t > IAP_MAXWAIT) {
			t = IAP_MAXWAIT;
		}
		mydelay(t + sendinterval);
	} else {
		t = (newsent - lastsent);
		if(t < sendinterval){
			mydelay(sendinterval - t);
		}
	}
	lastsent = newsent + sendtime;
	buffer->callback(buffer->param_callback, (const char *)buf, len);
}

static void processsendbuf(iap_buffer_t *buffer) {
	if(buffer->ready == 0)return;
	buffer->data[2] = (uint8_t)buffer->len;
	buffer->data[buffer->len + 3] = calcsum((uint8_t*)buffer->data + 2, buffer->len + 1);
	serialwrite(buffer, (uint8_t *)buffer->data, buffer->len + 4);
	buffer->ready = 0;
}

static uint16_t doreply(iap_buffer_t *buffer, const uint16_t len, const uint8_t recv[]) {
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
		retdata_041C[] = {0,0,0xea,0x60,0,0,0x27,0x10,1};
	const uint8_t retdata_ary0028[] = {1,4,4,4,5,6,7};
	const uint8_t retdata_buf0013[] = {0x00, 0x27, 0x00};
	const uint8_t retdata_d0027[] = {1, 0, 0, 0, 1};
	uint8_t * const sendbuf = buffer->data + 3;
	uint16_t retlen;

	if(recv[0] == 0 && len > 1) {
		uint8_t *rbuf = sendbuf + 2;
		sendbuf[0] = 0;
		sendbuf[1] = recv[1] + 1;
		buffer->len = 0;
		const uint16_t headersize = 2;
		switch(recv[1]) {
			case 0x01: // Identify
			case 0x02: // !ACK
				return 0;
			case 0x03: // RequesetRemoteUIMode
				rbuf[0] = 1;
				buffer->len = headersize + 1;
				break;
			case 0x05: // switch remote ui
			case 0x06: // Exit remote ui
				rbuf[0] = 0;
				rbuf[1] = recv[1];
				sendbuf[1] = 2;
				buffer->len = headersize + 2;
				break;
			case 0x07: // get ipod name
				buffer->len = headersize;
				WRITERET(R(_0007),rbuf, buffer->len);
				break;
			case 0x09: // get ipod version
				buffer->len = headersize;
				WRITERET(R(_0009),rbuf, buffer->len);
				break;
			case 0x0B: // get serial
				buffer->len = headersize;
				WRITERET(R(_000B),rbuf, buffer->len);
				break;
			case 0x0D: // Get Device Type
				buffer->len = headersize;
				WRITERET(R(_04str),rbuf, buffer->len);
				rbuf[1] = 3;
				break;
			case 0x0F: // req lingo protocol version
				buffer->len = headersize;
				WRITERET(R(_000F),rbuf, buffer->len);
				rbuf[0] = recv[2];
				break;
			case 0x13: // Identify Device Lingoes
				sendbuf[0] = 0;
				sendbuf[1] = 2;
				sendbuf[2] = 0;
				sendbuf[3] = 0x13;
				buffer->len = headersize + 2;
				buffer->ready = 1;
				processsendbuf(buffer);
				memcpy((void*)sendbuf, R(_buf0013), RL(_buf0013));
				buffer->len = headersize + 1;
				buffer->ready = 1;
				break;
			case 0x28: // Device RetAccessoryInfo
				rbuf[0] = R(_ary0028)[recv[2]];
				if(rbuf[0] > RL(_ary0028)){
					return 0;
				}
				buffer->len = headersize + 1;
				sendbuf[1] = 0x27;
				break;
			default:
				rbuf[0] = 5;
				rbuf[1] = recv[1];
				sendbuf[1] = 2;
				buffer->len = headersize + 2;
				procunkcmd(buffer, len, recv, "!!!Unk: ");
				break;
		}
	} else if (recv[0] == 4 && recv[1] == 0 && len > 2) {
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
			case 0x26: // Set PlayStatusChange notification
			case 0x28: // Play Current Selection
			case 0x2E: // Set Shuffle
			case 0x31: // Set Repeat
			case 0x38: // SelectSortDbRecord
				buffer->len = headersize;
				WRITERET(R(_04ack),rbuf, buffer->len);
				rbuf[2] = recv[2];
				sendbuf[2] = 1;
				break;
			case 0x02:  // Get current playing info
				buffer->len = headersize;
				WRITERET(R(_0101),rbuf, buffer->len);
				break;
			case 0x05: // Get current play status
				buffer->len = headersize;
				WRITERET(R(_02),rbuf, buffer->len);
				break;
			case 0x07: // Get chapter name
			case 0x20: // Get Track Title
			case 0x22: // Get Artist Name
			case 0x24: // Get Albun name
				buffer->len = headersize;
				WRITERET(R(_str),rbuf, buffer->len);
				break;
			case 0x09: // Get audiobook speed
			case 0x2C: // Get Shuffle
			case 0x2F: // Get Repeat
				buffer->len = headersize + 1;
				rbuf[0] = 0;
				break;
			case 0x18: // Get num of categorized db records
				buffer->len = headersize;
				WRITERET(R(_03),rbuf, buffer->len);
				break;
			case 0x1C: // Get PlayStatus
				buffer->len = headersize;
				WRITERET(R(_041C),rbuf, buffer->len);
				break;
			case 0x1E: // Get current playing track index
				buffer->len = headersize;
				WRITERET(R(_01),rbuf, buffer->len);
				break;
			case 0x35: // Get num playing tracks
				buffer->len = headersize;
				WRITERET(R(_03),rbuf, buffer->len);
				break;
			case 0x1A: // Retrieve categorized db records
				{
					uint32_t id_s, id_e;
					id_e = uc2ul(recv[7], recv[8], recv[9], recv[10]);
					if(id_e == 0xffffffff) {
						id_s = 0;
						id_e = 4;
					} else {
						id_s = uc2ul(recv[3], recv[4], recv[5], recv[6]);
						id_e = id_s + id_e;
						if(id_s > 3)id_s = 3;
						if(id_e > 4)id_e = 4;
					}
					memcpy(rbuf+4, R(_str), RL(_str));
					buffer->len = headersize;
					buffer->len += RL(_str) + 4;
					while(id_s < id_e) {
						if(buffer->ready == 1) {
							processsendbuf(buffer);
						}
						rbuf[0] = id_s >> 24;
						rbuf[1] = id_s >> 16;
						rbuf[2] = id_s >> 8;
						rbuf[3] = id_s;
						buffer->ready = 1;
						id_s ++;
					}
					return 0;
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
				buffer->len = headersize;
				WRITERET(R(_04ack),rbuf, buffer->len);
				rbuf[2] = recv[2];
				sendbuf[2] = 1;
				buffer->ready = 1;

				processsendbuf(buffer);
				sendbuf[2] = 0x27;
				buffer->len = headersize;
				WRITERET(R(_d0027),rbuf, buffer->len);
				buffer->ready = 1;

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
				buffer->len = headersize;
				WRITERET(R(_04ack),rbuf, buffer->len);
				rbuf[2] = recv[2];
				sendbuf[2] = 1;
				buffer->ready = 1;
				processsendbuf(buffer);

				sendbuf[2] = 0x27;
				buffer->len = headersize;
				WRITERET(R(_d0027),rbuf, buffer->len);
				buffer->ready = 1;
				break;
			default:
				buffer->len = headersize;
				WRITERET(R(_04ackunk),rbuf, buffer->len);
				rbuf[2] = recv[2];
				sendbuf[2] = 1;
				procunkcmd(buffer, len, recv, "!!!Unk: ");
				break;
		}
	} else {
		procunkcmd(buffer, len, recv, "!!!Unk: ");
		return 0;
	}

	if(buffer->len > 0) {
		buffer->ready = 1;
	}
	return retlen;
}

void processserial(iap_buffer_t *buffer, const uint8_t input1) {
	static uint8_t _buf[258];
	static uint8_t *buf = _buf +1;
	static uint8_t status = 0;
	static uint16_t pos = 0;
	static uint16_t len = 0;
	static uint8_t sum = 0;;
	// static uint32_t lasttime = -1;

	// uint32_t newtime;
	// newtime = xTaskGetTickCount() * 10;
	// if(newtime - lasttime > 100) {
	// 	//status = -1;
	// }
	// lasttime = newtime;
	if(status == -1) {
		status = 0;
		pos = 0;
		len = 0;
		sum = 0;
	}
	if(input1 == 0xff && status == 0) {
		status = 1;
	} else if(input1 == 0x55 && status == 1) {
		status = 2;
	} else if(status == 2) {
		len = input1;
		sum = input1;
		status = 3;
	} else if(status == 3) {
		if(pos < len){
			buf[pos] = input1;
			sum += input1;
			pos++;
		} else {
			status = 4;
			_buf[0] = len;
			buf[len] = input1;
			procunkcmd(buffer, len + 2, _buf, "recv: ");
			sum += input1;
			if(sum == 0) {
				doreply(buffer, len, buf);
			} else {
				//SerialDbg.print("!!!chksum: ");
				//SerialDbg.println(sum, HEX);
			}
			status = -1;
		}
	} else {
		status = -1;
	}
}

void iap_handler(void *param) {
	uint32_t lastrun = 0;
	// uint32_t newrun = millis();
	iap_buffer_t *buffer = (iap_buffer_t *)param;
	memset(buffer->data, 0, sizeof(((iap_buffer_t*)0)->data));
	buffer->data[0] = 0xff;
	buffer->data[1] = 0x55;
	buffer->len = 0;
	buffer->ready = 0;
	for(;;) {
		// if(newrun - lastrun > 50) {
		// 	//SerialDbg.print("Lagging: ");
		// 	//SerialDbg.println(newrun - lastrun);
		// }
		// lastrun = newrun;
		if(buffer->ready) {
			processsendbuf(buffer);
		} else {
			int ret1;
			ret1 = buffer->reader(buffer->param_reader);
			if(ret1 != -1) {
				processserial(buffer, (uint8_t)ret1);
			}
		}
	}
}
