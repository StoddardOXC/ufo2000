/*
This file is part of "UFO 2000" aka "X-COM: Gladiators"
                    http://ufo2000.sourceforge.net/
Copyright (C) 2000-2001  Alexander Ivanov aka Sanami
Copyright (C) 2002       ufo2000 development team

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

#include "global.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <fstream>
#include "version.h"
#include "pck.h"
#include "explo.h"
#include "soldier.h"
#include "platoon.h"
#include "map.h"
#include "icon.h"
#include "inventory.h"
#include "sound.h"
#include "multiplay.h"
#include "wind.h"
#include "config.h"
#include "mainmenu.h"
#include "editor.h"
#include "about.h"
#include "video.h"
#include "keys.h"
#include "crc32.h"
#include "pfxopen.h"
#ifdef WIN32
//#include <allegro/platform/aintwin.h>
#include "../resource.h"
#endif

#include "sysworkarounds.h"


//#define DEBUG
#define MSCROLL 10
#define BACKCOLOR 0

Target target;
int HOST, DONE, TARGET, turn;
Mode MODE;
ConsoleWindow *g_console;

//! Limit of time for a single turn in seconds
int g_time_limit;
//! Current counter for time left for this turn
volatile int g_time_left;

Net *net;
Map *map;
TerrainSet *terrain_set;
Icon *icon;
Inventory *inventory;
About *about;
Editor *editor;
Platoon *p1, *p2;
Platoon *platoon_local, *platoon_remote;
Soldier *sel_man = NULL;
Explosive *elist;

volatile int CHANGE=1;
volatile int MOVEIT = 0;
volatile int FLYIT = 0;
volatile int NOTICE = 1;
volatile int MAPSCROLL = 1;
int NOTICEremote = 0;
int NOTICEdemon = 0;

void mouser_proc(int flags)
{
	CHANGE = 1;
}
END_OF_FUNCTION(mouser_proc);

void timer_handler()
{
	MOVEIT++;
}
END_OF_FUNCTION(timer_handler);

void timer_handler2()
{
	FLYIT++;
}
END_OF_FUNCTION(timer_handler2);

void timer_1s()
{
	if (g_time_left > 0) g_time_left--;
	NOTICE++;
}
END_OF_FUNCTION(timer_1s);

void timer_handler4()
{
	MAPSCROLL++;
}
END_OF_FUNCTION(timer_handler4);

int speed_unit = 15;
int speed_bullet = 30;
int speed_mapscroll = 30;
int mapscroll = 10;

void install_timers(int _speed_unit, int _speed_bullet, int _speed_mapscroll)
{
	install_int_ex(timer_handler, BPS_TO_TIMER(_speed_unit));     //ticks each second
	install_int_ex(timer_handler2, BPS_TO_TIMER(_speed_bullet * 2));     //ticks each second
	install_int_ex(timer_handler4, BPS_TO_TIMER(_speed_mapscroll));     //ticks each second
	install_int_ex(timer_1s, BPS_TO_TIMER(1));     //ticks each second
}

void uninstall_timers()
{
	remove_int(timer_handler4);
	remove_int(timer_1s);
	remove_int(timer_handler2);
	remove_int(timer_handler);
}

int keyboard_proc(int key)
{
	if (key >> 8 == KEY_F10) {
		change_screen_mode();
		return 0;
	}
	return key;
}
END_OF_FUNCTION(keyboard_proc);

GEODATA mapdata;
PLAYERDATA pd1, pd2;
PLAYERDATA *pd_local, *pd_remote;
int local_platoon_size;
extern int RECALC_VISIBILITY;

//simple all of new - rem about rand in sol's constructors
//can diff on remote comps
void restartgame()
{
	//map = new Map(4,4);
	//map->load("floor0.map", Map::cultivat);
	//map = new Map("GEODATA.DAT");
	map = new Map(mapdata);
	p1 = new Platoon(1111, &pd1);
	p2 = new Platoon(2222, &pd2);

	int fh = OPEN_GTEMP("cur_map.dat", O_CREAT | O_TRUNC | O_RDWR | O_BINARY);
	assert(fh != -1);
	write(fh, &mapdata, sizeof(mapdata));
	close(fh);

	fh = OPEN_GTEMP("cur_p1.dat", O_CREAT | O_TRUNC | O_RDWR | O_BINARY);
	assert(fh != -1);
	write(fh, &pd1, sizeof(pd1));
	close(fh);

	fh = OPEN_GTEMP("cur_p2.dat", O_CREAT | O_TRUNC | O_RDWR | O_BINARY);
	assert(fh != -1);
	write(fh, &pd2, sizeof(pd2));
	close(fh);

	//srand(0); //workaround!!!
	//p1 = new Platoon(1111, P1S, P1Z, P1X, P1Y, 0);
	//p2 = new Platoon(2222, P2S, P2Z, P2X, P2Y, 4);
	//srand(time(NULL));

	/*map->place(0, P1X, P1Y+2)->put(new Item(HIGH_EXPLOSIVE));
	map->place(0, P2X, P2Y+2)->put(new Item(HIGH_EXPLOSIVE));

	map->place(0, P1X, P1Y+2)->put(new Item(LASER_PISTOL));
	map->place(0, P2X, P2Y+2)->put(new Item(LASER_PISTOL));

	map->place(0, P1X, P1Y+2)->put(new Item(LASER_GUN));
	map->place(0, P2X, P2Y+2)->put(new Item(LASER_GUN));*/

	elist = new Explosive();
	elist->reset();
	if (HOST) {
		platoon_local = p1;
		platoon_remote = p2;
		MODE = MAP3D;
	} else {
		platoon_local = p2;
		platoon_remote = p1;
		MODE = WATCH;
	}

	//sel_man = NULL;
	sel_man = platoon_local->captain();
	map->center(sel_man);
	DONE = 0; TARGET = 0; turn = 0;
	RECALC_VISIBILITY = 1;
}

int initgame()
{
	if (!net->init())
		return 0;

	install_timers(speed_unit, speed_bullet, speed_mapscroll);
	//mouse_callback = mouser_proc;

	reset_video();
	restartgame();
	resize_screen2(0, 0);
	//clear_to_color(screen, 58); //!!!!!

	return 1;
}

void closegame()
{
	delete p1;
	p1 = NULL;
	delete p2;
	p2 = NULL;
	delete map;
	map = NULL;
	delete elist;
	elist = NULL;

	uninstall_timers();
	net->close();
}

int print_y = 0;
Wind *print_win = NULL;

void print(const char *str)
{
	//#ifdef WIN32
	if (print_win != NULL) {
		print_win->printstr(str);
		print_win->printstr("\r\n");
	} else {
		text_mode( -1);
		textout(screen, font, str, 0, print_y, xcom1_color(255));
		print_y += 10;
	}
	//#else
	//	cprintf("%s\r\n", str);
	//#endif
}

class consoleBuf : public std::streambuf {
    std::string curline;
    bool doCout;

protected:
    virtual int overflow(int c) {
        if (doCout)
            if (c == 10)
                std::cout<<std::endl;
            else
                std::cout<<(static_cast<char>(c));
        if (c == 10) {
            if (print_win != NULL) {
                curline.append("\r\n");
                print_win->printstr(curline.c_str());
            } else {
                curline.append("\n");
                text_mode( -1);
                textout(screen, font, curline.c_str(), 0, print_y, xcom1_color(255));
                print_y += 10;
            }
            curline.assign("");
        } else {
            curline += static_cast<char>(c);
        }

        return c;
    }
public:
    consoleBuf(bool dco) {
        curline.assign("");
        doCout = dco;
    }
};


#define FADE_SPEED 20


/** Function to check if all necessary files exist and are OK
*/
void check_data_files()
{
	// Checking data files integrity
	std::vector<std::string> bad_files;
	if (get_corrupted_or_missing_files(bad_files)) {
		std::string error_text;
		std::vector<std::string>::size_type i;
		error_text += "The following data files are bad or missing:\n";
		for (i = 0; i < bad_files.size() && i <= 8; i++) {
			error_text += bad_files[i];
			error_text += '\n';
		}
		if (i < bad_files.size()) error_text += "...\n";
		error_text += '\n';
		error_text += "Please check that you have copied directories with data files from ";
		error_text += "'UFO: Enemy Unknown' by Microprose to ufo2000 directory ";
		error_text += "(for more instructions look at 'install' file)\n";

#ifdef WIN32
		MessageBox(NULL, error_text.c_str(), "UFO2000 Error!", MB_OK);
#else
		fprintf(stderr, "%s", error_text.c_str());
#endif
		exit(1);
	}
}

void initmain(int argc, char *argv[])
{

	srand(time(NULL));
	set_uformat(U_UTF8);
	allegro_init();
	register_bitmap_file_type("jpg", load_jpg, NULL);
	set_color_conversion(COLORCONV_REDUCE_TO_256);

    FLAGS = 0;
	push_config_state();
	set_config_file("ufo2000.ini");
	if (get_config_int("Flags", "F_CLEARSEEN", 0)) FLAGS |= F_CLEARSEEN;      // clear seen every time
	if (get_config_int("Flags", "F_SHOWROUTE", 0)) FLAGS |= F_SHOWROUTE;      // show pathfinder matrix
	if (get_config_int("Flags", "F_SHOWLOFCELL", 0)) FLAGS |= F_SHOWLOFCELL;  // show cell's LOF & BOF
	if (get_config_int("Flags", "F_SHOWLEVELS", 0)) FLAGS |= F_SHOWLEVELS;    // show all level
	if (get_config_int("Flags", "F_FASTSTART", 0)) FLAGS |= F_FASTSTART;      // skip
	if (get_config_int("Flags", "F_FULLSCREEN", 0)) FLAGS |= F_FULLSCREEN;    // start fullscreen mode
	if (get_config_int("Flags", "F_RAWMESSAGES", 0)) FLAGS |= F_RAWMESSAGES;  // show raw net packets
	if (get_config_int("Flags", "F_SEL_ANY_MAN", 0)) FLAGS |= F_SEL_ANY_MAN;  // allow select any man
	if (get_config_int("Flags", "F_SWITCHVIDEO", 1)) FLAGS |= F_SWITCHVIDEO;  // allow switch full/window screen mode
	if (get_config_int("Flags", "F_PLANNERDBG", 0)) FLAGS |= F_PLANNERDBG;    // mission planner debug mode
	if (get_config_int("Flags", "F_ENDLESS_TU", 0)) FLAGS |= F_ENDLESS_TU;    // endless soldier's time units
	if (get_config_int("Flags", "F_SAFEVIDEO", 1)) FLAGS |= F_SAFEVIDEO;      // enable if you experience bugs with video
	if (get_config_int("Flags", "F_SELECTENEMY", 1)) FLAGS |= F_SELECTENEMY;  // draw blue arrows and numbers above seen enemies
	if (get_config_int("Flags", "F_FILECHECK", 1)) FLAGS |= F_FILECHECK;      // check for datafiles integrity
	if (get_config_int("Flags", "F_LARGEFONT", 0)) FLAGS |= F_LARGEFONT;      // use big ufo font for dialogs, console and stuff.
    if (get_config_int("Flags", "F_SMALLFONT", 0)) FLAGS |= F_SMALLFONT;      // no, use small font instead.
    if (get_config_int("Flags", "F_SOUNDCHECK", 0)) FLAGS |= F_SOUNDCHECK;    // perform soundtest.
    if (get_config_int("Flags", "F_LOGTOSTDOUT", 0)) FLAGS |= F_LOGTOSTDOUT;  // Copy all init console output to stdout.
	origfiles_prefix = get_config_string("Paths",  "origfiles", NULL); // original ufo files here
	ownfiles_prefix  = get_config_string("Paths",  "ownfiles",  NULL); // own data files here (ufo2000.dat & bitmaps)
	gametemp_prefix  = get_config_string("Paths",  "gametemp",  NULL); // game temporary files here (may span launches)
	runtemp_prefix	 = get_config_string("Paths",  "runtemp",   NULL); // runtime temporary files here (get deleted by the time of exit)

	if (origfiles_prefix != NULL) {
		origfiles_prefix = ustrdup(origfiles_prefix);
	}
	if (ownfiles_prefix != NULL)  {
		ownfiles_prefix = ustrdup(ownfiles_prefix);
	}
	if (gametemp_prefix != NULL) {
		gametemp_prefix = ustrdup(gametemp_prefix);
	}
	if (runtemp_prefix != NULL) {
		runtemp_prefix = ustrdup(runtemp_prefix);
	}

	pop_config_state();

	if (FLAGS & F_FILECHECK) check_data_files();

	datafile = load_datafile("#");
	if (datafile == NULL) {
		datafile = LOADDATA_OWN("ufo2000.dat");
		if (datafile == NULL) {
			allegro_exit();
			fprintf(stderr, "Error loading datafile!\n\n");
			exit(1);
		}
	}

	memset(&mapdata, 0, sizeof(mapdata));
	if (argc > 1) {
		int fh = open(argv[1], O_RDONLY | O_BINARY);
		assert(fh != -1);
		read(fh, &mapdata, sizeof(mapdata));
		close(fh);
	} else {
		int fh = OPEN_GTEMP("geodata.dat", O_RDONLY | O_BINARY);
		assert(fh != -1);
		read(fh, &mapdata, sizeof(mapdata));
		close(fh);
	}

	set_window_title("UFO 2000");
	set_window_close_button(0);
#ifdef WIN32
	HICON hi = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
	SetClassLong(win_get_window(), GCL_HICON, (LPARAM)hi);
#endif

	set_video_mode();
	set_palette(black_palette);

	PALETTE pal;
	BITMAP *text_back = load_memory_jpg(datafile[DAT_TEXT_BACK_JPG].dat, datafile[DAT_TEXT_BACK_JPG].size, pal);

	stretch_blit(text_back, screen, 0, 0, text_back->w, text_back->h, 0, 0, screen->w, screen->h);
    fade_from(black_palette, pal, (64 - FADE_SPEED)/3 + FADE_SPEED);

	print_win = new Wind(text_back, 15, 300, 625, 390, 255);

    /* to use the init console as an ostream -very handy. */
    consoleBuf consbuf(FLAGS & F_LOGTOSTDOUT);
    std::ostream console(&consbuf);

	console<<"allegro_init"<<std::endl;

	console<<"loadini"<<std::endl;
	loadini();
	console<<"install_timer"<<std::endl;
	install_timer();
	console<<"install_mouse"<<std::endl;
	install_mouse();
	console<<"install_keyboard"<<std::endl;
	install_keyboard();
    {
        console<<"Initializing sound..."<<std::endl;
        rest(1500);
        std::string xml;
        soundSystem *ss = soundSystem::getInstance();
        std::ifstream ifs_xml("soundmap.xml");
        if (ifs_xml) {
            ISTREAM_TO_STRING(ifs_xml, xml);

            if (FLAGS & F_SOUNDCHECK) {
                if (0 == ss->initialize(xml, &console, true)) {
                    console<<"  Soundcheck in progress..."<<std::endl;
                    ss->playLoadedSamples(&console);
                } else {
                    console<<"  soundSystem initialization failed."<<std::endl;
                }
            } else {
                if ( 0 > ss->initialize(xml, &console, false))
                    console<<"  Failed."<<std::endl;
            }
        } else {
            console<<"  Error reading soundmap.xml"<<std::endl;
        }
    }
	console<<"initvideo"<<std::endl;
	initvideo();

	console<<"initmainmenu"<<std::endl;
	initmainmenu();

	LOCK_VARIABLE(CHANGE); LOCK_FUNCTION(mouser_proc);
	LOCK_VARIABLE(MOVEIT); LOCK_FUNCTION(timer_handler);
	LOCK_VARIABLE(FLYIT); LOCK_FUNCTION(timer_handler2);
	LOCK_VARIABLE(NOTICE); LOCK_VARIABLE(g_time_left); LOCK_FUNCTION(timer_1s);
	LOCK_VARIABLE(MAPSCROLL); LOCK_FUNCTION(timer_handler4);

	LOCK_FUNCTION(keyboard_proc);

	console<<"initpck units"<<std::endl;
	Soldier::initpck();

	console<<"initpck terrain"<<std::endl;
	Map::initpck();

	console<<"init obdata"<<std::endl;
	Item::initobdata();

	console<<"init bigobs"<<std::endl;
	Item::initbigobs();

	console<<"new console window"<<std::endl;
	std::string consolefont = get_config_string("General", "consolefont", "default");
	FONT * fnt = font;
	if (consolefont == "xcom_small") {
		fnt = g_small_font;
	} else if (consolefont == "xcom_large") {
		fnt = large;
	}
	g_console = new ConsoleWindow(screen->w, screen->h - SCREEN2H, fnt);

	console<<"new icon"<<std::endl;
	icon = new Icon((SCREEN2W - 320) / 2, SCREEN2H - 56);

	console<<"new inventory"<<std::endl;
	inventory = new Inventory();

	console<<"new about"<<std::endl;
	about = new About();

	console<<"new editor"<<std::endl;
	editor = new Editor();

	console<<"new net"<<std::endl;
	net = new Net();

	console<<"new terrain_set"<<std::endl;
	terrain_set = new TerrainSet();

	mouse_callback = mouser_proc;
	//keyboard_callback = keyboard_proc;

	fade_out(FADE_SPEED);
	clear(screen);

	delete print_win;
}

void closemain()
{
	saveini();
	/*	for(int i=0;i<obdata_num;i++) {
			delete []obdata[i];
		}
		delete []obdata;*/
	//closenet();

	delete terrain_set;
	delete net;
	delete editor;
	delete about;
	delete inventory;
	delete icon;
	delete g_console;

	Item::freebigobs();
	Map::freepck();
	Soldier::freepck();

	soundSystem::getInstance()->shutdown();
	closevideo();

	allegro_exit();
	std::cout<<"\nUFO 2000 remake version "
             <<UFO_VERSION_STRING
             <<" revision "
             <<UFO_SVNVERSION
             <<"\nCopyright Sanami (C) "
             <<__TIME__<<" "
             <<__DATE__
             <<"\n\n"
             <<"Yakutsk nightware\n\n"
             <<"http://sourceforge.net/projects/ufo2000/\n"
             <<"http://ufo2k-allegro.lxnt.info/\n"
             <<"http://pages.ykt.ru/ufo2000/\n\n";

}

int build_crc()
{
	char buf[200000]; memset(buf, 0, sizeof(buf));
	int buf_size = 0;

	p1->eot_save(buf, buf_size);
	p2->eot_save(buf, buf_size);
	map->eot_save(buf, buf_size);

	int fh = OPEN_GTEMP("eot_save.txt", O_CREAT | O_TRUNC | O_RDWR | O_BINARY);
	assert(fh != -1);
	buf_size = write(fh, buf, buf_size);
	close(fh);

	return crc16(buf);
	//return 2000;
}


extern int RECALC_VISIBILITY;

void next_turn(int crc)
{
	CONFIRM_REQUESTED = 0;
	int bcrc;
	if (crc != -1) {
		//!!!compare for remote crc
		bcrc = build_crc();
		if (crc != bcrc) {
			g_console->print("wrong wholeCRC");
			g_console->printf("crc=%d, bcrc=%d", crc, bcrc);
		}
	}

	turn++;

	if (MODE == WATCH) {
		g_time_left = g_time_limit;
		MODE = MAP3D;
	} else {
		g_time_left = 0;
		MODE = WATCH;
	}

	if (net->gametype == HOTSEAT) {
		icon->show_eot();
		map->m_minimap_area->set_full_redraw();
		g_console->set_full_redraw();

		Platoon *pt = platoon_local;
		platoon_local = platoon_remote;
		platoon_remote = pt;

		sel_man = platoon_local->captain();
		if (sel_man != NULL)
			map->center(sel_man);
//		map->clearseen();
		MODE = MAP3D;
		g_time_left = g_time_limit;
		RECALC_VISIBILITY = 1;      // !!!!!!!!!!!!!!!!!
	}

	if (crc == -1) {
		//!!!build map&man crc
		bcrc = build_crc();
		net->send_endturn(bcrc);
	}

	if (turn % 2 == 0) {
		map->step();      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		platoon_local->restore();
		platoon_remote->restore();
	}

	elist->step(crc);
	g_console->printf(
		xcom1_color(192),
		"Next turn. local = %d, remote = %d soldiers",
		platoon_local->num_of_men(),
		platoon_remote->num_of_men());
}

int GAMELOOP = 0;

/** Redraw field of view and minimap on the screen
*/
void build_screen(int & select_y)
{
	clear_to_color(screen2, BACKCOLOR);

	switch (MODE) {
		case UNIT_INFO:
			if (sel_man != NULL)
				sel_man->draw_unibord(SCREEN2W / 2 - 160, SCREEN2H / 2 - 100);
			else
				MODE = MAP3D;
			break;
		case MAP2D:
			map->draw2d();
			break;
		case WATCH:
		case MAP3D:
			map->set_sel(mouse_x, mouse_y);
			map->draw();

			if (key[KEY_LCONTROL] && sel_man != NULL && !sel_man->ismoving()) {
				if (TARGET)
					sel_man->draw_bullet_way();
				else
					map->draw_path_from(sel_man);
			}

			p1->bulldraw();
			p2->bulldraw();

			platoon_remote->draw_blue_selectors();

			if (sel_man != NULL) {
				sel_man->draw_selector(select_y);
				sel_man->draw_enemy_seen(select_y);
			}

			icon->draw();

			if (MODE == WATCH)
				textprintf(screen2, font, 0, 0, xcom1_color(1), "%s", "WATCH");
			break;
		case MAN:
			if (sel_man != NULL) {
				inventory->draw();
			} else {
				MODE = MAP3D;
				//map->cell[map->sel_col][map->sel_row].place.put(icon->sel_item);
				//icon->sel_item = NULL;
			}
			break;
	}

	draw_sprite(screen2, mouser, mouse_x, mouse_y);
	blit(screen2, screen, 0, 0, 0, 0, screen2->w, screen2->h);
	map->svga2d();

	if (FLAGS & F_SHOWLOFCELL) {
		map->show_lof_cell();
	}
}

/**
 * Save game state to "ufo2000.sav" file
 */
void savegame()
{
	std::fstream f("ufo2000.sav", std::ios::binary | std::ios::out);
	char sign[64];
	sprintf(sign, "ufo2000 %s (%s %s)\n", UFO_VERSION_STRING, __DATE__, __TIME__);
	f.write(sign, strlen(sign) + 1);

	persist::Engine archive(f, persist::Engine::modeWrite);

	PersistWriteBinary(archive, &turn, sizeof(turn));
	PersistWriteBinary(archive, &MODE, sizeof(MODE));
	PersistWriteBinary(archive, &pd1, sizeof(pd1));
	PersistWriteBinary(archive, &pd2, sizeof(pd2));
	PersistWriteBinary(archive, &mapdata, sizeof(mapdata));

	PersistWriteObject(archive, p1);
	PersistWriteObject(archive, p2);
	PersistWriteObject(archive, platoon_local);
	PersistWriteObject(archive, platoon_remote);
	PersistWriteObject(archive, map);
	PersistWriteObject(archive, sel_man);
	PersistWriteObject(archive, elist);
}

/**
 * Load game state from a savefile
 *
 * @param filename name of the file in which the game state is stored
 * @return         true on success, false on failure
 */
bool loadgame(const char *filename)
{
	std::fstream f(filename, std::ios::binary | std::ios::in);
	if (!f.is_open()) return false;

	char sign[64], buff[64];
	sprintf(sign, "ufo2000 %s (%s %s)\n", UFO_VERSION_STRING, __DATE__, __TIME__);
	f.read(buff, strlen(sign) + 1);
	if (strcmp(sign, buff) != 0) return false;

	persist::Engine archive(f, persist::Engine::modeRead);

	PersistReadBinary(archive, &turn, sizeof(turn));
	PersistReadBinary(archive, &MODE, sizeof(MODE));
	PersistReadBinary(archive, &pd1, sizeof(pd1));
	PersistReadBinary(archive, &pd2, sizeof(pd2));
	PersistReadBinary(archive, &mapdata, sizeof(mapdata));

	delete p1;
	delete p2;
	delete map;
	delete elist;

	PersistReadObject(archive, p1);
	PersistReadObject(archive, p2);
	PersistReadObject(archive, platoon_local);
	PersistReadObject(archive, platoon_remote);
	PersistReadObject(archive, map);
	PersistReadObject(archive, sel_man);
	PersistReadObject(archive, elist);

	TARGET = 0;

	return true;
}

/**
 * Main loop of the tactical part of the game
 */
void gameloop()
{
	int mouse_leftr = 1, mouse_rightr = 1, select_y = 0;

//	play_midi(g_midi_music, 1);

	clear_keybuf();
	GAMELOOP = 1;
	RECALC_VISIBILITY = 1;

	if (MODE != WATCH) {
		g_time_left = g_time_limit;
	} else {
		g_time_left = 0;
	}

	while (!DONE) {

		if (MODE != WATCH && g_time_left == 0) {
			TARGET = 0;
			if (platoon_local->nomoves()) {
				next_turn(-1);
			}
		}

		g_console->redraw(screen, 0, SCREEN2H);

		net->check();

		if (NOTICE) {
			if (NOTICEdemon) {
				net->send_notice();
//				g_console->printf("%d", NOTICEremote);
			}
			NOTICEremote++;
			NOTICE = 0;
		}

		if (CHANGE) {
			FLYIT = 0;
			MOVEIT = 0;
		}

		while (FLYIT > 0) {
			platoon_local->bullmove();     //!!!! bull of dead?
			platoon_remote->bullmove();

			FLYIT--;
		}

		while (MOVEIT > 0) {
			//map->clearvis();
			if (FLAGS & F_CLEARSEEN)
				map->clearseen();

			map->smoker();
			platoon_remote->move(0);
			platoon_local->move(1);      //!!sel_man may die

			select_y = (select_y + 1) % 3;
			CHANGE = 1;

			MOVEIT--;
		}

		if (CHANGE) {
			build_screen(select_y);
			CHANGE = 0;
		}

		if (MAPSCROLL) {
			if ((MODE == MAP3D) || (MODE == WATCH)) {
				if (map->scroll(mouse_x, mouse_y)) CHANGE = 1;
			}
			MAPSCROLL = 0;
		}

		if ((mouse_b & 1) && (mouse_leftr)) { //left
			mouse_leftr = 0;
			switch (MODE) {
				case UNIT_INFO:
					MODE = MAP3D;
					break;
				case WATCH:
					break;
				case MAP2D:
					if (map->center2d(mouse_x, mouse_y))
						MODE = MAP3D;
					break;
				case MAP3D:
					if (sel_man != NULL && sel_man->center_enemy_seen()) break;

					if (TARGET) {
						if (icon->inside(mouse_x, mouse_y)) {
							//TARGET=0;
							icon->execute(mouse_x, mouse_y);
							break;
						}

						if (sel_man != NULL) sel_man->try_shoot();
					} else
						if (!icon->inside(mouse_x, mouse_y)) {
							if (sel_man != NULL) {
								Soldier * ssman = map->sel_man();
								if (ssman == NULL) {
									if (!sel_man->ismoving()) {
										sel_man->wayto(map->sel_lev, map->sel_col, map->sel_row);
									}
								} else {
									if (!platoon_local->belong(ssman)) {
										if (!sel_man->ismoving()) {
											sel_man->wayto(map->sel_lev, map->sel_col, map->sel_row);
										}
									}

									if (!sel_man->ismoving()) {
										if (platoon_local->belong(ssman))
											sel_man = ssman;
									}
									if (FLAGS & F_SEL_ANY_MAN)
										sel_man = ssman;
								}
							} else {
								Soldier *ss = map->sel_man();
								if ((ss != NULL) && (platoon_local->belong(ss)))
									sel_man = ss;
								//net_send("_sel_man");
								if (FLAGS & F_SEL_ANY_MAN) {
									if (ss != NULL)
										sel_man = ss;
								}
							}
						} else {
							icon->execute(mouse_x, mouse_y);
						}
					break;
				case MAN:
					inventory->execute();
					break;
			}
		}

		if ((mouse_b & 2) && (mouse_rightr)) { //right
			mouse_rightr = 0;
			switch (MODE) {
				case UNIT_INFO:
					MODE = MAP3D;
					break;
				case WATCH:
					break;
				case MAP2D:
					map->center2d(mouse_x, mouse_y);
					MODE = MAP3D;
					break;
				case MAP3D:
					if (TARGET)
						TARGET = 0;
					else {
						if (sel_man != NULL) {
							if (!sel_man->ismoving()) {
								sel_man->faceto(map->sel_col, map->sel_row);

								//if (sel_man->curway == -1)
								//	sel_man->open_door();
								if (!sel_man->ismoving())
									sel_man->open_door();
							} else if (sel_man->is_marching()) {
								//sel_man->finish_march();
								sel_man->break_march();
								//sel_man->waylen = sel_man->curway - 1;
							}
						}
					}
					break;
				case MAN:
					inventory->close();
					break;
			}
		}

		if (!(mouse_b & 1)) {
			mouse_leftr = 1;
			//			CHANGE = 1;
		}

		if (!(mouse_b & 2)) {
			mouse_rightr = 1;
			//			CHANGE = 1;
		}

		process_keyswitch();

		if (keypressed()) {
			int scancode;
			int keycode = ureadkey(&scancode);

			switch (scancode) {
				case KEY_PGUP:
					if (map->sel_lev < map->level - 1) {
						map->sel_lev++;
						//mouse_y -= 24;
						position_mouse(mouse_x, mouse_y - 24);
					}
					break;
				case KEY_PGDN:
					if (map->sel_lev > 0) {
						map->sel_lev--;
						//mouse_y += 24;
						position_mouse(mouse_x, mouse_y + 24);
					}
					break;
				case KEY_PLUS_PAD:
					resize_screen2(10, 10);
					break;
				case KEY_MINUS_PAD:
					resize_screen2( -10, -10);
					break;
				case KEY_LEFT:
					resize_screen2( -10, 0);
					//map->move(MSCROLL*2,0);
					break;
				case KEY_UP:
					resize_screen2(0, -10);
					//map->move(0,MSCROLL*2);
					break;
				case KEY_RIGHT:
					resize_screen2(10, 0);
					//map->move(-MSCROLL*2,0);
					break;
				case KEY_DOWN:
					resize_screen2(0, 10);
					//map->move(0,-MSCROLL*2);
					break;
				case KEY_F1:
					if (FLAGS & F_RAWMESSAGES) {
						FLAGS &= ~F_RAWMESSAGES;
					} else {
						FLAGS |= F_RAWMESSAGES;
					}
					break;
				case KEY_F2:
					if (askmenu("SAVE GAME"))
						savegame();
					break;
				case KEY_F3:
					if (askmenu("LOAD GAME")) {
						if (!loadgame("ufo2000.sav")) {
							alert("saved game not found", "", "", "OK", NULL, 0, 0);
						}
					}
					break;
				case KEY_F5:
					if (askmenu("RESTART GAME")) {
						//restartgame();
						net->send_restart();
						//position_mouse(160, 100);
					}
					break;
				case KEY_F9:
					if (FLAGS & F_SEL_ANY_MAN) {
						if (MODE == WATCH)
							MODE = MAP3D;
						else
							MODE = WATCH;
					}
					break;
				case KEY_F10:
					change_screen_mode();
					break;
				case KEY_F11:
					if (NOTICEdemon) {
						if (askmenu("STOP NOTIFY"))
							NOTICEdemon = 0;
					} else {
						if (askmenu("START NOTIFY")) {
							NOTICEdemon = 1;
							NOTICEremote = 0;
						}
					}
					break;
				case KEY_F12:
					if (askmenu("SNAPSHOT")) {
						savescreen();
					}
					break;
				case KEY_ESC:
					if (askmenu("EXIT GAME"))
						DONE = 1;
					break;
				default:
					if (g_console->process_keyboard_input(keycode, scancode))
						net->send_message((char *)g_console->get_text());
			}
			CHANGE = 1;
		}
	}

	play_midi(NULL, 0);

	GAMELOOP = 0;
	net->send_quit();

	fade_out(10);
	//readkey();
	clear(screen);
}

void faststart()
{
	HOST = sethotseatplay();

	//initgame();
	install_timers(speed_unit, speed_bullet, speed_mapscroll);

	reset_video();

	//restartgame();

	int fh = OPEN_GTEMP("cur_map.dat", O_RDONLY | O_BINARY);
	assert(fh != -1);
	read(fh, &mapdata, sizeof(mapdata));
	close(fh);

	fh = OPEN_GTEMP("cur_p1.dat", O_RDONLY | O_BINARY);
	assert(fh != -1);
	read(fh, &pd1, sizeof(pd1));
	close(fh);

	fh = OPEN_GTEMP("cur_p2.dat", O_RDONLY | O_BINARY);
	assert(fh != -1);
	read(fh, &pd2, sizeof(pd2));
	close(fh);

	map = new Map(mapdata);
	elist = new Explosive();
	p1 = new Platoon(1111, &pd1);
	p2 = new Platoon(2222, &pd2);

	//map->place(0, 0, 0)->put(new Item(KASTET));
	//map->place(0, 0, 0)->put(new Item(KNIFE));

	elist->reset();
	if (HOST) {
		platoon_local = p1;
		platoon_remote = p2;
		MODE = MAP3D;
	} else {
		platoon_local = p2;
		platoon_remote = p1;
		MODE = WATCH;
	}

	sel_man = platoon_local->captain();
	map->center(sel_man);
	DONE = 0; TARGET = 0; turn = 0;

	resize_screen2(0, 0);
	clear_to_color(screen, 58);      //!!!!!
	gameloop();
	closegame();
	closemain();
}

void start_loadgame()
{
	HOST = sethotseatplay();

	install_timers(speed_unit, speed_bullet, speed_mapscroll);

	reset_video();

   	if (!loadgame("ufo2000.sav"))
   	{
   		alert("saved game not found", "", "", "OK", NULL, 0, 0);
   		return;
   	}

	map->center(sel_man);
	DONE = 0;

	resize_screen2(0, 0);
	clear_to_color(screen, 58);      //!!!!!
	gameloop();
	closegame();
}

int main(int argc, char *argv[])
{
#ifdef WIN32
	// to simplify debugging from Microsoft Visual Studio change
	// current directory to the place where the program executable is
	char szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, sizeof(szPath));
	if (strrchr(szPath, '\\')) *strrchr(szPath, '\\') = '\0';
	SetCurrentDirectory(szPath);
#endif

	initmain(argc, argv);

	if (FLAGS & F_FASTSTART) {
		faststart();
	} else {
        int mm = 2, h = -1;
        while ((mm = do_mainmenu()) != MAINMENU_QUIT) {
            h = -1;
            switch (mm) {
                case MAINMENU_ABOUT:
                    about->show();
                    continue;
                case MAINMENU_EDITOR:
                    editor->do_mapedit();
                    continue;
                case MAINMENU_HOTSEAT:
                    h = sethotseatplay();
                    break;
                case MAINMENU_TCPIP:
                    h = setsocketplay();
                    break;
#ifdef HAVE_DPLAY
                case MAINMENU_DPLAY:
                    h = setdplayplay();
                    break;
#endif
                case MAINMENU_LOADGAME:
                    start_loadgame();
                    break;
                default:
                    continue;
            }
            if (h == -1)
                continue;
            HOST = h;

            if (initgame()) {
                gameloop();
                closegame();
            }
        }
    }

	closemain();
	return 0;
}
END_OF_MAIN();
