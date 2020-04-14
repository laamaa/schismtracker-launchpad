#include "launchpad.h"

int lp_grid_buttoncode(int val) {
	if (val >= 0 && val < 64) {
		int ledRow;
		ledRow = val/8;
		return 0x00+(0x10*ledRow)+(val-(ledRow*8));
	}
	return 0;
}

int lp_grid_button_hex_to_int(int val) {
	int ledRow;
	ledRow = val/0x10;
	return val-(ledRow*8);
}

