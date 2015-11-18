#include "Object.h"
#include "Ship.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <zmq.hpp>
#include <memory>
#include <functional>
#include "Messages.h"
#include <thread>
#include <mutex>

std::mutex g_objects_mtx;
typedef std::shared_ptr<Object> ObjectPtr;
std::vector<ObjectPtr> g_objects;

zmq::context_t* g_context = nullptr;

void command_thread();

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_TIMER);
    g_context = new zmq::context_t(1);
    zmq::socket_t gamestate (*g_context, ZMQ_PUB);

    gamestate.bind("tcp://*:5555");

    std::hash<std::string> player_hash_fn;
    std::thread t1(command_thread);

    while(1)
    {
        SDL_Delay(10);

        // Broadcast the status of each player
        ServerPlayerUpdateMsg svr_player_update;
        uint32_t playerCount = 0;
        g_objects_mtx.lock();
        for(auto itr : g_objects)
        {
            Ship* pShip = dynamic_cast<Ship*>(itr.get());
            if(pShip){
                svr_player_update.updates[playerCount].valid = true;
                svr_player_update.updates[playerCount].player = player_hash_fn(pShip->GetName());

                auto pos = pShip->GetPosition();
                svr_player_update.updates[playerCount].position[0] = pos[0];
                svr_player_update.updates[playerCount].position[1] = pos[1];
                svr_player_update.updates[playerCount].position[2] = pos[2];

                auto vel = pShip->GetVelocity();
                svr_player_update.updates[playerCount].velocity[0] = vel[0];
                svr_player_update.updates[playerCount].velocity[1] = vel[1];
                svr_player_update.updates[playerCount].velocity[2] = vel[2];

                svr_player_update.updates[playerCount].orientation = pShip->GetOrientation();
                svr_player_update.updates[playerCount].angularVelocity = pShip->GetAngularVelocity();

                ++playerCount;
            }
        }
        g_objects_mtx.unlock();

        zmq::message_t net_msg(sizeof(ServerPlayerUpdateMsg));
        memcpy(net_msg.data(), &svr_player_update, sizeof(svr_player_update));
        gamestate.send(net_msg);
    }

    t1.join();
    return 0;
}

void command_thread()
{
    zmq::socket_t command (*g_context, ZMQ_REP);
    command.bind("tcp://*:5556");

    while(true)
    {
        zmq::message_t request;
        command.recv(&request);

        sleep(1000);
    }
}
