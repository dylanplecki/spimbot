#ifndef FRUIT_CATCHING_H
#define FRUIT_CATCHING_H

#include <vector>

#undef K
#include <QBrush>
#define K 1024
#include <utility>

#include "../CPU/spim.h"
#include "../CPU/string-stream.h"
#include "../CPU/inst.h"
#include "../CPU/reg.h"
#include "../CPU/mem.h"

#include "puzzlegenerator.h"

typedef struct
{
    int points;
    unsigned scan_address;

    // vector of fruits that are currently smooshed to this bot
    std::vector<int> smooshed_fruit_ids;

    // The following 2 fields are used to make sure the correct number of
    // FRUIT_SMOOSHED interrupts are delivered even if we need to delay an
    // interrupt due to them already being in the interrupt handler.

    // Running total of all fruits ever smooshed to this bot. Incremented when
    // a fruit is smooshed to this bot. See smoosh_fruit_to_bot().
    long total_smoosh_fruits;
    // Running total of all FRUIT_SMOOSHED interrupts sent. Incremented only
    // when a FRUIT_SMOOSHED interrupt is successfully sent.
    long total_smoosh_interrupts_sent;
    // Energy
    double bot_energy;
    // Flag for out of energy
    bool out_of_energy;
    // Flag for keeping track of whether interrupt was sent or not
    bool sent_out_of_energy_interrupt;
    // Outdated X Location of bot (to simulate lag when reading enemy position)
    double x;
    // Outdated Y Location of bot (to simulate lag when reading enemy position)
    double y;

    // -1 if no puzzle requested yet, otherwise set to the cycle number that
    // the request was made
    int puzzle_request_cycle;
    // address to store puzzle in
    mem_addr puzzle_addr;

    // Puzzle stuff
    int num_rows;
    int num_cols;
    boardElem** puzzle_board;

    int word_len;
    char *word;
} scenario_bot_state_t;

#define FRUIT_SCAN ((mem_addr) 0xffff005c)

#define SPIMBOT_ENEMY_X   ((mem_addr) 0xffff00a0) // this has been defined in robot.h SPIMBOT_OTHER_X_ADDR
#define SPIMBOT_ENEMY_Y   ((mem_addr) 0xffff00a4)

#define FRUIT_SMOOSHED_ACK ((mem_addr) 0xffff0064)
#define FRUIT_SMOOSHED_INT_MASK 0x2000

#define FRUIT_SMASH ((mem_addr) 0xffff0068)

#define OUT_OF_ENERGY_ACK ((mem_addr) 0xffff00c4)
#define OUT_OF_ENERGY_INT_MASK 0x4000 // TODO: check if used somewhere else

#define GET_ENERGY ((mem_addr) 0xffff00c8)

#define REQUEST_PUZZLE ((mem_addr) 0xffff00d0)
#define SUBMIT_SOLUTION ((mem_addr) 0xffff00d4)

#define REQUEST_PUZZLE_ACK ((mem_addr) 0xffff00d8)
#define REQUEST_PUZZLE_INT_MASK 0x800

#define REQUEST_WORD ((mem_addr) 0xffff00dc)


struct bot_state_t;

void world_initialize();
void scenario_bot_initialize(bot_state_t *bot);
bool bot_motion_update_scenario(bot_state_t *bot);
int bot_io_update_scenario(bot_state_t *bot);
int scenario_get_winner();
std::pair<int, int> scenario_get_scores();
void scenario_map_init(QPainter* painter);
void redraw_scenario(QPainter* painter);
void scenario_set_bot_string(char *str, int i);
int scenario_write_spimbot_IO(int context, mem_addr addr, mem_word value);
mem_word scenario_read_spimbot_IO(int context, mem_addr addr, bool *success);
int count_free_fruits();
void scenario_world_update();

// Scenario specific functions

void smoosh_fruit_to_bot(int fruit_id, int bot_num);

#endif // #ifdef FRUIT_CATCHING_H
