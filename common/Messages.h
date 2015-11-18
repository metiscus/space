#ifndef MESSAGES_H_INCLUDED
#define MESSAGES_H_INCLUDED

#include <cstdint>

const uint32_t MaxPlayerCount = 10;

struct ServerShipUpdate
{
    uint32_t ship_id;
    float position[3];
    float velocity[3];
    float orientation;
    float angularVelocity;
};

const uint32_t PlayerUpdateMagic = 0x00010203;
struct ServerPlayerUpdateMsg
{
    uint32_t player_count;
    ServerShipUpdate updates[MaxPlayerCount];
};

enum ClientMessageType
{
    PlayerUpdateMsg = 0x100
};

const uint32_t PlayerNameMax = 100;

struct ClientShipUpdate
{
    bool fired;
    char name[PlayerNameMax];
    float force[3];
    float torque;
};

struct ClientMessage
{
    uint32_t type;
    union
    {
        ClientShipUpdate update;
    };
};

#endif // MESSAGES_H_INCLUDED
