/*
This file is part of "UFO 2000" aka "X-COM: Gladiators"
                    http://ufo2000.sourceforge.net/
Copyright (C) 2000-2001  Alexander Ivanov aka Sanami
Copyright (C) 2002-2004  ufo2000 development team

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"

#include "global.h"
#include "spk.h"
#include "video.h"
#include "config.h"
#include "multiplay.h"
#include "colors.h"

extern void install_timers(int _speed_unit, int _speed_bullet, int _speed_mapscroll);
extern void uninstall_timers();
extern int speed_unit;
extern int speed_bullet;
extern int speed_mapscroll;
extern int mapscroll;

static char *gen = "General";
static char *edit = "Editor";

static int base_accuracy = 75;
static int preferred_screen_xres = 640;
static int preferred_screen_yres = 400;
static int min_color_depth = 8;
static std::string menu_midi_file_name;
static std::string setup_midi_file_name;
static std::string editor_midi_file_name;
static std::string combat1_midi_file_name;
static std::string combat2_midi_file_name;
static std::string win_midi_file_name;
static std::string lose_midi_file_name;
static std::string net1_midi_file_name;
static std::string net2_midi_file_name;

static std::string loading_image_file_name;
static std::string menu_image_file_name;
static std::string endturn_image_file_name;
static std::string win_image_file_name;
static std::string lose_image_file_name;

std::string g_server_host;
std::string g_server_login;
std::string g_server_password;
std::string g_server_proxy;
int         g_server_autologin;

static std::string consolefont;
static int music_volume;

FONT *cfg_get_console_font()
{
	FONT *fnt = (SCREEN_W >= 800) ? large : g_small_font;

	if (consolefont == "xcom_small") {
		fnt = g_small_font;
	} else if (consolefont == "xcom_large") {
		fnt = large;
	}

	return fnt;
}

int cfg_get_music_volume()
{
	return music_volume;
}

int cfg_get_base_accuracy()
{
	return base_accuracy;
}

int cfg_get_screen_x_res()
{
	return preferred_screen_xres;
}

int cfg_get_screen_y_res()
{
	return preferred_screen_yres;
}

int cfg_get_min_color_depth()
{
	return min_color_depth;
}

const std::string &cfg_get_server_host()
{
	return g_server_host;
}

const std::string &cfg_get_server_login()
{
	return g_server_login;
}

const std::string &cfg_get_server_password()
{
	return g_server_password;
}

const std::string &cfg_get_server_proxy()
{
	return g_server_proxy;
}

const char *cfg_get_menu_music_file_name()
{
	return menu_midi_file_name.c_str();
}
 
const char *cfg_get_setup_music_file_name()
{
	return setup_midi_file_name.c_str();
}

const char *cfg_get_editor_music_file_name()
{
	return editor_midi_file_name.c_str();
}

const char *cfg_get_combat1_music_file_name()
{
	return combat1_midi_file_name.c_str();
}

const char *cfg_get_combat2_music_file_name()
{
	return combat2_midi_file_name.c_str();
}

const char *cfg_get_win_music_file_name()
{
	return win_midi_file_name.c_str();
}

const char *cfg_get_lose_music_file_name()
{
	return lose_midi_file_name.c_str();
}

const char *cfg_get_net1_music_file_name()
{
	return net1_midi_file_name.c_str();
}

const char *cfg_get_net2_music_file_name()
{
	return net2_midi_file_name.c_str();
}

const char *cfg_get_loading_image_file_name() { return loading_image_file_name.c_str(); }
const char *cfg_get_menu_image_file_name() { return menu_image_file_name.c_str(); }
const char *cfg_get_endturn_image_file_name() { return endturn_image_file_name.c_str(); }
const char *cfg_get_win_image_file_name() { return win_image_file_name.c_str(); }
const char *cfg_get_lose_image_file_name() { return lose_image_file_name.c_str(); }

void loadini()
{
	set_config_file(F("$(home)/ufo2000.ini"));

	SCREEN2W = get_config_int(gen, "width", 430);
	SCREEN2H = get_config_int(gen, "height", 280);

	MAP_WIDTH = get_config_int(gen, "map_width", 4);
	MAP_HEIGHT = get_config_int(gen, "map_height", 4);
	speed_unit = get_config_int(gen, "speed_unit", 30);
	speed_bullet = get_config_int(gen, "speed_bullet", 30);
	speed_mapscroll = get_config_int(gen, "speed_mapscroll", 30);
	mapscroll = get_config_int(gen, "mapscroll", 10);

	local_platoon_size = get_config_int(edit, "platoon_size", 1);
	strcpy(last_map_name, get_config_string(edit, "last_map_name", ""));
	strcpy(last_unit_name, get_config_string(edit, "last_unit_name", ""));

	g_time_limit = get_config_int(gen, "time_limit", -1);

//	base_accuracy = get_config_int(gen, "accuracy", 75);
	preferred_screen_xres = get_config_int(gen, "screen_x_res", 640);
	preferred_screen_yres = get_config_int(gen, "screen_y_res", 400);
	min_color_depth = get_config_int(gen, "color_depth", 8);
	menu_midi_file_name = get_config_string(gen, "menu_music", "$(xcom)/sound/gmstory.mid");
	setup_midi_file_name = get_config_string(gen, "setup_music", "$(xcom)/sound/gmenbase.mid");
	editor_midi_file_name = get_config_string(gen, "editor_music", "$(xcom)/sound/gmdefend.mid");
	combat1_midi_file_name = get_config_string(gen, "combat1_music", "$(xcom)/sound/gmtactic.mid");
	combat2_midi_file_name = get_config_string(gen, "combat2_music", "$(xcom)/sound/gmtactic.mid");
	win_midi_file_name = get_config_string(gen, "win_music", "$(xcom)/sound/gmwin.mid");
	lose_midi_file_name = get_config_string(gen, "lose_music", "$(xcom)/sound/gmlose.mid");
	net1_midi_file_name = get_config_string(gen, "net_music1", "$(xcom)/sound/gmgeo1.mid");
	net2_midi_file_name = get_config_string(gen, "net_music2", "$(xcom)/sound/gmgeo2.mid");

	loading_image_file_name = get_config_string(gen, "loading_image", "$(ufo2000)/arts/text_back.jpg");
	menu_image_file_name = get_config_string(gen, "menu_image", "$(xcom)/ufointro/pict2.lbm");
	endturn_image_file_name = get_config_string(gen, "endturn_image", "$(xcom)/ufograph/tac00.scr");
	win_image_file_name = get_config_string(gen, "win_image", "$(xcom)/geograph/back01.scr");
	lose_image_file_name = get_config_string(gen, "lose_image", "$(xcom)/geograph/back02.scr");

	consolefont = get_config_string(gen, "consolefont", "xcom_small");
	music_volume = get_config_int(gen, "music_volume", 255);

	g_server_host = get_config_string("Server", "host", "127.0.0.1");
	g_server_proxy = get_config_string("Server", "http_proxy", "auto");
	g_server_autologin = get_config_int("Server", "autologin", 0);
}

void saveini()
{
	set_config_file(F("$(home)/ufo2000.ini"));

	set_config_int(gen, "width", SCREEN2W);
	set_config_int(gen, "height", SCREEN2H);

	set_config_int(gen, "speed_unit", speed_unit);
	set_config_int(gen, "speed_bullet", speed_bullet);
	set_config_int(gen, "speed_mapscroll", speed_mapscroll);
	set_config_int(gen, "mapscroll", mapscroll);

	set_config_int(edit, "platoon_size", local_platoon_size);
	set_config_string(edit, "last_map_name", last_map_name);
	set_config_string(edit, "last_unit_name", last_unit_name);

	set_config_string("Server", "host", g_server_host.c_str());
	set_config_string("Server", "login", g_server_login.c_str());
	set_config_string("Server", "password", g_server_password.c_str());
	set_config_int("Server", "autologin", g_server_autologin);
}

int sethotseatplay()
{
	net->gametype = GAME_TYPE_HOTSEAT;
	return 1;
}

#define FG 0
#define BG 1

#define SPEED_UNIT       2
#define SPEED_BULLET     3
#define SPEED_MAPSCROLL  4
#define MAPSCROLL        5
#define OK_BUTTON        6
#define MAX_VALUE        99

static int d_slider_pro2(int msg, DIALOG *d, int c)
{
	char s[100];
	int v;
	v = d_slider_proc(msg, d, c);
	switch (msg) {
		case MSG_DRAW:
			sprintf(s, "%2d", d->d2);
			text_mode(d->bg);
			gui_textout(screen, s, d->x - 18, d->y + 4, d->fg, 0);
			break;
		default:
			break;
	}
	return v;
}

static DIALOG config_dlg[] = {
	{ d_shadow_box_proc, 0, 0, 320, 200, FG, BG, 0, 0, 0, 0, NULL, NULL, NULL },
	{ d_text_proc, 72, 8, 144, 16, FG, BG, 0, 0, 0, 0, (void *)"Configuration", NULL, NULL },
	{ d_slider_pro2, 24, 40, 136, 16, FG, BG, 0, 0, MAX_VALUE, 4, NULL, NULL, NULL },
	{ d_slider_pro2, 24, 64, 136, 16, FG, BG, 0, 0, MAX_VALUE, 4, NULL, NULL, NULL },
	{ d_slider_pro2, 24, 88, 136, 16, FG, BG, 0, 0, MAX_VALUE, 4, NULL, NULL, NULL },
	{ d_slider_pro2, 24, 112, 136, 16, FG, BG, 0, 0, MAX_VALUE, 4, NULL, NULL, NULL },
	{ d_button_proc, 160, 168, 64, 16, FG, BG, 0, D_EXIT, 0, 0, (void *)"OK", NULL, NULL },
	{ d_button_proc, 232, 168, 64, 16, FG, BG, 0, D_EXIT | D_GOTFOCUS, 0, 0, (void *)"Cancel", NULL, NULL },
	{ d_text_proc, 176, 44, 88, 16, FG, BG, 0, 0, 0, 0, (void *)"movement speed", NULL, NULL },
	{ d_text_proc, 176, 68, 104, 16, FG, BG, 0, 0, 0, 0, (void *)"fire speed", NULL, NULL },
	{ d_text_proc, 176, 92, 128, 16, FG, BG, 0, 0, 0, 0, (void *)"scroll speed", NULL, NULL },
	{ d_text_proc, 176, 116, 128, 16, FG, BG, 0, 0, 0, 0, (void *)"mapscroll points", NULL, NULL },
	{ d_yield_proc,           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL},
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

/**
 * Animation speed and other settings dialog available from battlescape
 */
void configure()
{
	config_dlg[SPEED_UNIT].d2 = speed_unit;
	config_dlg[SPEED_BULLET].d2 = speed_bullet;
	config_dlg[SPEED_MAPSCROLL].d2 = speed_mapscroll;
	config_dlg[MAPSCROLL].d2 = mapscroll;

	centre_dialog(config_dlg);
	set_mouse_range(0, 0, SCREEN_W - 1, SCREEN_H - 1);
	set_dialog_color(config_dlg, COLOR_BLACK1, COLOR_WHITE);

	if (popup_dialog(config_dlg, -1) == OK_BUTTON) {
		if (config_dlg[SPEED_UNIT].d2)
			speed_unit = config_dlg[SPEED_UNIT].d2;
		if (config_dlg[SPEED_BULLET].d2)
			speed_bullet = config_dlg[SPEED_BULLET].d2;
		if (config_dlg[SPEED_MAPSCROLL].d2)
			speed_mapscroll = config_dlg[SPEED_MAPSCROLL].d2;
		if (config_dlg[MAPSCROLL].d2)
			mapscroll = config_dlg[MAPSCROLL].d2;

		uninstall_timers();
		install_timers(speed_unit, speed_bullet, speed_mapscroll);
	}
}
