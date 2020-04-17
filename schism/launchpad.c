#include "headers.h"
#include "launchpad.h"
#include "log.h"
#include "midi.h"
#include "sndfile.h"
#include "song.h"
#include "it.h"
#include "page.h"
#include <stdio.h>

int loop_start;
int loop_end;
int active_order = 0;
int queued_order = -1;
int lp_port;

int lp_get_port()
{
	return lp_port;
}

void lp_set_port(int num)
{
	if (num > -1) lp_port = num;
}

void push_keyboard_enter_event()
{
	SDL_Event sdlevent = {};
	sdlevent.type = SDL_KEYDOWN;
	sdlevent.key.keysym.sym = SDLK_RETURN;
	SDL_PushEvent(&sdlevent);
	sdlevent.type = SDL_KEYUP;
	sdlevent.key.keysym.sym = SDLK_RETURN;
	SDL_PushEvent(&sdlevent);
}

void lp_check_active_order()
{
	if (active_order != song_get_current_order()){
		active_order = song_get_current_order();
		if (status.lp_flags & LP_UPDATE_GRID == LP_UPDATE_GRID)
			status.lp_flags &= ~(LP_UPDATE_GRID);
		lp_update_grid();
	}
}

/* Return the hex code for a LP grid button, values 1-64 */
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

void lp_set_led(int num, int color)
{
	unsigned char buf[3];
	buf[0] = 0x90; //Note on = LED on
	buf[1] = num;
	buf[2] = color;
	midi_send_now_launchpad((unsigned char *)buf, 3);
	midi_send_flush();
}

void lp_resetall()
{
	unsigned char buf[3];
	/* This sequence resets Launchpad */
	buf[0] = 0xB0;
	buf[1] = 0x00;
	buf[2] = 0x00;
	midi_send_now_launchpad((unsigned char *)buf, 3); 
	midi_send_flush();
	/* This sequence sets buffer mode to enable flashing */
	buf[0] = 0xB0;
	buf[1] = 0x00;
	buf[2] = 0x28;
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
	lp_set_led(LP_BTN_SCENE_H,LP_LED_RED_LOW);
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
/* TODO */
void lp_set_loop_start(int order);
void lp_set_loop_end(int order);

void lp_update_grid()
{
	if (status.current_page == PAGE_LOAD_MODULE) {
		/* TODO */
		int num_files = get_flist_num_files();
		lp_draw_grid(num_files,LP_LED_RED_LOW);
		lp_set_grid_led(get_current_file(),LP_LED_GREEN_FULL);
	} else {
		/* Light up order leds for LP */
		lp_draw_grid(csf_get_num_orders(current_song),LP_LED_AMBER_LOW);
		if (queued_order > -1)
			lp_set_grid_led(queued_order,LP_LED_GREEN_FLASH);
		lp_set_grid_led(active_order,LP_LED_GREEN_FULL);
		if (active_order == queued_order)
			queued_order = -1;
	}
}

void lp_handle_midi(int *st)
{
	if (st[0] == MIDI_NOTEON) {
		/* Save pressed buttons in array */
		lp_grid_buttons_down[lp_grid_button_hex_to_int(st[2])] = 1;
		if (lp_is_hex_code_grid_button(st[2]) == 1) {
			/* Grid button is pressed */
			if (status.current_page == PAGE_LOAD_MODULE) {
				set_current_file(lp_grid_button_hex_to_int(st[2]));
				lp_set_led(st[2],LP_LED_GREEN_LOW);
			} else {
				if (song_get_mode() == MODE_STOPPED) {
					song_set_current_order(lp_grid_button_hex_to_int(st[2]));
					queued_order = lp_grid_button_hex_to_int(st[2]);
					lp_update_grid();
				} else {
					int next_order = lp_grid_button_hex_to_int(st[2]);
					song_set_next_order(next_order);
					queued_order = next_order;
				}
			}
		} else {
			if (st[2] == LP_BTN_SCENE_H){
				if (status.current_page == PAGE_ABOUT || status.current_page == PAGE_LOAD_MODULE) {
					push_keyboard_enter_event();
				} else {
					/* Playback & Looping control */
					if (song_get_mode() == MODE_PLAYING) {
						song_stop();
						lp_set_led(LP_BTN_SCENE_H,LP_LED_RED_LOW);
					} else if (song_get_mode() == MODE_STOPPED || song_get_mode() == MODE_SINGLE_STEP) {
						if (queued_order > -1) {
							song_start_at_order(queued_order,0);
						} else {
							song_start_at_order(song_get_current_order(),0);
						}
						lp_set_led(LP_BTN_SCENE_H,LP_LED_GREEN_FULL);
					}
				}
			}
		}
	} else {
		/* Note off message */
		lp_grid_buttons_down[lp_grid_button_hex_to_int(st[2])] = 0;
	}
}
