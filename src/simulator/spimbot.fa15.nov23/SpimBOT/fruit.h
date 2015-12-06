#ifndef FRUIT_H
#define FRUIT_H

#include "robot.h"

extern int cycle;

struct fruit_information_t
{
    int id;
    int points;
    int x, y;
};

class Fruit
{
public:
    Fruit();
    virtual ~Fruit() {}
    Fruit(double x0, double y0, double velocity_x, double velocity_y,
        double acceleration_y, int points);
    //Sets smooshed variable appropriately if fruit has collided
    virtual void  check_collision(bot_state_t *bots) = 0;
    virtual void draw(QPainter *painter) = 0;
    //Returns 1 if fruit crashes (hits bottom of screen)
    virtual int update_location(bot_state_t *bots);
    fruit_information_t get_fruit_information();
    int get_id();
    int get_points() { return points; }
    int is_smooshed();

protected:
    double  x, y;           //current x, y values of bot
    int     smooshed;       //-1 if not caught, 0 if caught by bot 0, 1 for 1
    int     id;             //unique fruit id
    double  x0, y0;         //initial x, y values of bot
    double  velocity_x, velocity_y; //velocity in pixels/cycle
    double  acceleration_y; //acceleration in pixels/cycle
    int     t0;             //initial cycle count the fruit appears on
    int     crashed();      //returns if fruit has hit bottom of screen

private:
    int     points;         //number of points this fruit is worth
    static int id_count;
};

#endif //#ifdef FRUIT_H
