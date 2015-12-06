#include "fruit.h"

int Fruit::id_count = 1;

Fruit::Fruit() : x(0), y(0), smooshed(-1), id(id_count++), x0(0), y0(0),
    velocity_x(0), velocity_y(0), acceleration_y(0), t0(cycle),
    points(INT_MAX)
{}

Fruit::Fruit(double x0, double y0, double velocity_x, double velocity_y,
    double acceleration_y, int points) : x(x0), y(y0), smooshed(-1),
    id(id_count++), x0(x0), y0(y0), velocity_x(velocity_x),
    velocity_y(velocity_y), acceleration_y(acceleration_y), t0(cycle),
    points(points)
{}

int Fruit::update_location(bot_state_t *bots)
{
    check_collision(bots);

    if(smooshed < 0)
    {
        x = velocity_x * (cycle - t0) + x0;
        y = 0.5 * acceleration_y * (cycle - t0) * (cycle - t0) +
            velocity_y * (cycle - t0) + y0;
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

fruit_information_t Fruit::get_fruit_information()
{
    fruit_information_t f = {id, points, (int) x, (int) y};
    return f;
}

int Fruit::crashed()
{
    return (y >= WORLD_SIZE || y < 0) ? 1 : 0;
}

int Fruit::get_id()
{
    return id;
}

int Fruit::is_smooshed()
{
    return smooshed;
}
