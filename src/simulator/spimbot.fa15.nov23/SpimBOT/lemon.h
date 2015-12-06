#ifndef LEMON_H
#define LEMON_H

#include <QColor>

#include "fruit.h"

class Lemon : public Fruit
{
public:
    Lemon();
    Lemon(double x0, double y0, double velocity_x, double velocity_y,
        double acceleration_y, int points);
    virtual void draw(QPainter *painter);
    virtual void check_collision(bot_state_t *bots);

private:
    int radius;
    QColor color;
};

#endif //#ifdef LEMON_H
