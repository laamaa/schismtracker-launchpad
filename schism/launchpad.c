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
#include <math.h>

/* TODO:
 * - Song changes, reset loops, mutes and lp_state
 * - Put mutes/solos to their own grid view instead of top row
 */

enum lp_view
{
	about,
	orders,
	channels,
	files,
	sampler,
	phraser
};

typedef struct lp_loop
{
	int start;
	int end;
	int active;
} lp_loop;

typedef struct lp_mute
{
	int active;
	int channels[128];
} lp_mute;

typedef struct lp_state
{
	int midiport;
	enum lp_view active_view;
	int active_order;
	int queued_order;
} lp_state;

lp_state state = {-1, about, 0, -1};
lp_loop loop = {-1, -1, 0};
lp_mute mute = {0, {0}};
int active_order = 0;
int queued_order = -1;

const int lp_vu_colors[8] = {LP_LED_OFF, LP_LED_GREEN_LOW, LP_LED_GREEN_FULL, LP_LED_YELLOW_FULL, LP_LED_AMBER_LOW, LP_LED_AMBER_FULL, LP_LED_RED_LOW, LP_LED_RED_FULL};

double _get_time_ms()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (t.tv_sec + (t.tv_usec / 1000000.0)) * 1000.0;
}

int lp_get_port()
{
	return state.midiport;
}

void lp_set_port(int num)
{
	if (num > -1)
		state.midiport = num;
}

void _push_keyboard_enter_event()
{
	SDL_Event sdlevent = {};
	sdlevent.type = SDL_KEYDOWN;
	sdlevent.key.keysym.sym = SDLK_RETURN;
	SDL_PushEvent(&sdlevent);
	sdlevent.type = SDL_KEYUP;
	sdlevent.key.keysym.sym = SDLK_RETURN;
	SDL_PushEvent(&sdlevent);
}

/* Return the hex code for a LP grid button, values 1-64 */
int lp_grid_buttoncode(int val)
{
	if (val >= 0 && val < 64)
	{
		int ledRow;
		ledRow = val / 8;
		return 0x00 + (0x10 * ledRow) + (val - (ledRow * 8));
	}
	return 0;
}

int lp_grid_button_hex_to_int(int val)
{
	int ledRow;
	ledRow = val / 0x10;
	return val - (ledRow * 8);
}

int lp_is_hex_code_grid_button(int val)
{
	if (val == LP_BTN_SCENE_A || val == LP_BTN_SCENE_B || val == LP_BTN_SCENE_C || val == LP_BTN_SCENE_D || val == LP_BTN_SCENE_E || val == LP_BTN_SCENE_F || val == LP_BTN_SCENE_G || val == LP_BTN_SCENE_H)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void lp_check_loop_state()
{
	if (loop.start != -1 && loop.end != -1)
	{
		//log_appendf(3,"loop start: %d, loop end: %d", loop.start, loop.end);
		if (song_get_current_order() == loop.start)
		{
			loop.active = 1;
		}
		else
		{
			/* If the loop is , check if we're approaching the end */
			if (queued_order != loop.start && song_get_current_order() == loop.end)
			{
				//log_appendf(3,"Loop end, setting next order %d", loop.start);
				state.queued_order = loop.start;
				song_set_next_order(loop.start);
			}
		}
	}
}

void lp_set_automap_led(int num, int color)
{
	if (num < 0 || num > 7)
		return;
	unsigned char buf[3];
	buf[0] = 0xB0;
	buf[1] = 0x68 + num;
	buf[2] = color;
	midi_send_now_launchpad((unsigned char *)buf, 3);
	midi_send_flush();
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

/* LP automap leds (top row 1-8 btns) act as VU meters and mute/solo buttons */
void lp_update_vu_meters()
{
	static double vu_timer_start = 0;
	if (vu_timer_start = 0)
		vu_timer_start = _get_time_ms();
	if ((song_get_mode() == (MODE_PLAYING || MODE_PATTERN_LOOP || MODE_SINGLE_STEP)) && _get_time_ms() - vu_timer_start > 100)
	{
		vu_timer_start = 0;
		for (int i = 0; i < 8; i++)
		{
			song_voice_t *voice = current_song->voices + i;
			if (!(voice->current_sample_data && voice->length))
				continue;
			int vu = voice->vu_meter / 16;
			if (vu > 0 && vu < 1)
				vu = 1;
			if (vu > 7)
				vu = 7;
			if (current_song->channels[i].flags & CHN_MUTE)
				vu = -1; //Muted channels should be flashing red
			if (vu != -1)
				lp_set_automap_led(i, lp_vu_colors[vu]);
		}
	}
}

/* Checks if the song has advanced to the next order and the grid should be updated */
void lp_check_active_order()
{
	if (state.active_order != song_get_current_order())
	{
		if (song_get_current_order() == state.queued_order)
			state.queued_order = -1;
		if (status.current_page != PAGE_LOAD_MODULE)
			lp_set_grid_led(state.active_order, (loop.active == 1 && state.active_order >= loop.start && state.active_order <= loop.end) ? LP_LED_YELLOW_FLASH : LP_LED_AMBER_LOW);
		state.active_order = song_get_current_order();
		lp_check_loop_state();
		if (status.current_page != PAGE_LOAD_MODULE)
			lp_set_grid_led(state.active_order, LP_LED_GREEN_FULL);
		if (status.lp_flags & LP_UPDATE_GRID == LP_UPDATE_GRID)
			status.lp_flags &= ~(LP_UPDATE_GRID);
		//lp_update_grid();
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

	if (start + num < 1)
		return;

	if (start + num > 64)
		num = 64 - start;

	for (int i = start; i < start + num; i++)
	{
		lp_set_grid_led(i, color);
	}
}

void lp_initialize()
{
	/* Called after LP has been found */
	if (!state.midiport)
		return;

	lp_resetall();
	lp_set_led(LP_BTN_SCENE_H, LP_LED_RED_LOW);
}

void lp_turn_on_all_leds()
{
	unsigned char buf[3];
	buf[0] = 0xB0;
	buf[1] = 0x00;
	buf[2] = 0x7D;
	midi_send_now_launchpad((unsigned char *)buf, 3);
	midi_send_flush();
}

void lp_update_grid()
{
	switch (status.current_page)
	{
	case PAGE_ABOUT:
		/* On startup, just turn on all leds */
		state.active_view = about;
		lp_turn_on_all_leds();
		break;
	case PAGE_LOAD_MODULE:
	{
		int num_files;
		/* Draw file browsing grid */
		state.active_view = files;
		num_files = get_flist_num_files();
		lp_draw_grid(0, num_files, LP_LED_RED_LOW);
		lp_set_grid_led(get_current_file(), LP_LED_GREEN_FULL);
		lp_set_led(LP_BTN_SCENE_F, LP_LED_RED_FULL);
		break;
	}
	default:
		/* Light up order leds for LP */
		lp_draw_grid(0, csf_get_num_orders(current_song), LP_LED_AMBER_LOW);
		if (loop.start != -1 && loop.end != -1)
			lp_draw_grid(loop.start, loop.end - loop.start + 1, LP_LED_YELLOW_FLASH);
		lp_set_grid_led(state.active_order, LP_LED_GREEN_FULL);
		if (state.active_order == state.queued_order)
			state.queued_order = -1;
		if (state.queued_order > -1 && loop.start == -1)
			lp_set_grid_led(state.queued_order, LP_LED_GREEN_FLASH);
		/* Because the scene leds are also reset when switching between views, we need to explicitly set them on update */
		if (mute.active)
			lp_set_led(LP_BTN_SCENE_A, LP_LED_RED_FLASH);
		if (current_song->flags & SONG_ORDERLOCKED)
			lp_set_led(LP_BTN_SCENE_G, LP_LED_AMBER_FLASH);
		if (song_get_mode() == MODE_STOPPED)
			lp_set_led(LP_BTN_SCENE_H, LP_LED_RED_FULL);
		else
			lp_set_led(LP_BTN_SCENE_H, LP_LED_GREEN_FULL);
		break;
	}
}

int lp_is_mute_active()
{
	int count = 0;
	for (int i = 0; i < 128; i++)
		if (mute.channels[i] == 1)
			return 1;
	return 0;
}

void lp_unmute_all()
{
	mute.active = 0;
	for (int i = 0; i < 128; i++)
		if (mute.channels[i] == 1)
			song_toggle_channel_mute(i);
	orderpan_recheck_muted_channels();
}

/* This handles LP top row buttons: mute / solo */
void lp_handle_midi_cc(int *st)
{
	//0x68 = first button, 0x6f = last button
	if (st[2] >= 0x68 && st[2] <= 0x6f && st[0] == 0x7f)
	{
		int channel = st[2] - 0x68;
		song_toggle_channel_mute(channel);
		mute.channels[channel] = !mute.channels[channel];
		orderpan_recheck_muted_channels();
		if (lp_is_mute_active())
		{
			mute.active = 1;
			lp_set_led(LP_BTN_SCENE_A, LP_LED_RED_FLASH);
		}
		lp_set_automap_led(st[2] - 0x68, LP_LED_RED_FLASH); //we can set the led to flashing red in either case, VU update will override if it when unmuted
	}
}

/* A grid button is pressed */
static void lp_handle_grid_button_noteon(int *st, int *loop_created, int (*lp_grid_buttons_down)[64])
{
	*loop_created = 0;
	int button = lp_grid_button_hex_to_int(st[2]);
	switch (status.current_page)
	{
	case PAGE_LOAD_MODULE:
		/* On load module page, change the selected file according to pressed grid button */
		if (button < get_flist_num_files())
		{
			set_current_file(button);
			lp_set_led(st[2], LP_LED_GREEN_LOW);
		}
		break;

	default:
		if (song_get_mode() == MODE_STOPPED)
		{
			/* If song is stopped, set the starting order */
			if (button < csf_get_num_orders(current_song))
			{
				song_set_current_order(button);
				state.queued_order = button;
				lp_set_grid_led(state.queued_order, LP_LED_GREEN_FLASH);
			}
		}
		else
		{
			/* Song is playing */
			for (int i = 0; i < 64; i++)
			{
				/* Check if there is more than one button pressed and enable loop if so */
				if (button < csf_get_num_orders(current_song) && *lp_grid_buttons_down[i] == 1)
				{
					/* Check if there is already a loop defined and make the buttons stop blinking */
					if (loop.start != -1 && loop.end != -1)
						lp_draw_grid(loop.start, loop.end - loop.start + 1, LP_LED_AMBER_LOW);
					/* The two buttons might be pressed in either order, check that */
					if (i > button)
					{
						loop.start = button;
						loop.end = i;
					}
					else
					{
						loop.start = i;
						loop.end = button;
					}
					/* If we're already in the loop start pattern, mark it active */
					if (song_get_current_order() == loop.start)
					{
						loop.active = 1;
						song_set_next_order(loop.start + 1);
					}
					else
					{
						song_set_next_order(loop.start);
					}

					lp_draw_grid(loop.start, loop.end - loop.start + 1, LP_LED_YELLOW_FLASH);
					lp_set_grid_led(state.active_order, LP_LED_GREEN_FULL);

					state.queued_order = loop.start;
					*loop_created = 1;
					break;
				}
			}
			*lp_grid_buttons_down[button] = 1; // Save pressed buttons in an array
		}
		break;
	}
}

static void lp_handle_grid_button_noteoff(int *st, int *loop_created, int (*lp_grid_buttons_down)[64])
{
	int button = lp_grid_button_hex_to_int(st[2]);
	if (button < csf_get_num_orders(current_song))
	{
		if (*loop_created == 0)
		{
			/* There is no loop currently, queue the next order */
			song_set_next_order(button);
			if (state.queued_order != -1)
				lp_set_grid_led(state.queued_order, LP_LED_AMBER_LOW);
			state.queued_order = button;
			lp_set_grid_led(state.queued_order, LP_LED_GREEN_FLASH);
			if (loop.start != -1)
			{
				lp_draw_grid(loop.start, loop.end - loop.start + 1, LP_LED_AMBER_LOW);
				loop.active = 0;
				loop.start = -1;
				loop.end = -1;
			}
		}
	}
	*lp_grid_buttons_down[button] = 0;
}

static void lp_handle_scene_button_noteon(int *st)
{
	switch (st[2])
	{
	case LP_BTN_SCENE_H:
		if (status.current_page == PAGE_LOAD_MODULE)
		{
			_push_keyboard_enter_event();
			return;
		}
		/* Playback & Looping control */
		if (song_get_mode() == MODE_PLAYING)
		{
			song_stop();
			lp_set_led(LP_BTN_SCENE_H, LP_LED_RED_FULL);
		}
		else if (song_get_mode() == (MODE_STOPPED || MODE_SINGLE_STEP))
		{
			if (state.queued_order > -1)
			{
				song_start_at_order(state.queued_order, 0);
			}
			else
			{
				song_start_at_order(song_get_current_order(), 0);
			}
			lp_set_led(LP_BTN_SCENE_H, LP_LED_GREEN_FULL);
		}
		break;
	case LP_BTN_SCENE_G:
		/* Playback mode: normal / loop single pattern */
		if (current_song->flags & SONG_ORDERLOCKED)
		{
			current_song->flags &= ~SONG_ORDERLOCKED;
			lp_set_led(LP_BTN_SCENE_G, LP_LED_OFF);
		}
		else
		{
			current_song->flags |= SONG_ORDERLOCKED;
			lp_set_led(LP_BTN_SCENE_G, LP_LED_AMBER_FLASH);
		}
		break;
	case LP_BTN_SCENE_F:
		if (status.current_page == PAGE_LOAD_MODULE)
		{
			set_page(status.previous_page);
		}
		else
		{
			set_page(PAGE_LOAD_MODULE);
		}
		lp_update_grid();
		break;
	case LP_BTN_SCENE_C:
		/* Phrase mode, play oneshot orders */
		if (song_get_mode() == (MODE_STOPPED || MODE_SINGLE_STEP))
		{
			// TO DO
		}
		break;
	case LP_BTN_SCENE_A:
		/* If mute is active, unmute all */
		if (mute.active)
		{
			lp_unmute_all();
			lp_set_led(LP_BTN_SCENE_A, LP_LED_OFF);
		}
		break;
	}
}

/* This handles LP grid button presses */
void lp_handle_midi(int *st)
{
	static int lp_grid_buttons_down[64];
	static int loop_created = 0;

	if (st[0] == MIDI_NOTEON)
	{
		if (status.current_page == PAGE_ABOUT)
		{
			/* Pressing any button exits the about screen */
			_push_keyboard_enter_event();
			return;
		}

		if (lp_is_hex_code_grid_button(st[2]) == 1)
		{
			/* A button in the 8x8 grid is pressed */
			lp_handle_grid_button_noteon(st, &loop_created, &lp_grid_buttons_down);
		}
		else
		{
			/* A scene button is pressed */
			lp_handle_scene_button_noteon(st);
		}
	}
	else
	{
		/* The received message is a note off message -> key lifted */
		if (lp_is_hex_code_grid_button(st[2]) == 1 && status.current_page != PAGE_LOAD_MODULE)
		{
			/* Note off message, triggers order queuing etc. */
			lp_handle_grid_button_noteoff(st, &loop_created, &lp_grid_buttons_down);
		}
	}
}
