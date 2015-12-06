#ifndef CHERRY_H
#define CHERRY_H

#include <QColor>

#include "fruit.h"
#include "robot.h"

class Cherry : public Fruit
{
public:
    Cherry();
    Cherry(double x0, double y0, double velocity_x, double velocity_y,
        double acceleration_y, int points, double added_velocity_x,
        double added_velocity_y);
    virtual void draw(QPainter *painter);
    virtual void check_collision(bot_state_t *bots);
    virtual int update_location(bot_state_t *bots);

private:
    float radius;
    QColor color;
    //added velocity cherry will receive if a bot dares to enter its territory
    double  added_velocity_x, added_velocity_y;
    //cycle count when the bot first encroaches on cherry's territory
    int     t1;
    //radius within which a bot's presence in can trigger cherry runaway
    double territory_radius;
};

#endif //#ifdef CHERRY_H
