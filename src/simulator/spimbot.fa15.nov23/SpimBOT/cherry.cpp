#include <QPainter>

#include "cherry.h"

Cherry::Cherry() : Fruit(), radius(1.5), color(QColor(240, 0, 10, 255)),
    added_velocity_x(0), added_velocity_y(0), t1(-1), territory_radius(50)
{}

Cherry::Cherry(double x0, double y0, double velocity_x, double velocity_y,
    double acceleration_y, int points, double added_velocity_x,
    double added_velocity_y) : Fruit(x0, y0, velocity_x, velocity_y,
    acceleration_y, points), radius(1.5), color(QColor(240, 0, 10, 255)),
    added_velocity_x(std::abs(added_velocity_x)),
    added_velocity_y(std::abs(added_velocity_y)), t1(-1), territory_radius(50)
{}

void Cherry::draw(QPainter *painter)
{
    QPen pen(color);
    QBrush brush(Qt::SolidPattern);
    brush.setColor(color);
    painter->setBrush(brush);
    painter->setPen(pen);
    painter->drawEllipse(SCALE(x - 2 * radius), SCALE(y - 2 * radius),
        SCALE(4 * radius), SCALE(4 * radius));
}

void Cherry::check_collision(bot_state_t *bots)
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

int Cherry::update_location(bot_state_t *bots)
{
    check_collision(bots);

    if(smooshed < 0)
    {
        x = velocity_x * (cycle - t0) + x0;
        y = 0.5 * acceleration_y * (cycle - t0) * (cycle - t0) +
            velocity_y * (cycle - t0) + y0;
        if(t1 > 0)
        {
            x += added_velocity_x * (cycle - t1);
            y += added_velocity_y * (cycle - t1);
        }
        else
        {
            for(int i = 0; i < num_contexts; i++)
            {
                double delta_x = bots[i].x - x;
                double delta_y = bots[i].y - y;
                double dist = sqrt(delta_x * delta_x + delta_y * delta_y);
                if(dist < territory_radius && bots[i].velocity != 0)
                {
                    t1 = cycle;
                    if(bots[i].x < x && velocity_x < 0)
                        added_velocity_x = -2 * velocity_x + added_velocity_x;
                    else if(bots[i].x > x && velocity_x > 0)
                        added_velocity_x = -2 * velocity_x - added_velocity_x;
                    else if(bots[i].x > x && velocity_x < 0)
                        added_velocity_x *= -1;
                }
            }
        }
        if(x >= WORLD_SIZE)
        {
            x0 = 0;
            y0 = y;
            velocity_y += acceleration_y * (cycle - t0);
            t0 = cycle;
        }
        else if(x <= 0)
        {
            x0 = WORLD_SIZE;
            y0 = y;
            velocity_y += acceleration_y * (cycle - t0);
            t0 = cycle;
        }
    }
    else
    {
        x = bots[smooshed].x;
        y = bots[smooshed].y - BOT_RADIUS;
    }

    return crashed();
}
