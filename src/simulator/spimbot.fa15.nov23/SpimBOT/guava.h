#ifndef GUAVA_H
#define GUAVA_H

#include <QColor>

#include "fruit.h"

class Guava : public Fruit
{
public:
    Guava();
    Guava(double x0, double y0, double velocity_x, double velocity_y,
        double acceleration_y, int points);
    virtual void draw(QPainter *painter);
    virtual void check_collision(bot_state_t *bots);

private:
    int radius;
    QColor color;
};

#endif //#ifdef GUAVA_H
