#include "headers.h"
#include "launchpad.h"
#include "log.h"
#include "midi.h"
#include "sndfile.h"
#include "song.h"
#include "it.h"
#include "page.h"
#include "time.h"
#include <stdio.h>

typedef struct lp_loop {
	int start;
	int end;
	int active;
} lp_loop;

lp_loop loop = {-1,-1,0};
int active_order = 0;
int queued_order = -1;
int lp_port;

double get_time_ms()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (t.tv_sec + (t.tv_usec / 1000000.0)) * 1000.0;
}

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

void lp_check_loop_state()
{
	if (loop.start != -1 && loop.end != -1) {
		if (song_get_current_order() == loop.start) {
			loop.active = 1;
		} else {
			/* If the loop is , check if we're approaching the end */
			if (queued_order != loop.start && song_get_current_order() == loop.end) {
				queued_order = loop.start;
				song_set_next_order(loop.start);
			}
		}
	} 
}

void lp_check_active_order()
{
	lp_check_loop_state();
	if (active_order != song_get_current_order()) {
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

void lp_draw_grid(int start, int num, int color)
{
	if (start < 0 || start > 63)
		return;
		
	if (start+num < 1)
		return;
		
	if (start+num > 64)
		num = 64-start;
	
	for (int i=start; i < start+num; i++){	
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

void lp_update_grid()
{
	int num_files;
	switch (status.current_page) {
		case PAGE_ABOUT:
			break;
		case PAGE_LOAD_MODULE:
			num_files = get_flist_num_files();
			lp_draw_grid(0,num_files,LP_LED_RED_LOW);
			lp_set_grid_led(get_current_file(),LP_LED_GREEN_FULL);
			lp_set_led(LP_BTN_SCENE_F,LP_LED_RED_FULL);
			break;
		default:
			/* Light up order leds for LP */
			lp_draw_grid(0,csf_get_num_orders(current_song),LP_LED_AMBER_LOW);
			if (loop.start != -1 && loop.end != -1)
				lp_draw_grid(loop.start,loop.end-loop.start+1,LP_LED_YELLOW_FLASH);
			lp_set_grid_led(active_order,LP_LED_GREEN_FULL);
			if (active_order == queued_order)
				queued_order = -1;			
			if (queued_order > -1 && loop.start == -1)
				lp_set_grid_led(queued_order,LP_LED_GREEN_FLASH);
			if (song_get_mode() == MODE_STOPPED)
				lp_set_led(LP_BTN_SCENE_H,LP_LED_RED_FULL); // Because we reset the controller when switching pages, we need to check this as well...
			else
				lp_set_led(LP_BTN_SCENE_H,LP_LED_GREEN_FULL);
			break;
	}
}

void lp_handle_midi(int *st)
{
	static int lp_grid_buttons_down[64];
	static int previous_message[2];
	static double debounce_start, debounce_end = 0;
	static int loop_created = 0;
	
	if (debounce_start == 0)
		debounce_start = get_time_ms();
		
	if (previous_message[0] == st[0] && previous_message[1] == st[2])
	{
		debounce_end = get_time_ms();
		if (debounce_end-debounce_start < 20){
			debounce_start=0;
			return;
		}
	}
	
	previous_message[0] = st[0];
	previous_message[1] = st[2];
	
	if (st[0] == MIDI_NOTEON) {
		loop_created = 0;
		if (lp_is_hex_code_grid_button(st[2]) == 1) {
			/* A grid button is pressed */
			if (status.current_page == PAGE_LOAD_MODULE) {
				/* On load module page, change the selected file according to pressed grid button */
				set_current_file(lp_grid_button_hex_to_int(st[2]));
				lp_set_led(st[2],LP_LED_GREEN_LOW);
			} else {
				if (song_get_mode() == MODE_STOPPED) {
					/* If song is stopped, set the starting order */
					song_set_current_order(lp_grid_button_hex_to_int(st[2]));
					queued_order = lp_grid_button_hex_to_int(st[2]);
					lp_update_grid();
				} else {
					/* Check if there is more than one button pressed and enable loop if so */
					for (int i=0;i<64;i++) {
						if (lp_grid_buttons_down[i] == 1) {
							if (i > lp_grid_button_hex_to_int(st[2])) {
								loop.start = lp_grid_button_hex_to_int(st[2]);
								loop.end = i;
							} else {
								loop.start = i;
								loop.end = lp_grid_button_hex_to_int(st[2]);
							}
							if (song_get_current_order == loop.start) {
								loop.active = 1;
								log_appendf(3,"loop activated");
							}
							log_appendf(3,"loop start: %d",loop.start);
							log_appendf(3,"loop end: %d",loop.end);
							lp_draw_grid(loop.start,loop.end-loop.start+1,LP_LED_YELLOW_FLASH);
							lp_set_grid_led(active_order,LP_LED_GREEN_FULL);
							song_set_next_order(loop.start);
							queued_order = loop.start;
							loop_created = 1;
							break;
						}
					}
					lp_grid_buttons_down[lp_grid_button_hex_to_int(st[2])] = 1; // Save pressed buttons in an array
				}
			}
		} else {
			/* A scene button is pressed */
			switch (st[2]) {
				case LP_BTN_SCENE_H:
					if (status.current_page == PAGE_ABOUT || status.current_page == PAGE_LOAD_MODULE) {
						push_keyboard_enter_event();
					} else {
						/* Playback & Looping control */
						if (song_get_mode() == MODE_PLAYING) {
							song_stop();
							lp_set_led(LP_BTN_SCENE_H,LP_LED_RED_FULL);
						} else if (song_get_mode() == MODE_STOPPED || song_get_mode() == MODE_SINGLE_STEP) {
							if (queued_order > -1) {
								song_start_at_order(queued_order,0);
							} else {
								song_start_at_order(song_get_current_order(),0);
							}
							lp_set_led(LP_BTN_SCENE_H,LP_LED_GREEN_FULL);
						}
					}
					break;
				case LP_BTN_SCENE_G:
					/* TODO Song repeat mode: infinite / repeat pattern */
					break;
				case LP_BTN_SCENE_F:
					if (status.current_page == PAGE_LOAD_MODULE)
						set_page(status.previous_page);
					else
						set_page(PAGE_LOAD_MODULE);
					lp_update_grid();
					break;
				case LP_BTN_SCENE_A:
					/* TODO Shift button: solo channels instead of muting */
					break;
			}
		}
	} else {
		/* Note off message, queue the order */
		if (lp_is_hex_code_grid_button(st[2]) == 1 && status.current_page != PAGE_LOAD_MODULE) {
			if (loop_created == 0) {
				int next_order = lp_grid_button_hex_to_int(st[2]);
				song_set_next_order(next_order);
				queued_order = next_order;
				log_appendf(3,"queued order: %d",next_order);
				if (loop.start != -1) {						
					loop.active = 0;
					loop.start = -1;
					loop.end = -1;
					log_appendf(3,"loop deactivated");
				}
			}
			lp_grid_buttons_down[lp_grid_button_hex_to_int(st[2])] = 0;
		}
	}
}
