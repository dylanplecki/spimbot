/* The SPIMbot software is derived from James Larus's SPIM.  Modifications 
    to SPIM are covered by the following license:

 Copyright (c) 2004, University of Illinois.  All rights reserved.

 Developed by:  Craig Zilles
                Department of Computer Science
                     University of Illinois at Urbana-Champaign
                     http://www-faculty.cs.uiuc.edu/~zilles/spimbot

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal with the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimers.

 Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimers in the
 documentation and/or other materials provided with the distribution.

 Neither the name of the University of Illinois nor the names of its
 contributors may be used to endorse or promote products derived from
 this Software without specific prior written permission.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT.  IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
 SOFTWARE. */

#ifndef ROBOT_H
#define ROBOT_H

//#include "spimbotview.h"

#include <math.h>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#define SPIMBOT_MAXINT_32 INT32_MAX

#include <boost/random/mersenne_twister.hpp>


#include "../CPU/spim.h"
#include "../CPU/string-stream.h"
#include "../CPU/inst.h"
#include "../CPU/reg.h"
#include "../CPU/mem.h"

const int max_contexts = 2;

/* SCENARIO defitions and global */
#include "fruit_catching.h"

/* BOT defitions and global */
struct bot_state_t {
    int context;
    char name[80];
    unsigned color;

    double x, y;
    double last_x, last_y;
    double velocity;
    double orientation;
    double last_orientation;
    int turn_value;
    bool bonk;
    bool done;

    scenario_bot_state_t scenario;

    int timer;

};

typedef struct {
    double curr_delta_x;
    double curr_delta_y;

    long energy_cnt;
    double energy_amount;

    double field_active;
    double stationary_strength;

    int inactivation_cnt;

    int strat_state;
    short sector_stay;
    short sector_station;

    short cooldown_cnt;
} vpt_info;

extern const int BOT_RADIUS;
extern const int BOT_POINTER_LENGTH;
extern const double VELOCITY_SCALING_FACTOR;

extern bot_state_t robots[2];

extern bool spimbot_debug;
extern bool spimbot_exit_when_done;
extern bool spimbot_grading;
extern bool spimbot_largemap;
extern bool spimbot_maponly;
extern bool spimbot_nomap;
extern bool spimbot_randommap;
extern int spimbot_randomseed;
extern bool spimbot_run;
extern bool spimbot_testing;
extern bool spimbot_tournament;
extern bool spimbot_fancy;
extern int repaint_cycles;

extern boost::random::mt19937 puzzle_rng;
extern boost::random::mt19937 seeded_rng;
int random_loc();


extern double drawing_scaling;

extern const int LABEL_SPACE;
extern const int BOT_TRAY_LENGTH;

void world_initialize();
void bot_initialize(int context, bool randomize);
void redraw_map_window(QPainter* painter);
int world_update();

void spimbot_map_click();

void handle_wall_collisions(bot_state_t *bot, double delta_x, double delta_y);
void vpt_info_init();
void vpt_motion_update(bot_state_t * bot);

void vpt_motion_sector(bot_state_t * bot);
void vpt_sector_stay(bot_state_t * bot);
void vpt_sector_planet(bot_state_t * bot);
void vpt_planet_stay(bot_state_t * bot);
void vpt_cooldown(bot_state_t * bot);
void vpt_motion_periphery(bot_state_t * bot);
void vpt_motion_tagential(bot_state_t * bot);

void draw_bot(QPainter * painter, bot_state_t * bot);
void draw_vpt(QPainter * painter, bot_state_t * bot);

const int WORLD_SIZE = 300;

#define SPIMBOT_IO_BOT ((mem_addr) 0xffff0000)
#define SPIMBOT_IO_TOP ((mem_addr) 0xffffffff)
#define SPIMBOT_VEL_ADDR ((mem_addr) 0xffff0010)
#define SPIMBOT_TURN_VALUE_ADDR ((mem_addr) 0xffff0014)
#define SPIMBOT_TURN_CONTROL_ADDR ((mem_addr) 0xffff0018)
#define SPIMBOT_CYCLE ((mem_addr) 0xffff001c)

#define SPIMBOT_RELATIVE_ANGLE_COMMAND 0
#define SPIMBOT_ABSOLUTE_ANGLE_COMMAND 1

#define SPIMBOT_X_ADDR ((mem_addr) 0xffff0020)
#define SPIMBOT_Y_ADDR ((mem_addr) 0xffff0024)
#define SPIMBOT_EXIT_COMMAND ((mem_addr) 0xffff0030)
#define SPIMBOT_EXIT_X_ADDR ((mem_addr) 0xffff0038)
#define SPIMBOT_EXIT_Y_ADDR ((mem_addr) 0xffff003c)

#define SPIMBOT_BONK_ACK ((mem_addr) 0xffff0060)
#define SPIMBOT_TIMER_ACK ((mem_addr) 0xffff006c)

#define SPIMBOT_PRINT_INT ((mem_addr) 0xffff0080)
#define SPIMBOT_PRINT_FLOAT ((mem_addr) 0xffff0084)
#define SPIMBOT_PRINT_HEX ((mem_addr) 0xffff0088)

#define SPIMBOT_OTHER_X_ADDR ((mem_addr) 0xffff00a0)
#define SPIMBOT_OTHER_Y_ADDR ((mem_addr) 0xffff00a4)

#define SPIMBOT_REQUEST 1
#define SPIMBOT_ACKNOWLEDGE 2

void write_spimbot_IO(int context, mem_addr addr, mem_word value);
mem_word read_spimbot_IO(int context, mem_addr addr);

#define SB_BONK_INT_MASK 0x1000
#define SB_TIMER_INT_MASK 0x8000


#define SCALE(X) ((int) (drawing_scaling * (X)))

#ifndef __unused
#define __unused	__attribute__((unused))
#endif

#endif /* ROBOT_H */
