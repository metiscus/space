#ifndef MESSAGES_H_INCLUDED
#define MESSAGES_H_INCLUDED

#include <cstdint>

const uint32_t MaxPlayerCount = 10;

struct ServerPlayerUpdate
{
    bool valid;
    uint32_t player;
    float position[3];
    float velocity[3];
    float orientation;
    float angularVelocity;
};

struct ServerPlayerUpdateMsg
{
    uint32_t player_count;
    ServerPlayerUpdate updates[MaxPlayerCount];
};

struct ClientPlayerUpdateMsg
{
    bool fired;
    float force[3];
    float torque;
};

#endif // MESSAGES_H_INCLUDED
