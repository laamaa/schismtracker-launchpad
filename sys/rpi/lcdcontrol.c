#ifdef LCD
#include "headers.h"
#include "it.h"
#include "lcdcontrol.h"
#include "song.h"
#include "sndfile.h"
#include "log.h"
#include "page.h"

#include "ArduiPi_OLED_lib.h"
#include "ArduiPi_OLED_C.h"

/*used by lcd thread only */
typedef struct lcd_playhead {
	int current_row;
	int current_order;
	int max_orders;
	int player_state;
	int current_file;
	int current_page;
} lcd_playhead;

/* main.c creates this as a thread on startup */
void *lcd_update()
{
	lcd_playhead ph;
	ph.current_row = -1;
	ph.current_order = -1;
	ph.max_orders = -1;
	ph.player_state = -1;
	ph.current_file = -1;

	if( !PiOLED_Init(OLED_SH1106_I2C_128x64, "/dev/i2c-1") )
		exit(EXIT_FAILURE);

    PiOLED_SetTextSize(2);
    PiOLED_SetTextColor(WHITE);
	
	do {
		switch (status.current_page) {
			case PAGE_ABOUT:
				PiOLED_ClearDisplay();
				PiOLED_SetCursor(0,0);
				PiOLED_Print("Hello Schism!");
				PiOLED_Display();
				break;
			case PAGE_LOAD_MODULE:
				if (get_current_file() != ph.current_file) {
					ph.current_file = get_current_file();
					PiOLED_ClearDisplay();
					PiOLED_SetCursor(0,0);
					PiOLED_Print("Load file:\n");
					PiOLED_Print(get_current_filename());
					PiOLED_Display();
				}
				break;
			default:
				if (song_get_mode() == MODE_PLAYING || MODE_PATTERN_LOOP || MODE_SINGLE_STEP) {
					ph.current_order = song_get_current_order();
					ph.current_row = song_get_current_row();
					PiOLED_ClearDisplay();
					PiOLED_FillTriangle(0,0,16,8,0,16,WHITE);
					PiOLED_SetCursor(0,20);
					PiOLED_SetTextSize(1);
					PiOLED_Printf("%02d/",ph.current_order);
					PiOLED_SetCursor(16,20);
					PiOLED_Printf("%02d",csf_get_num_orders(current_song));
					PiOLED_SetCursor(56,20);
					PiOLED_Printf("%03d/",ph.current_row);
					PiOLED_SetCursor(80,20);
					PiOLED_Print("xxx");
					PiOLED_Display();
				} else {
					PiOLED_FillRect(0,0,16,16,WHITE);
				}
				break;
		}		
		usleep(1000);
	} while (1);
}
#endif
