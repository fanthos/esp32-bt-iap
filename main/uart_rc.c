#include "uart_rc.h"

#include "play_ctrl.h"

#include "esp_log.h"
#include "rciap.h"

#define TAG "UART_RC"

static xTaskHandle uart_task_handle = NULL;
static QueueHandle_t uart0_queue;
const int uart_num = UART_NUM_1;
#define BUF_SIZE (1024)

#define UART_APP_TXD 16
#define UART_APP_RXD 17

#define UART_APP_RX_WAIT_TIME (500 / portTICK_PERIOD_MS)

static void uart_app_evt(uint16_t evt, void *param) {
    //switch(ev)
}

int uart_app_cb_write(const char *data, const int32_t len, void *params) {
    uart_app_write(data, len);
    return 0;
}

static void uart_task_handler(void *arg) {
    uart_event_t event;
    size_t buffered_size;
    char* data = (char*) malloc(BUF_SIZE);
	uint32_t lastrun = 0;
	// uint32_t newrun = millis();
	// iap_buffer_t *buffer = (iap_buffer_t *)param;
	// memset(buffer->data, 0, sizeof(((iap_buffer_t*)0)->data));
	char _recvbuf[258];
	char *recvbuf = _recvbuf +1;
	int8_t recvstatus = 0;
	uint16_t recvpos = 0;
	uint16_t recvlen = 0;
	uint8_t recvsum = 0;;
	for(;;) {
		// if(newrun - lastrun > 50) {
		// 	//SerialDbg.print("Lagging: ");
		// 	//SerialDbg.println(newrun - lastrun);
		// }
		// lastrun = newrun;
        //Waiting for UART event.
        if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)UART_APP_RX_WAIT_TIME)) {
            ESP_LOGI(TAG, "uart[%d] event:", uart_num);
            switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.
                in this example, we don't process data in event, but read data outside.*/
                case UART_DATA: {
                    int len = uart_read_bytes(uart_num, (uint8_t *)data, BUF_SIZE, 100 / portTICK_RATE_MS);
                    for(int i = 0; i < len; i++) {
                        int input1 = data[i];
                        if(input1 != -1) {
                            if(recvstatus == -1) {
                                recvstatus = 0;
                                recvpos = 0;
                                recvlen = 0;
                                recvsum = 0;
                            }
                            if(input1 == 0xff && recvstatus == 0) {
                                recvstatus = 1;
                            } else if(input1 == 0x55 && recvstatus == 1) {
                                recvstatus = 2;
                            } else if(recvstatus == 2) {
                                recvlen = input1;
                                recvsum = input1;
                                recvstatus = 3;
                            } else if(recvstatus == 3) {
                                if(recvpos < len){
                                    recvbuf[recvpos] = input1;
                                    recvsum += input1;
                                    recvpos++;
                                } else {
                                    recvstatus = 4;
                                    _recvbuf[0] = len;
                                    recvbuf[len] = input1;
                                    // procunkcmd(buffer, len + 2, _recvbuf, "recv: ");
                                    recvsum += input1;
                                    if(recvsum == 0) {
                                        iap_proc_msg(recvbuf, recvlen, uart_app_cb_write, NULL);
                                    }
                                    recvstatus = -1;
                                }
                            } else {
                                recvstatus = -1;
                            }
                        }
                    }

                    break;
                }
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow\n");
                    //If fifo overflow happened, you should consider adding flow control for your application.
                    //We can read data out out the buffer, or directly flush the rx buffer.
                    uart_flush(uart_num);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full\n");
                    //If buffer full happened, you should consider encreasing your buffer size
                    //We can read data out out the buffer, or directly flush the rx buffer.
                    uart_flush(uart_num);
                    break;
                //Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d\n", event.type);
                    break;
            }
        } else {
            // Send notification
            iap_event_notify(PC_SEEK, uart_app_cb_write, NULL);
        }
    }
    free(data);
    data = NULL;
    vTaskDelete(NULL);
}

void uart_app_start() {
    uart_config_t uart_config = {
        .baud_rate = 19200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 120,
    };
    //Configure UART1 parameters
    uart_param_config(uart_num, &uart_config);
    //Set UART1 pins(TX: IO4, RX: I05, RTS: IO18, CTS: IO19)
    uart_set_pin(uart_num, UART_APP_TXD, UART_APP_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    //Install UART driver, and get the queue.
    uart_driver_install(uart_num, BUF_SIZE * 2, BUF_SIZE * 2, 10, &uart0_queue, 0);
    xTaskCreate(uart_task_handler, "UartT", 2048, NULL, configMAX_PRIORITIES - 2, &uart_task_handle);
}

void uart_app_stop() {
    if(uart_task_handle) {
        vTaskDelete(uart_task_handle);
        uart_task_handle = NULL;
    }
}

void uart_app_write(const char *buf, uint32_t len) {
    while(len > 0) {
        int ret = uart_write_bytes(uart_num, (const char*)buf, len);
        len -= ret;
    }
}

void uart_update_status(uint8_t status) {
    iap_event_notify(status, uart_app_cb_write, NULL);
}
