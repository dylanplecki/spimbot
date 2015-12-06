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

#include <QPainter>
#include <QTimer>
#include "qmath.h"

#ifdef _WIN32
#define _USE_MATH_DEFINES
#define round(fp) ((fp) >= 0 ? floor((fp) + 0.5) : ceil((fp) - 0.5))
#endif

#include <math.h>
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <assert.h>
#include <time.h>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>

#include "robot.h"
#include "../QtSpim/spimview.h"
#include "spimbotview.h"
#include "fruit_catching.h"


extern SpimView* Window;
extern SpimbotView* MapWindow;
extern int invisible_bot;

const int CYCLE_END = 10000000;
//TODO Make flag for extending game length
int spimbot_cycle_limit = CYCLE_END;
bool spimbot_debug = false;
bool spimbot_exit_when_done = false;
bool spimbot_grading = false;
bool spimbot_largemap = false;
bool spimbot_maponly = false;
bool spimbot_nomap = false;
bool spimbot_randommap = false;
int spimbot_randomseed = -1;
bool spimbot_run = false;
bool spimbot_testing = false;
bool spimbot_tournament = false;
bool spimbot_fancy = false;
int repaint_cycles = 0x2000;

// for scaling the map window
double drawing_scaling = 1.0;

// to allow user to interrupt (FIXME: does this still do anything?)
bool map_click = false;
void spimbot_map_click() { map_click = true; }

// spimBOT world constants
const int LABEL_SPACE = 50;
const int BOT_RADIUS = 5;
const int BOT_POINTER_LENGTH = 10;
const int BOT_TRAY_LENGTH = 8;
const double PI_DELTA = .001;
const double VELOCITY_SCALING_FACTOR = .0002;  //pixels per cycle
const bool WALLS = true;
const int MAX_VELOCITY = 10;

// contest related variables
bool reverse_image = false;
#define FLIP(context, X) ((double) (context==1 && reverse_image) ? (WORLD_SIZE - X) : X)
#define ROTATE(context, X) ((int) (context==1 && reverse_image) ? (X+M_PI) : X)
int cycle = 0;
bot_state_t robots[2];

// random number generators
boost::random::mt19937 puzzle_rng;
boost::random::mt19937 seeded_rng;

// test whether something happened recently
#define WINDOW 100
int withinWINDOW(int cycle1) { return ((cycle1 <= cycle) && ((cycle1 + WINDOW) >= cycle)); }

// get a random location
int random_loc() {
    static boost::random::uniform_int_distribution<> dist(BOT_RADIUS+5, WORLD_SIZE - BOT_RADIUS-5);
    return dist(seeded_rng);
}



void bot_initialize(int context, bool __unused randomize) {
    bot_state_t *bot = &(robots[context]);
    bot->context = context;
    bot->color = (context ? 0xff : 0xff0000);
    bot->done = false;
    bot->bonk = false;

    /*
    bot->x = FLIP(context, (randomize ? random_loc()/2 : WORLD_SIZE / 6)) ;
    bot->y = randomize ? random_loc() : WORLD_SIZE/2 ;
    boost::random::uniform_real_distribution<> dist(0, 2 * M_PI);
    bot->orientation = randomize ? dist(seeded_rng) : ROTATE(context, 0);
    */

    bot->x = context == 0 ? random_loc() : robots[0].x;
    bot->y = context == 0 ? random_loc() : robots[0].y;
    bot->orientation = 0;

    bot->velocity = 5;
    bot->last_x = 0;
    bot->last_y = 0;
    bot->turn_value = 0;
    bot->timer = SPIMBOT_MAXINT_32;
    scenario_bot_initialize(bot);
}

void handle_wall_collisions(bot_state_t *bot, double delta_x, double delta_y) {
    if (WALLS) {
        if ((bot->x < BOT_RADIUS) || (bot->x > (WORLD_SIZE - BOT_RADIUS))) {
            bot->x -= 2 * delta_x;
            bot->bonk = true;
            bot->velocity = 0.0;
        }
        if ((bot->y < BOT_RADIUS) || (bot->y > (WORLD_SIZE - BOT_RADIUS))) {
            bot->y -= 2 * delta_y;
            bot->bonk = true;
            bot->velocity = 0.0;
        }
    } else {
        if (bot->x < BOT_RADIUS) {
            bot->x += WORLD_SIZE - 2*BOT_RADIUS;
        } else if (bot->x > (WORLD_SIZE - BOT_RADIUS)) {
            bot->x -= WORLD_SIZE - 2*BOT_RADIUS;
        }
        if (bot->y < BOT_RADIUS) {
            bot->y += WORLD_SIZE - 2*BOT_RADIUS;
        } else if (bot->y > (WORLD_SIZE - BOT_RADIUS)) {
            bot->y -= WORLD_SIZE - 2*BOT_RADIUS;
        }
    }
}

void bot_motion_update(bot_state_t *bot) {
    if (bot->done) {
        return;
    }

    double velocity = VELOCITY_SCALING_FACTOR * bot->velocity;

    double delta_x = cos(bot->orientation) * velocity;
    double delta_y = sin(bot->orientation) * velocity;

    bot->x += delta_x;
    bot->y += delta_y;

    // do scenario-specific stuff
    bot_motion_update_scenario(bot);

    handle_wall_collisions(bot, delta_x, delta_y);

    if (bot->orientation > (2*M_PI + PI_DELTA)) {
        bot->orientation -= 2*M_PI;
    } else if (bot->orientation < (0 - PI_DELTA)) {
        bot->orientation += 2*M_PI;
    }
    assert ((bot->orientation <= (2*M_PI + PI_DELTA)) &&
            (bot->orientation >= (0 - PI_DELTA)));
}

void bot_io_update(bot_state_t *bot) {
    reg_image_t &reg_image = reg_images[bot->context];
    int cause = 0;
    if (bot->bonk
            && INTERRUPTS_ON(reg_image)
            && (CP0_Status(reg_image) & SB_BONK_INT_MASK)
            && ((CP0_Cause(reg_image) & SB_BONK_INT_MASK) == 0)) {
        bot->bonk = false;
        cause |= SB_BONK_INT_MASK;
        if (!bot->done && spimbot_debug) {
            if (!spimbot_tournament) {
                printf("bot %d: bonk interrupt\n", bot->context);
                fflush(stdout);
            }
        }
    }

    if ((bot->timer < cycle)
            && INTERRUPTS_ON(reg_image)
            && (CP0_Status(reg_image) & SB_TIMER_INT_MASK)) {
        bot->timer = SPIMBOT_MAXINT_32;
        cause |= SB_TIMER_INT_MASK;
        if (!bot->done && spimbot_debug) {
            if (!spimbot_tournament) {
                printf("bot %d: timer interrupt\n", bot->context);
                fflush(stdout);
            }
        }
    }

    // do scenario-specific stuff
    cause |= bot_io_update_scenario(bot);

    if (cause != 0) {
        if (spimbot_grading) {
            printf("bot %d: raising an interrupt/exception; cause = %x\n", bot->context, cause);
            fflush(stdout);
        }
        RAISE_EXCEPTION (reg_image, ExcCode_Int, CP0_Cause(reg_image) |= cause);
    }
}

void bot_update(bot_state_t *bot, int bot_index) {
    if (bot_index < num_contexts)
    {
        bot_motion_update(bot);
        bot_io_update(bot);
    }
}

int world_update() {
    for (int i = 0 ; i < num_contexts ; ++ i) {
        // alternate which bot goes first for fairness
        int index = i ^ (cycle & (num_contexts-1));
        bot_update(&robots[index], index);
    }

    scenario_world_update();

    ++ cycle;
    if (!spimbot_nomap && cycle % repaint_cycles == 0) {
        MapWindow->centralWidget()->repaint();
    }

    if (map_click) {
        if (spimbot_tournament)
            map_click = false;
        else
            return 1;
    }

    if ((robots[0].done && ((num_contexts == 1) || robots[1].done)) ||
            (spimbot_cycle_limit && (cycle >= spimbot_cycle_limit))) {

        if (spimbot_testing) {
            // exit 0 normally, 2 for timeout
            exit(2 * (cycle >= spimbot_cycle_limit));
        }

        MapWindow->centralWidget()->repaint();
        int winner = scenario_get_winner();
        printf("\ncycles: %d\n", cycle);
        std::pair<int, int> scores = scenario_get_scores();
        printf("scores: %d %d\n", scores.first, scores.second);
        printf("========================================\n");
        if (num_contexts != 1) {
            if (winner == -1) {
                winner = seeded_rng() & 1;
                printf("no winner (random winner %s)\n", robots[winner].name);
            } else {
                printf("winner: %s\n", robots[winner].name);
            }
        }
        mem_dump_profile();
        if (spimbot_exit_when_done) {
            spim_return_value = winner + 2;
            QTimer::singleShot(2000, Window, SLOT(sim_Exit()));
        }
        fflush(stdout);
        return 0;
    }

    return -1;
}

void draw_bot(QPainter* painter, bot_state_t *bot) {
    if (bot->done) {
        return;
    }
    int delta_x = (int) (cos(bot->orientation) * BOT_POINTER_LENGTH);
    int delta_y = (int) (sin(bot->orientation) * BOT_POINTER_LENGTH);


    QColor color = QColor(bot->color);
    QPen pen(color);
    QBrush brush(Qt::SolidPattern);
    brush.setColor(color);

    painter->setPen(pen);
    painter->setBrush(brush);

    painter->drawPie(SCALE(bot->x - BOT_RADIUS), SCALE(bot->y - BOT_RADIUS), SCALE(2*BOT_RADIUS), SCALE(2*BOT_RADIUS), 0, 360*64);

    pen.setWidth(1);

    painter->drawLine( SCALE(bot->x), SCALE(bot->y), SCALE(bot->x + delta_x), SCALE(bot->y + delta_y));
    painter->drawLine(SCALE(bot->x - 0.5 * BOT_TRAY_LENGTH),
        SCALE(bot->y - 2 * BOT_RADIUS), SCALE(bot->x + 0.5 * BOT_TRAY_LENGTH),
        SCALE(bot->y - 2 * BOT_RADIUS));

    bot->last_x = (int)(bot->x);
    bot->last_y = (int)(bot->y);
    bot->last_orientation = bot->orientation;
}

void redraw_map_window(QPainter* painter) {
    int draw_text = 1; // TODO was in capture_t_f.c

    // do scenario specific stuff
    redraw_scenario(painter);

    QPen pen(Qt::black);
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawLine(SCALE(0), SCALE(WORLD_SIZE) - 1, SCALE(WORLD_SIZE) - 1, SCALE(WORLD_SIZE) - 1);

    // re-draw bots and associated text
    for (int i = 0 ; i < num_contexts ; ++ i) {
        if (!(invisible_bot == i))
        {
            draw_bot(painter, &robots[i]);
        }
        if (draw_text) {
            QFont font = QFont();
            font.setPointSize(SCALE(8));
            painter->setFont(font);

            char str[200];
            scenario_set_bot_string(str, i);
            QString st = QString(str);
            // bot names no longer needed, and scores are replaced
            painter->drawText(SCALE(5), SCALE(WORLD_SIZE + 12 + (i*20)), st);

            painter->drawText(SCALE(64), SCALE(WORLD_SIZE + 12 + (i*20)), robots[i].name);
        }
    }

    write_output (message_out, "");
}

void write_spimbot_IO(int context, mem_addr addr, mem_word value) {
    bool success = scenario_write_spimbot_IO(context, addr, value);

    if (success) {
        return;
    }

    bot_state_t *bot = &robots[context];
    switch (addr) {
    case SPIMBOT_BONK_ACK:
        if (spimbot_grading) {
            printf("bot %d: bonk interrupt acknowledged\n", bot->context);
            fflush(stdout);
        }
        CP0_Cause(reg_images[bot->context]) &= ~SB_BONK_INT_MASK;
        break;
    case SPIMBOT_TIMER_ACK:
        if (spimbot_grading) {
            printf("bot %d: timer interrupt acknowledged\n", bot->context);
            fflush(stdout);
        }
        CP0_Cause(reg_images[bot->context]) &= ~SB_TIMER_INT_MASK;
        break;
    case SPIMBOT_CYCLE:
        bot->timer = value;
        if (spimbot_grading) {
            printf("bot %d: timer set after %d cycles\n", bot->context, value - cycle);
            fflush(stdout);
        }
        break;
    case SPIMBOT_VEL_ADDR:
        bot->velocity = MIN(MAX_VELOCITY, MAX(-MAX_VELOCITY, (int)value));
        break;
    case SPIMBOT_TURN_VALUE_ADDR:
        if ((value < -360) || (value > 360)) {
            if (spimbot_debug) {
                //printf("turn value %d is out of range\n", value);
                //fflush(stdout);
            }
        } else {
            bot->turn_value = value;
        }
        break;
    case SPIMBOT_TURN_CONTROL_ADDR:
        switch (value) {
        case SPIMBOT_RELATIVE_ANGLE_COMMAND:
            bot->orientation += bot->turn_value * (M_PI/180.0);
            if (spimbot_debug) {
                //printf("relative orientation: %f %f\n", (int)bot->turn_value, bot->orientation);
                //fflush(stdout);
            }
            break;
        case SPIMBOT_ABSOLUTE_ANGLE_COMMAND: {
            int turn_amount = bot->turn_value;
            if ((bot->context == 1) && reverse_image) {
                turn_amount = turn_amount + 180;
                if (turn_amount > 360) {
                    turn_amount -= 360;
                }
            }
            bot->orientation = turn_amount * (M_PI/180.0);
            if (spimbot_debug) {
                //printf("absolute orientation: %f %f\n", (int)bot->turn_value, bot->orientation);
                //fflush(stdout);
            }
            break;
        }
        default:
            if (spimbot_debug) {
                printf("unexpected angle command: %d\n", value);
                fflush(stdout);
            }
        }
        break;
    case SPIMBOT_PRINT_INT:
        if (!spimbot_tournament) {
            printf("%d\n", value);
            fflush(stdout);
        }
        break;
    case SPIMBOT_PRINT_HEX:
        if (!spimbot_tournament) {
            printf("%x\n", value);
            fflush(stdout);
        }
        break;
    case SPIMBOT_PRINT_FLOAT: {
        float *f = (float *)&value;
        if (!spimbot_tournament) {
            printf("%f\n", *f);
            fflush(stdout);
        }
        break;
    }
    default:
        run_error ("write to unused memory-mapped IO address (0x%x)\n", addr);
    }
}

mem_word read_spimbot_IO(int context, mem_addr addr) {
    bool success = 0;

    mem_word ret_val = scenario_read_spimbot_IO(context, addr, &success);
    if (success) {
        return ret_val;
    }

    bot_state_t *bot = &robots[context];
    switch (addr) {
    case SPIMBOT_X_ADDR: {
        mem_word x = (mem_word) FLIP(bot->context, bot->x);
        return x;
    }
    case SPIMBOT_Y_ADDR: {
        mem_word y = (mem_word) bot->y;
        return y;
    }
    case SPIMBOT_OTHER_X_ADDR: {
        mem_word x = (mem_word) FLIP(bot->context, robots[1 - bot->context].scenario.x);
        return x;
    }
    case SPIMBOT_OTHER_Y_ADDR: {
        mem_word y = (mem_word) robots[1 - bot->context].scenario.y;
        return y;
    }
    case SPIMBOT_VEL_ADDR:
        return mem_word(int(bot->velocity));
    case SPIMBOT_TURN_VALUE_ADDR:
        return mem_word(int(round(bot->orientation*180.0/M_PI)));
    case SPIMBOT_CYCLE:
        return cycle;
    default:
        run_error ("read from unused memory-mapped IO address (0x%x)\n", addr);
        return (0);
    }
}
