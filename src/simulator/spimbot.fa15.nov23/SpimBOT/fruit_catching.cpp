/* The SPIMbot software is derived from James Larus's SPIM.  Modifications 
	to SPIM are covered by the following license:

 Copyright © 2012, University of Illinois.  All rights reserved.

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

#define __STDC_LIMIT_MACROS

#include <QPainter>
#include <algorithm>
#include <boost/random/uniform_int_distribution.hpp>
#include <climits>
#include <map>
#include <math.h>
#include <iostream>

#include "fruit_catching.h"
#include "lemon.h"
#include "mango.h"
#include "guava.h"
#include "loquat.h"
#include "cherry.h"
#include "robot.h"
#include "spimbotview.h"
#include "puzzlegenerator.h"

extern double drawing_scaling;
extern const double VELOCITY_SCALING_FACTOR;
extern SpimbotView* MapWindow;

int invisible_bot = -1;
int draw_text = 1;

#ifndef min
#define min(x,y) (((x)<(y)) ? (x) : (y))
#endif
#ifndef max
#define max(x,y) (((x)>(y)) ? (x) : (y))
#endif


const int MAX_NUM_FRUITS = 16;
//how often fruits can get added
const int FRUIT_CLOCK = 16384;
//how often enemy position gets updated
const int ENEMY_BOT_CLOCK = 5000;

const double INIT_BOT_ENERGY = 100;
const double MAX_BOT_ENERGY = 1000;
// multiply this factor by the number of fruits the bot carry
const double ENERGY_DRAIN_FACTOR = 0.00001;
// multiple this factor by the number of puzzle the bot solve
const double ENERGY_AWARD_FACTOR = 20;
const double SMASH_ENERGY_COST = 5;

const int PUZZLE_MIN_ROW_COL = 16;
const int PUZZLE_MAX_ROW_COL = 64;

const int PUZZLE_WORD_MAX_LENGTH = PUZZLE_MAX_ROW_COL + PUZZLE_MAX_ROW_COL / 4;

const int PUZZLE_DELAY_CYCLES = 1000;


std::map<int, Fruit *> fruits;


void init_bots_energy(bot_state_t *bot, long init_energy);


/**
 * @brief scenario_bot_initialize
 * @param bot
 *
 * This initializes the scenario data for each bot
 */
void 
scenario_bot_initialize(bot_state_t *bot) {
  bot->scenario.points = 0;
  bot->scenario.scan_address = 0;

  bot->scenario.total_smoosh_fruits = 0;
  bot->scenario.total_smoosh_interrupts_sent = 0;

  init_bots_energy(bot, INIT_BOT_ENERGY);

  bot->scenario.puzzle_request_cycle = -1;
  bot->scenario.puzzle_addr = 0;

  bot->scenario.num_rows = 0;
  bot->scenario.num_cols = 0;
  bot->scenario.puzzle_board = NULL;
}

/**
 * Initialize bot energy
 */
void init_bots_energy(bot_state_t *bot, long init_energy) {
	bot->scenario.bot_energy = init_energy;
	bot->scenario.out_of_energy = false;
	bot->scenario.sent_out_of_energy_interrupt = false;
}

/**
 * Update (Add) bot energy for solving puzzle
 * energy could not exceed the MAX_BOT_ENERGY
 */
void add_bots_energy(int id, int num_solved) {
    double energy_gained = num_solved * ENERGY_AWARD_FACTOR;
    robots[id].scenario.bot_energy = 
	    min(MAX_BOT_ENERGY, robots[id].scenario.bot_energy + energy_gained);
}

/**
 * Update (decrease) bot energy for smash fruit
 * energy could not go below zero
 */
void dec_bot_energy(int id) {
    robots[id].scenario.bot_energy = 
	    max(0, robots[id].scenario.bot_energy - SMASH_ENERGY_COST);
}

/**
 * Update (decrease) the energy of each bot according to the 
 * fruit they carried
 */
void update_bots_energy() {
    for (int i = 0; i < num_contexts; i ++) {
	// when energy <= 0 (<0 should never happen)
	if (robots[i].scenario.bot_energy <= 0) {
	    // when energy turns to 0
	    // turn up out_of_energy flag
	    // lose all current smooshed fruits (this is EVIL)
	    if (!robots[i].scenario.out_of_energy ) {
            printf("bot %d: out of energy\n", i);
            fflush(stdout);
		// disable smooshing interrupt
		robots[i].scenario.out_of_energy = true;
		// reset sent_interrupt flag
		robots[i].scenario.sent_out_of_energy_interrupt = false;

		// empty the smooshed_fruits_ids vector
		std::vector<int> &smooshed_fruit_ids = robots[i].scenario.smooshed_fruit_ids;
		for (std::vector<int>::iterator fruit_id_iter = smooshed_fruit_ids.begin();
                fruit_id_iter != smooshed_fruit_ids.end(); ++fruit_id_iter) {
		Fruit *fruit = fruits[*fruit_id_iter];
		fruits.erase(*fruit_id_iter);
		delete fruit;
		}
		smooshed_fruit_ids.clear();

        //TODO: consider adding the fruits back into play so the other bot could catch them
        //      and it'd be more obvious you just lost a bunch of fruits
	    }
	}
	// when energy > 0
	else { 
	    // when energy turns to > 0
	    // turn down out_of_energy flag
	    if (robots[i].scenario.out_of_energy) {
	        robots[i].scenario.out_of_energy = false;
	    }
	    double energy_consumed = robots[i].scenario.smooshed_fruit_ids.size() * ENERGY_DRAIN_FACTOR; 
	    robots[i].scenario.bot_energy = 
		    max(robots[i].scenario.bot_energy - energy_consumed, 0);
	}
    }
}



/**
 * Called in menu.cpp to init scenario world.
 */
void
world_initialize() {
  if (spimbot_randommap) {
    if (spimbot_randomseed == -1) {
      puzzle_rng.seed(time(NULL));
      seeded_rng.seed(time(NULL));
    }
    else {
      puzzle_rng.seed(spimbot_randomseed);
      seeded_rng.seed(spimbot_randomseed);
    }
  }
  else {
    puzzle_rng.seed(time(NULL));
    seeded_rng.seed(42);
  }

  if (spimbot_largemap)
      drawing_scaling = 2.0;

  MapWindow->resize(SCALE(WORLD_SIZE), SCALE(WORLD_SIZE + LABEL_SPACE));

  bot_initialize(0, /* random */ 0);

  bot_initialize(1, /* random */ 0);

  MapWindow->repaint();
}

/**
 * Called during bot_motion_update to allow
 * scenario chance to affect bot.
 * TODO: change name to bot_scenario_update
 */
bool
bot_motion_update_scenario(bot_state_t *bot) {
  if(cycle % ENEMY_BOT_CLOCK == 0)
  {
    bot->scenario.x = bot->x;
    bot->scenario.y = bot->y;
  }
  return false;
}

unsigned lower_bound = 0;
unsigned upper_bound = 0;

/**
 * Simple sanity check on bot memory addresses. Be sure to set lower_bound and
 * upper_bound correctly.
 */
void
checked_set_mem_word(int context, unsigned address, unsigned value) {
  if ((address < lower_bound) || (address >= upper_bound)) {
    printf("overflow: 0x%x 0x%x 0x%x\n", address, lower_bound, upper_bound);
    fflush(stdout);
  }

  set_mem_word(context, address, value);
}

/**
 * Simple sanity check on bot memory addresses. Be sure to set lower_bound and
 * upper_bound correctly.
 */
void
checked_set_mem_byte(int context, unsigned address, unsigned char value) {
  if ((address < lower_bound) || (address >= upper_bound)) {
    printf("overflow: 0x%x 0x%x 0x%x\n", address, lower_bound, upper_bound);
    fflush(stdout);
  }

  set_mem_byte(context, address, value);
}

/**
 * Writes fruit data to spimbot memory.
 */
void write_fruit_data(bot_state_t *bot) {
    struct fruit_information_t data;
    int c = bot->context;
    unsigned sa = bot->scenario.scan_address;
    int cur_fruit = 0;

    lower_bound = bot->scenario.scan_address;
    upper_bound =
        MAX_NUM_FRUITS * sizeof(struct fruit_information_t) + lower_bound + 4;

    std::map<int, Fruit *>::iterator fruit_iter = fruits.begin();
    while (fruit_iter != fruits.end()) {
        if(fruit_iter->second->is_smooshed() == -1) {
            data = fruit_iter->second->get_fruit_information();
            checked_set_mem_word(c, cur_fruit * sizeof(struct fruit_information_t) +sa, data.id);
            checked_set_mem_word(c, cur_fruit * sizeof(struct fruit_information_t) +sa + 4, data.points);
            checked_set_mem_word(c, cur_fruit * sizeof(struct fruit_information_t) +sa + 8, data.x);
            checked_set_mem_word(c, cur_fruit * sizeof(struct fruit_information_t) +sa + 12, data.y);
            cur_fruit++;
        }
        fruit_iter++;
    }
        checked_set_mem_word(c, cur_fruit * sizeof(struct fruit_information_t) +sa + 0, 0);
}

/**
 * Destroys the puzzle board and sol word in the scenario struct if there is
 * one.
 */
void destroy_puzzle(scenario_bot_state_t *scenario) {
    if (scenario->puzzle_board) {
        for (int row = 0; row < scenario->num_rows; ++row) {
            delete scenario->puzzle_board[row];
            scenario->puzzle_board[row] = NULL;
        }
        delete scenario->puzzle_board;
        scenario->puzzle_board = NULL;
        scenario->num_rows = 0;
        scenario->num_cols = 0;
    }

    if (scenario->word) {
        delete scenario->word;
        scenario->word = NULL;
        scenario->word_len = 0;
    }
}

/**
 * Allocates an empty puzzle board and sol word in the scenario struct. Will
 * automatically destroy puzzle that is already there.
 */
void allocate_puzzle(scenario_bot_state_t *scenario, int num_rows, int num_cols, int word_len) {
    destroy_puzzle(scenario);

    scenario->puzzle_board = new boardElem* [num_rows];
    for (int row = 0; row < num_rows; ++row) {
        scenario->puzzle_board[row] = new boardElem[num_cols];
    }
    scenario->num_rows = num_rows;
    scenario->num_cols = num_cols;

    scenario->word = new char[word_len+1];
    // NULL terminate just in case
    scenario->word[word_len] = '\0';
    scenario->word_len = word_len;
}

/**
 * Delivers puzzle store in bot.scenario.puzzle_board to the bot's memory.
 */
void deliver_puzzle(const bot_state_t *bot) {
    const scenario_bot_state_t *scenario = &bot->scenario;
    if (!scenario->puzzle_addr) {
        printf("bot %d: puzzle_addr was NULL somehow\n", bot->context);
        fflush(stdout);
        return;
    } else if (!scenario->puzzle_board) {
        printf("bot %d: puzzle_board was NULL somehow\n", bot->context);
        fflush(stdout);
        return;
    }

    lower_bound = scenario->puzzle_addr;
    upper_bound = lower_bound + 8 + scenario->num_rows * scenario->num_cols;

    if (spimbot_debug) {
      printf("bot %d: Starting to write puzzle of size %d x %d with word %s\n", bot->context, scenario->num_rows, scenario->num_cols, scenario->word);
      fflush(stdout);
    }

    checked_set_mem_word(bot->context, scenario->puzzle_addr, scenario->num_rows);
    checked_set_mem_word(bot->context, scenario->puzzle_addr + 4, scenario->num_cols);

    mem_addr puzzle_ptr = scenario->puzzle_addr + 8;
    for (int row = 0; row < scenario->num_rows; ++row) {
        for (int col = 0; col < scenario->num_cols; ++col) {
            checked_set_mem_byte(bot->context, puzzle_ptr++, scenario->puzzle_board[row][col].letter);
        }
    }

    if (spimbot_debug) {
      printf("bot %d: Finished writing puzzle to memory\n", bot->context);
      fflush(stdout);
    }
}

/**
 * Sends interrupts to bot. Also updates things related to interrupts.
 * Return value is bitwise OR with cause
 */
int
bot_io_update_scenario(bot_state_t *bot) {
  reg_image_t &reg_image = reg_images[bot->context];
  int cause = 0;

  if (bot->scenario.total_smoosh_fruits > bot->scenario.total_smoosh_interrupts_sent &&
      !bot->scenario.out_of_energy && // disable smoosh when out of energy
      INTERRUPTS_ON(reg_image) &&
      (CP0_Status(reg_image) & FRUIT_SMOOSHED_INT_MASK) &&
      ((CP0_Cause(reg_image) & FRUIT_SMOOSHED_INT_MASK) == 0)) {
    cause |= FRUIT_SMOOSHED_INT_MASK;
    bot->scenario.total_smoosh_interrupts_sent++;
    if (spimbot_debug) {
      printf("bot %d: received FRUIT_SMOOSHED interrupt\n", bot->context);
      fflush(stdout);
    }
  }

  if (bot->scenario.out_of_energy &&
      !bot->scenario.sent_out_of_energy_interrupt &&
	INTERRUPTS_ON(reg_image) &&
      (CP0_Status(reg_image) & OUT_OF_ENERGY_INT_MASK) &&
      ((CP0_Cause(reg_image) & OUT_OF_ENERGY_INT_MASK) == 0)) {
    cause |= OUT_OF_ENERGY_INT_MASK;
    // Set flag to avoid sending interrupt again
    bot->scenario.sent_out_of_energy_interrupt = true;
    if (spimbot_debug) {
      printf("bot %d: received OUT_OF_ENERGY interrupt\n", bot->context);
      fflush(stdout);
    }
  }

  if (bot->scenario.puzzle_request_cycle != -1 &&
          cycle - bot->scenario.puzzle_request_cycle >= PUZZLE_DELAY_CYCLES &&
          INTERRUPTS_ON(reg_image) &&
          (CP0_Status(reg_image) & REQUEST_PUZZLE_INT_MASK) &&
          ((CP0_Cause(reg_image) & REQUEST_PUZZLE_INT_MASK) == 0)) {
      cause |= REQUEST_PUZZLE_INT_MASK;
      bot->scenario.puzzle_request_cycle = -1;
      if (spimbot_debug) {
          printf("bot %d: received REQUEST_PUZZLE interrupt, delivering puzzle\n", bot->context);
          fflush(stdout);
      }

      boost::random::uniform_int_distribution<int> rand_rowcols(PUZZLE_MIN_ROW_COL, PUZZLE_MAX_ROW_COL);
      int num_rows = rand_rowcols(puzzle_rng);
      int num_cols = rand_rowcols(puzzle_rng);
      int base_length = min(num_rows, num_cols);
      boost::random::uniform_int_distribution<int> rand_length(-base_length / 4, base_length / 4);
      int word_length = base_length + rand_length(puzzle_rng);
      allocate_puzzle(&bot->scenario, num_rows, num_cols, word_length);
      generatePuzzle(bot->scenario.puzzle_board, bot->scenario.num_rows, bot->scenario.num_cols);
      extractWord(bot->scenario.puzzle_board, bot->scenario.word,
                  bot->scenario.num_rows, bot->scenario.num_cols,
                  &bot->scenario.word_len, true);
      deliver_puzzle(bot);
  }

  return cause;
}

/**
 * Adds the smooshed fruit to the bots smooshed_fruit_ids and sets up
 * FRUIT_SMOOSHED interrupt. fruits[fruit_id]->smooshed should be set before
 * calling this function.
 */
void
smoosh_fruit_to_bot(int fruit_id, int bot_num) {
    robots[bot_num].scenario.smooshed_fruit_ids.push_back(fruit_id);
    robots[bot_num].scenario.total_smoosh_fruits++;
    if (spimbot_debug) {
      printf("bot %d: smooshed fruit with id %d worth %d point(s)\n", bot_num, fruit_id, fruits[fruit_id]->get_points());
      fflush(stdout);
    }
}

/**
 * Return the idx in robots[] of the winning robot. Called in robot.c to decide
 * the winner.
 * @note -1 means tie
 */
int 
scenario_get_winner() {
  if (robots[0].scenario.points == robots[1].scenario.points) {
  	return -1;
  }
  int winner = robots[0].scenario.points > robots[1].scenario.points ? 0 : 1;
  return winner;
}

/**
 * Return pair of scores for each bot. Called in robot.c to print out the
 * scores.
 */
std::pair<int, int>
scenario_get_scores() {
  return std::make_pair(robots[0].scenario.points, robots[1].scenario.points);
}

// TODO: Figure out why this isn't called anywhere.
/**
 * Supposedly initializes what the map looks like I guess.
 */
#if 0
void
scenario_map_init(QPainter* __unused painter) {
}
#endif

/**
 * Exactly what it sounds like. Redraws the scenario specific stuff every
 * map_window redraw cycle. Bots and text are drawn on top of this stuff.
 */
void
redraw_scenario(QPainter* painter) {
    for (std::map<int, Fruit *>::iterator fruit_iter = fruits.begin();
         fruit_iter != fruits.end(); ++fruit_iter) {
        fruit_iter->second->draw(painter);
    }

    for (int i = 0; i < num_contexts; ++i) {
        // Scaled energy that will be out of 200 pixels
        double scaled_energy = 200 * robots[i].scenario.bot_energy / MAX_BOT_ENERGY;

        QColor color = QColor(robots[i].color);
        painter->setPen(color);
        painter->setBrush(color);
        painter->drawRect(SCALE(64), SCALE(WORLD_SIZE + 15 + (i*20)), SCALE(scaled_energy), SCALE(5));
    }
}

/**
 * Write string to the box right below the map. The bot's name will be to the
 * right so there isn't much space.
 */
void
scenario_set_bot_string(char *str, int i) {
  sprintf(str, "%d", robots[i].scenario.points);
}

/**
 * Generate the solution linked list from bot memory. Returns head to linked
 * list. Don't forget to free!
 */
node_t *generate_sol_linked_list(bot_state_t *bot, mem_addr bot_cur_node) {
    mem_word x = read_mem_word(bot->context, bot_cur_node);
    mem_word y = read_mem_word(bot->context, bot_cur_node + 4);
    node_t *head = new node_t(x, y);
    if (spimbot_debug) {
        printf("bot %d: sol: %d,%d:%c", bot->context, x, y, bot->scenario.puzzle_board[x][y].letter);
    }

    node_t *cur_node = head;
    while ((bot_cur_node = read_mem_word(bot->context, bot_cur_node + 8))) {
        mem_word x = read_mem_word(bot->context, bot_cur_node);
        mem_word y = read_mem_word(bot->context, bot_cur_node + 4);

        cur_node->next = new node_t(x, y);
        cur_node = cur_node->next;

        if (spimbot_debug) {
            printf(" %d,%d:%c", x, y, bot->scenario.puzzle_board[x][y].letter);
        }
    }

    if (spimbot_debug) {
        printf("\n");
        fflush(stdout);
    }
    return head;
}

void destroy_linked_list(node_t *cur_node) {
    if (cur_node) {
        destroy_linked_list(cur_node->next);
        cur_node->next = NULL;
        delete cur_node;
    }
}

/**
 * Processing memory IO writes made by bot
 * @param bot_index the index of the bot in robots
 * @param addr the address that was written
 * @param value the value that was written
 *
 * @return false if we didn't handle the interrupt
 *         true if we handled it
 */
int
scenario_write_spimbot_IO(int context, mem_addr addr, mem_word value) {
  bot_state_t *bot = &robots[context];
  switch (addr) {
  case FRUIT_SMOOSHED_ACK: {
    CP0_Cause(reg_images[bot->context]) &= ~FRUIT_SMOOSHED_INT_MASK;
    if (spimbot_debug) {
      printf("bot %d: acknowledged FRUIT_SMOOSHED\n", context);
      fflush(stdout);
    }
    break;
  }
  case OUT_OF_ENERGY_ACK: {
    CP0_Cause(reg_images[bot->context]) &= ~OUT_OF_ENERGY_INT_MASK;
    if (spimbot_debug) {
      printf("bot %d: acknowledged OUT_OF_ENERGY\n", context);
      fflush(stdout);
    }
    break;
  }
  case REQUEST_PUZZLE_ACK:
    CP0_Cause(reg_images[bot->context]) &= ~REQUEST_PUZZLE_INT_MASK;
    if (spimbot_debug) {
      printf("bot %d: acknowledged REQUEST_PUZZLE\n", context);
      fflush(stdout);
    }
    break;
  case FRUIT_SMASH: {
    dec_bot_energy(bot->context); // add energy cost for smashing
    if (bot->scenario.smooshed_fruit_ids.empty()) {
      if (spimbot_debug) {
        printf("bot %d: attempted to smash nonexistent fruit >:(\n", context);
        fflush(stdout);
      }
      break;
    }

    int fruit_id = bot->scenario.smooshed_fruit_ids.back();
    bot->scenario.smooshed_fruit_ids.pop_back();
    Fruit *fruit = fruits[fruit_id];
    int fruit_points = fruit->get_points();
    fruits.erase(fruit_id);
    delete fruit;

    if (bot->y < 299.0 - BOT_RADIUS) {
      if (spimbot_debug) {
        printf("bot %d: attempted to smash fruit when not at bottom of map >:(\n", context);
        printf("bot %d: lost fruit with id %d\n", context, fruit_id);
        fflush(stdout);
      }
      break;
    }

    reg_image_t &reg_image = reg_images[context];
    if ((CP0_Cause(reg_image) & SB_BONK_INT_MASK) == 0) {
      if (spimbot_debug) {
        printf("bot %d: attempted to smash fruit when not in bonk interrupt >:(\n", context);
        printf("bot %d: lost fruit with id %d\n", context, fruit_id);
        fflush(stdout);
      }
      break;
    }

    bot->scenario.points += fruit_points;

    if (spimbot_debug) {
      printf("bot %d: successfully smashed fruit with id %d worth %d point(s) >:)\n", context, fruit_id, fruit_points);
      printf("bot %d: gained %d points\n", context, fruit_points);
      fflush(stdout);
    }
    break;
  }
  case FRUIT_SCAN:
    bot->scenario.scan_address = value;
    write_fruit_data(bot);
    break;
  case REQUEST_PUZZLE:
    if (spimbot_debug) {
      printf("bot %d: requested puzzle at addr %x\n", context, value);
      fflush(stdout);
    }
    bot->scenario.puzzle_request_cycle = cycle;
    bot->scenario.puzzle_addr = value;
    break;
  case SUBMIT_SOLUTION: {
    if (spimbot_debug) {
        printf("bot %d: Reading solution from bot at addr %x\n", bot->context, value);
        fflush(stdout);
    }
    bool success = false;
    if (value) {
      node_t *head = generate_sol_linked_list(bot, value);
      success = checkSolution(bot->scenario.puzzle_board, head, bot->scenario.word,
                              bot->scenario.num_rows, bot->scenario.num_cols,
                              bot->scenario.word_len);
      destroy_puzzle(&bot->scenario);
      destroy_linked_list(head);
      head = NULL;
    }
    if (success) {
        if (spimbot_debug) {
          printf("bot %d: submitted correct solution at addr %x yay!\n", context, value);
          fflush(stdout);
        }
        add_bots_energy(context, 1);
    } else {
        if (spimbot_debug) {
          printf("bot %d: submitted incorrect solution at addr %x :(\n", context, value);
          fflush(stdout);
        }
    }
    break;
  }
  case REQUEST_WORD:
    if (spimbot_debug) {
      printf("bot %d: requested puzzle word at addr %x\n", context, value);
      fflush(stdout);
    }
    if (bot->scenario.word) {
      lower_bound = value;
      upper_bound = lower_bound + bot->scenario.word_len + 1;
      int i;
      for (i = 0; i < bot->scenario.word_len; ++i) {
        checked_set_mem_byte(bot->context, value + i, bot->scenario.word[i]);
      }
      checked_set_mem_byte(bot->context, value + i, 0);
      if (spimbot_debug) {
        printf("bot %d: successfully delivered word\n", context);
        fflush(stdout);
      }
    } else {
      if (spimbot_debug) {
        printf("bot %d: requested puzzle word but there was no word to get :(\n", context);
        fflush(stdout);
      }
    }
    break;
  default:
    return 0;
  }
  return 1;
}

/**
 * Handles IO reads made by bot
 * @param bot_index the index of the bot in robots
 * @param addr the address that was written
 * @param success should be set to true if the IO was handled here
 *
 * @return the value the bot will receive
 */
mem_word
scenario_read_spimbot_IO(int context, mem_addr addr, bool *success) {
  *success = 1;
  bot_state_t *bot = &robots[context];
  switch (addr) {
    case FRUIT_SCAN:
	 return bot->scenario.scan_address;
    case GET_ENERGY:
   return (int) bot->scenario.bot_energy;
  }
  *success = 0;
  return 0;
}

// TODO: Double check this isn't useful anymore
#if 0
void
handle_wall_collisions_scenario(bot_state_t __unused *bot, double __unused delta_x, double __unused delta_y) {
}
#endif

int count_free_fruits() {
    int free_fruit_count = 0;

    std::map<int, Fruit *>::iterator fruit_iter = fruits.begin();
    while (fruit_iter != fruits.end()) {
        if(fruit_iter->second->is_smooshed() == -1)
            free_fruit_count++;
        fruit_iter++;
    }

    return free_fruit_count;
}

/**
 * Updates the scenario things that aren't updated by other update functions.
 */
void scenario_world_update() {
    static boost::random::uniform_int_distribution<unsigned> add_fruit(1, 100);

    // Generate fruits
    // This loop is extremely hacky, it could be way more elegant; sorry guys
    int free_fruits = count_free_fruits();
    if(free_fruits < MAX_NUM_FRUITS && cycle % FRUIT_CLOCK == 0)
    {
        int p = add_fruit(seeded_rng);
        if(p < 2 * pow(abs(free_fruits - MAX_NUM_FRUITS), 1.2))
        {
            p = add_fruit(seeded_rng);
            Fruit *f = NULL;
            int x0 = cycle % 250 + 25;
            if(p < 40)
                f = new Lemon(x0, 0, 0, 5 * VELOCITY_SCALING_FACTOR, 0, 1);
            else if(p < 60)
            {
                int accel_scale = add_fruit(seeded_rng) % 3;
                accel_scale = (p % 2 == 0) ? 5 - accel_scale : accel_scale - 4;
                f = new Mango(x0, 0, 0, 9 * VELOCITY_SCALING_FACTOR,
                              accel_scale * 0.00000001, 2);
            }
            else if(p < 75)
            {
                int x_vel = add_fruit(seeded_rng) % 5;
                x_vel = (p % 2 == 0) ? 10 - x_vel : x_vel - 10;
                int y_vel = add_fruit(seeded_rng) % 7;
                y_vel = 10 - y_vel;
                f = new Guava(x0, 0, x_vel * VELOCITY_SCALING_FACTOR,
                              y_vel * VELOCITY_SCALING_FACTOR, 0, 4);
            }
            else if(p < 90)
            {
                int x_vel = add_fruit(seeded_rng) % 5;
                x_vel = (p % 2 == 0) ? 10 - x_vel : x_vel - 10;
                int accel_scale = add_fruit(seeded_rng) % 3;
                accel_scale = (p % 2 == 0) ? 5 - accel_scale : -accel_scale;
                f = new Loquat(x0, 0, x_vel * VELOCITY_SCALING_FACTOR,
                              6 * VELOCITY_SCALING_FACTOR,
                              accel_scale * 0.00000001, 6);
            }
            else
            {
                //default velocity values
                int x_vel = add_fruit(seeded_rng) % 5;
                x_vel = (p % 2 == 0) ? 10 - x_vel : x_vel - 10;
                int y_vel = add_fruit(seeded_rng) % 7 + 4;
                //velocity to add if bots get to close to cherry
                int added_x_vel = add_fruit(seeded_rng) % 8 + 3;
                int added_y_vel = add_fruit(seeded_rng) % 8 + 5;

                f = new Cherry(x0, 0, x_vel * VELOCITY_SCALING_FACTOR,
                              y_vel * VELOCITY_SCALING_FACTOR,
                              0, 10, added_x_vel * VELOCITY_SCALING_FACTOR,
                              added_y_vel * VELOCITY_SCALING_FACTOR);
            }
            fruits.insert(std::make_pair(f->get_id(), f));
        }
    }

    // Update fruits locations
    std::map<int, Fruit *>::iterator fruit_iter = fruits.begin();
    while (fruit_iter != fruits.end()) {
        // If fruit hit bottom of screen
        if (fruit_iter->second->update_location(robots)) {
            Fruit *fruit = fruit_iter->second;
            fruits.erase(fruit_iter++);
            delete fruit;
        } else {
            ++fruit_iter;
        }
    }

    // Update energy
    update_bots_energy();
}
