#include "launchpad.h"
#include "log.h"
#include "midi.h"
#include "sndfile.h"
#include "song.h"
#include <stdio.h>

int loop_start;
int loop_end;

int lp_grid_buttoncode(int val)
{
	if (val >= 0 && val < 64) {
		int ledRow;
		ledRow = val/8;
		return 0x00+(0x10*ledRow)+(val-(ledRow*8));
	}
	return 0;
}

int lp_grid_button_hex_to_int(int val)
{
	int ledRow;
	ledRow = val/0x10;
	return val-(ledRow*8);
}

int lp_is_hex_code_grid_button(int val)
{
	if (val == LP_BTN_SCENE_A || val == LP_BTN_SCENE_B || val == LP_BTN_SCENE_C || val == LP_BTN_SCENE_D || val == LP_BTN_SCENE_E || val == LP_BTN_SCENE_F || val == LP_BTN_SCENE_G || val == LP_BTN_SCENE_H){
		return 0;
	} else {
		return 1;
	}
}

void lp_resetall()
{
	unsigned char buf[3];
	buf[0] = 0xB0;
	buf[1] = 0x00;
	buf[2] = 0x00;
	midi_send_now_launchpad((unsigned char *)buf, 3); 
	midi_send_flush();
}

void lp_draw_grid(int num, int color)
{
	if (num < 1)
		return;
		
	if (num > 64)
		num = 64;
	
	for (int i=0; i < num; i++){	
		lp_set_grid_led(i,color);
	}	
}

void lp_initialize()
{
	/* Called after LP has been found */
	if (!lp_port)
		return;
		
	lp_resetall();
}

void lp_set_grid_led(int num, int color)
{
	unsigned char buf[3];
	buf[0] = 0x90; //Note on = LED on
	buf[1] = lp_grid_buttoncode(num);
	buf[2] = color;
	midi_send_now_launchpad((unsigned char *)buf, 3);
	midi_send_flush();	
}

void lp_set_loop_start(int order);
void lp_set_loop_end(int order);

void lp_update_grid(){
	log_appendf(3,"LP update grid");
	log_appendf(3,"numorders %d",csf_get_num_orders(current_song));
	/* Light up order leds for LP */
	lp_draw_grid(csf_get_num_orders(current_song),LP_LED_AMBER_LOW);
	if (current_song->process_order != song_get_current_order()){
		lp_set_grid_led(current_song->process_order+1,LP_LED_GREEN_FLASH);
	}
}
