#include "headers.h"
#include "it.h"
#include "lcdcontrol.h"
#include "song.h"
#include "sndfile.h"
#include "log.h"
#include "page.h"

#ifdef LCD
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"
#endif

/*used by lcd thread only */
typedef struct lcd_playhead {
	int current_row;
	int current_order;
	int max_orders;
	int player_state;
	int current_file;
} lcd_playhead;

void lcd_print(int x, int y, char* msg)
{
	#ifdef LCD
	
	#else
		log_appendf(3,"%s",msg);
	#endif
}

/* main.c creates this as a thread on startup */
void *lcd_update()
{
	lcd_playhead ph;
	ph.current_row = -1;
	ph.current_order = -1;
	ph.max_orders = -1;
	ph.player_state = -1;
	ph.current_file = -1;
	
	do {
		switch (status.current_page) {
			case PAGE_ABOUT:
				lcd_print(0,0,"Howdy!");
				break;
			case PAGE_LOAD_MODULE:
				if (get_current_file() != ph.current_file) {
					ph.current_file = get_current_file();
					lcd_print(0,0,get_current_filename());
				}
				break;
			default:			
				if (song_get_mode() != ph.player_state) {
					ph.player_state = song_get_mode();
					switch (ph.player_state) {
						case MODE_PLAYING:
							lcd_print(0,0,"play");
							break;
						case MODE_STOPPED:
							lcd_print(0,0,"stop");
							break;
						case MODE_PATTERN_LOOP:
							lcd_print(0,0,"1ptn");
							break;
						case MODE_SINGLE_STEP:
							lcd_print(0,0,"singlestep");
							break;
					}
				}
				if (song_get_mode() == MODE_PLAYING) {
					if (song_get_current_order() != ph.current_order) {
						ph.current_order = song_get_current_order();
						lcd_print(0,0,"order updated");
					}
					
					if (song_get_current_row() != ph.current_row) {
						ph.current_row = song_get_current_row();
						lcd_print(0,0,"row updated");
					}						
				}
				break;
		}
		usleep(1000);
	} while (1);
}
