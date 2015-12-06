#include <QPainter>

#include "mango.h"

Mango::Mango() : Fruit(), radius(3), color(QColor(200, 50, 20, 255))
{}

Mango::Mango(double x0, double y0, double velocity_x, double velocity_y,
    double acceleration_y, int points) : Fruit(x0, y0, velocity_x, velocity_y,
    acceleration_y, points), radius(3), color(QColor(200, 50, 20, 255))
{}

void Mango::draw(QPainter *painter)
{
    QPen pen(color);
    QBrush brush(Qt::SolidPattern);
    brush.setColor(color);
    painter->setBrush(brush);
    painter->setPen(pen);
    painter->drawEllipse(SCALE(x - radius), SCALE(y - 2 * radius),
        SCALE(2 * radius), SCALE(4 * radius));
}

void Mango::check_collision(bot_state_t *bots)
{
    if(!bots) return;
    if (smooshed == -1)
    {
        for (int i = 0; i < num_contexts; i++)
        {
            int index = i ^ (cycle & (num_contexts-1));
            if (bots[index].scenario.out_of_energy)
                continue;
            double delta_x = bots[index].x - x;
            double delta_y = bots[index].y - y;
            double dist = sqrt(delta_x * delta_x + delta_y * delta_y);
            if (dist < radius)
            {
                smooshed = index;
                smoosh_fruit_to_bot(id, index);
                // Need to break to avoid calling smoosh_fruit_to_bot more than
                // once per fruit to avoid extra interrupts.
                break;
            }
        }
    }
}
