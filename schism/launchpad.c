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

int lp_is_hex_code_grid_button(int val) {
	if (val == LP_BTN_SCENE_A || val == LP_BTN_SCENE_B || val == LP_BTN_SCENE_C || val == LP_BTN_SCENE_D || val == LP_BTN_SCENE_E || val == LP_BTN_SCENE_F || val == LP_BTN_SCENE_G || val == LP_BTN_SCENE_H){
		return 0;
	} else {
		return 1;
	}
}
