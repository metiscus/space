#include "Object.h"
#include "Ship.h"
#include "Network.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <memory>
#include "Messages.h"
#include <thread>
#include <mutex>
#include <cstdarg>
#include <random>
#include <sys/time.h>
#include <unordered_map>
#include <set>

std::mutex g_objects_mtx;
typedef std::shared_ptr<Object> ObjectPtr;
std::vector<ObjectPtr> g_objects;
std::hash<std::string> player_hash_fn;

std::unordered_map<std::string, uint32_t> g_update_map;

std::set<Endpoint> remotes;

void command_thread();

void log(const char* pattern, ...);

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_TIMER);

    Endpoint state_ep(std::string("127.0.0.1"), 5555);
    MailboxPtr gamestate = Mailbox::Create(state_ep);

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

        svr_player_update.player_count = playerCount;
        if(playerCount != playerCountOld)
        {
            log("new player count: %u", playerCount);
            playerCountOld = playerCount;
        }

        struct timeval tv;
        gettimeofday(&tv, nullptr);
        svr_player_update.timestamp = tv.tv_sec * 1e6 + tv.tv_usec;

        //Prepare message for send
        NetMessagePtr state_message = NetMessage::Create(state_ep);
        state_message->SetData(&svr_player_update, sizeof(svr_player_update));

        //log("remotes size=%u", remotes.size());
        for(auto& client_ep : remotes)
        {
            //log("sending to %s:%u", client_ep.GetHost().c_str(), client_ep.GetPort());
            state_message->SetRecipient(client_ep);
            if(!gamestate->SendMessage(state_message))
            {
                log("removing %s:%u as a remote", client_ep.GetHost().c_str(), client_ep.GetPort());
                remotes.erase(client_ep);
            }
        }
        g_objects_mtx.unlock();

    }

    t1.join();
    return 0;
}

void command_thread()
{
    Endpoint command_ep(std::string("127.0.0.1"), 5556);
    MailboxPtr command = Mailbox::Create(command_ep);
    std::vector<MailboxPtr> boxes;
    boxes.push_back(command);

    log("running command thread");

    std::random_device rd;
    std::mt19937 gen(rd());

    while(true)
    {
        SDL_Delay(1);

        while(0 != Mailbox::UpdateMailboxes(boxes, 800));
        //fprintf(stderr, "message count = %u\n", command->GetMessageCount());
        for(uint32_t messageCount = command->GetMessageCount(); messageCount>0; --messageCount)
        {
            NetMessagePtr message = command->GetMessage();

            if(message->GetData().size() == sizeof(ClientMessage))
            {
                ClientMessage *clientMsg = (ClientMessage*)&(message->GetData()[0]);
                //log("received a client message of type (%u)", clientMsg->type);

                g_objects_mtx.lock();
                remotes.insert(message->GetSender());

                // look to see if the current player exists
                std::string name(clientMsg->update.name);
                Ship* pShip = nullptr;
                for(auto obj : g_objects)
                {
                    pShip = dynamic_cast<Ship*>(obj.get());
                    if(pShip && name == pShip->GetName())
                    {
                        break;
                    }
                    else
                    {
                        pShip = nullptr;
                    }
                }

                // Ship was not in list
                if(!pShip)
                {
                    pShip = new Ship(name);
                    g_objects.push_back(ObjectPtr(pShip));
                    // Randomize the start locations of new ships
                    std::uniform_real_distribution<> dis(0.0, WorldSize * 0.8);
                    pShip->SetPosition(Vector3f(dis(gen), dis(gen), 0.0));
                }

                struct timeval tv;
                gettimeofday(&tv, nullptr);
                double now  = tv.tv_sec + (double)tv.tv_usec / 1e6;
                double then = (double)clientMsg->timestamp / 1e6;
                if((now-then) > 0.012)
                {
                    log("excessive frametime: dt= %lf", now-then);
                }

                // Update the ship
                pShip->AddForce(Vector3f(clientMsg->update.force[0], clientMsg->update.force[1], clientMsg->update.force[2]));
                pShip->AddTorque(clientMsg->update.torque);
                g_objects_mtx.unlock();
            }
            else
            {
                log("got a strange sized message of size: %u", message->GetData().size());
            }
        }
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
