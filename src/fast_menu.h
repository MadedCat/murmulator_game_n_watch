#pragma once
#include "inttypes.h"
#include <pico/stdlib.h>

#define FAST_MENU_MAIN 0

#define FAST_SUBMENU_LINES 10


#define FAST_MENU_COUNT 1
#define FAST_MENU_LINES 11


#define FAST_MAIN_MANAGER		(0)
#define FAST_GAME_A				(1)
#define FAST_GAME_B				(2)
#define FAST_TIME				(3)
#define FAST_ALARM				(4)
#define FAST_MAIN_HELP			(5)
#define FAST_MAIN_SETTINGS		(6)
#define FAST_MAIN_SOFT_RESET	(7)
#define FAST_MAIN_HARD_RESET	(8)


const uint8_t __in_flash() *fast_menu_lines[FAST_MENU_COUNT]={(uint8_t*)9}; //,(uint8_t*)5,(uint8_t*)11,(uint8_t*)6,(uint8_t*)9
const char __in_flash() *fast_menu[FAST_MENU_COUNT][FAST_MENU_LINES]={
	{ 
		" [FILE BROWSER] ",
		"     Game A     ",
		"     Game B     ",
		"      TIME      ",
		"     ALARM      ",
		"     [HELP]     ",
		"   [SETTINGS]   ",
		"   Soft reset   ",
		"   Hard reset   ",
		"\0",
	}
};