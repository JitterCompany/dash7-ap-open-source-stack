/*
 *  Created on: Feb 2, 2013
 *  Authors:
 * 		maarten.weyn@artesis.be
 */


#include <string.h>
#include <nwl/nwl.h>
#include <hal/system.h>
#include <hal/button.h>
#include <hal/leds.h>
#include <hal/rtc.h>
#include <log.h>
#include <timer.h>


#define ADV_TIMESPAN 10
#define MSG_TIMESPAN 2000;
static u16 counter = 0;

static timer_event event;
static u16 timer = 500;

void send_adv_prot_data(void * arg)
{
	led_on(1);
	nwl_tx_advertising_protocol_data(0x10, timer, 0, 0xFF, 0x10);
	timer -= 10;
	if (timer > 0)
	{
		timer_add_event(&event);
	} else {
		nwl_tx_network_protocol_data((uint8_t*) &counter, 2, NULL, NULL, 0xFF, 0x10, 0);
		counter++;
		event.next_event = MSG_TIMESPAN;
		timer_add_event(&event);

		timer = 500;
		event.next_event = ADV_TIMESPAN;
	}
}

void rx_callback(nwl_rx_res_t* rx_res)
{
	log_print_string("RX CB");
}

void tx_callback(Dll_Tx_Result result)
{
	if(result == DLLTxResultOK)
	{
		counter++;
		led_off(1);
		log_print_string("TX OK");
	}
	else if (result == DLLTxResultCCAFail)
	{
		led_toggle(2);
		log_print_string("TX CCA FAIL");
	} else
	{
		led_toggle(2);
		log_print_string("TX FAIL");
	}

}


void main(void) {
	system_init();
	button_enable_interrupts();

	nwl_init();
	nwl_set_tx_callback(&tx_callback);
	nwl_set_rx_callback(&rx_callback);

	log_print_string("started");


	event.f = &send_adv_prot_data;
	event.next_event = ADV_TIMESPAN;

	timer_add_event(&event);

	while(1)
	{
		system_lowpower_mode(4,1);
	}
}

