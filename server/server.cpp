#include "Object.h"
#include "Ship.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <zmq.hpp>
#include <memory>
#include "Messages.h"
#include <thread>
#include <mutex>
#include <stdarg.h>

std::mutex g_objects_mtx;
typedef std::shared_ptr<Object> ObjectPtr;
std::vector<ObjectPtr> g_objects;
std::hash<std::string> player_hash_fn;

zmq::context_t* g_context = nullptr;

void command_thread();

void log(const char* pattern, ...);

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_TIMER);
    g_context = new zmq::context_t(1);
    zmq::socket_t gamestate (*g_context, ZMQ_PUB);

    gamestate.bind("tcp://*:5555");

    std::thread t1(command_thread);

    uint32_t playerCountOld = 10;

    while(1)
    {
        SDL_Delay(10);

        uint32_t playerCount = 0;
        // Broadcast the status of each player
        ServerPlayerUpdateMsg svr_player_update;
        g_objects_mtx.lock();
        for(auto itr : g_objects)
        {
            itr->Update(0.01);
            Ship* pShip = dynamic_cast<Ship*>(itr.get());
            if(pShip){
                svr_player_update.updates[playerCount].player = player_hash_fn(pShip->GetName());
                pShip->GetUpdate(svr_player_update.updates[playerCount++]);
            }
        }

        g_objects_mtx.unlock();
        svr_player_update.player_count = playerCount;
        if(playerCount != playerCountOld)
        {
            log("new player count: %u", playerCount);
            playerCountOld = playerCount;
        }

        zmq::message_t magic(sizeof(uint32_t));
        memcpy(magic.data(), &PlayerUpdateMagic, sizeof(uint32_t));
        gamestate.send(magic, ZMQ_SNDMORE);
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

    log("running command thread");

    zmq::message_t null_message(0);
    while(true)
    {
        log("calling receive");
        zmq::message_t message(sizeof(ClientMessage)*10);
        command.recv(&message);
        if(message.size() == sizeof(ClientMessage))
        {
            ClientMessage clientMsg;
            memcpy(&clientMsg, message.data(), sizeof(ClientMessage));
            log("received a client message of type (%u)", clientMsg.type);
            g_objects_mtx.lock();
            // look to see if the current player exists
            std::string name(clientMsg.update.name);
            Ship* pShip = nullptr;
            for(auto obj : g_objects)
            {
                pShip = dynamic_cast<Ship*>(obj.get());
                if(pShip && name == pShip->GetName())
                {
                    break;
                }
            }

            // Ship was not in list
            if(!pShip)
            {
                pShip = new Ship(name);
                g_objects.push_back(ObjectPtr(pShip));
                log("adding new ship");
            }

            // Update the ship
            pShip->AddForce(Vector3f(clientMsg.update.force[0], clientMsg.update.force[1], clientMsg.update.force[2]));
            pShip->AddTorque(clientMsg.update.torque);
            g_objects_mtx.unlock();
        }
        else
        {
            log("got a strange sized message");
        }
        command.send(null_message);
        log("reply sent");
    }
}

void log(const char* pattern, ...)
{
    va_list l;
    va_start(l, pattern);
    fprintf(stderr, "[server] : ");
    vfprintf(stderr, pattern, l);
    fprintf(stderr, "\n");
    va_end(l);
}
