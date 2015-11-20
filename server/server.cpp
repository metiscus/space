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
#include <cstdarg>
#include <random>
#include <sys/time.h>

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
                Vector3f velocity = pShip->GetVelocity();
                Vector3f position = pShip->GetPosition();
                if(position[0] > WorldSize)
                {
                    position[0] = WorldSize;
                    velocity[0] = 0;
                }
                if(position[0] < 0.0)
                {
                    position[0] = 0;
                    velocity[0] = 0;
                }

                if(position[1] > WorldSize)
                {
                    position[1] = WorldSize;
                    velocity[1] = 0;
                }
                if(position[1] < 0.0)
                {
                    position[1] = 0;
                    velocity[1] = 0;
                }

                pShip->SetPosition(position);
                pShip->SetVelocity(velocity);

                pShip->GetServerUpdateMessage(svr_player_update.updates[playerCount++]);
            }
        }

        // Check for collisions
        for(auto a : g_objects)
        {
            for(auto b : g_objects)
            {
                if(a!=b)
                {
                    float distanceSqr = (b->GetPosition() - a->GetPosition()).squaredNorm();
                    if(distanceSqr <= pow(a->GetRadius(), 2.0) + pow(b->GetRadius(), 2.0))
                    {
                        // the objects did collide
                        Vector3f collision = (b->GetPosition() - a->GetPosition()) / (sqrt(distanceSqr) + 0.01);
                        float speedA = a->GetVelocity().norm();
                        float speedB = b->GetVelocity().norm();
                        a->SetVelocity(-0.85 * speedA * collision);
                        b->SetVelocity(0.85 * speedB * collision);
                    }
                }
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

    std::random_device rd;
    std::mt19937 gen(rd());

    while(true)
    {
        zmq::message_t null_message(0);
        static uint32_t serial = 0;
        //log("calling receive");
        zmq::message_t message(sizeof(ClientMessage));
        command.recv(&message);
        if(message.size() == sizeof(ClientMessage))
        {
            ClientMessage clientMsg;
            memcpy(&clientMsg, message.data(), sizeof(ClientMessage));
            //log("received a client message of type (%u)", clientMsg.type);

            struct timeval tv;
            gettimeofday(&tv, nullptr);
            double now = tv.tv_sec + (double)tv.tv_usec / 1e6;
            double then = (double)clientMsg.timestamp / 1e6;
            if(fabs(then-now) > 0.1)
            {
                log("excessive frametime: dt= %lf", then-now);
            }

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
                // Randomize the start locations of new ships
                std::uniform_real_distribution<> dis(0.0, WorldSize * 0.8);
                pShip->SetPosition(Vector3f(dis(gen), dis(gen), 0.0));
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
