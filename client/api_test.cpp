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

#include <cstdarg>

std::mutex g_objects_mtx;
typedef std::shared_ptr<Object> ObjectPtr;
std::vector<ObjectPtr> g_objects;

zmq::context_t* g_context = nullptr;

void command_thread();

void log(const char* pattern, ...);

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_TIMER);
    g_context = new zmq::context_t(1);

    ClientMessage updateMessage;
    updateMessage.type = PlayerUpdateMsg;

    Ship *pShip = new Ship("api test player");
    //pShip->AddForce(Vector3f(0.0, 0.0, 10.0));
    pShip->GetClientUpdateMessage(updateMessage.update);

    zmq::socket_t command (*g_context, ZMQ_REQ);
    command.connect("tcp://localhost:5556");

    zmq::message_t message(sizeof(updateMessage));
    memcpy(message.data(), &updateMessage, sizeof(updateMessage));
    command.send(message);

    zmq::message_t recvd_msg;
    command.recv(&recvd_msg);

    log("reply received");
    usleep(10000);

    log("listening for state updates");
    zmq::socket_t gamestate (*g_context, ZMQ_SUB);
    gamestate.connect("tcp://localhost:5555");
    gamestate.setsockopt(ZMQ_SUBSCRIBE, &PlayerUpdateMagic, sizeof(PlayerUpdateMagic));
    for(int i=0; i<10; ++i)
    {
        zmq::message_t state_msg(2 * sizeof(ServerPlayerUpdateMsg));
        gamestate.recv(&state_msg);
        if(state_msg.size() == sizeof(ServerPlayerUpdateMsg))
        {
            ServerPlayerUpdateMsg svr_update;
            memcpy(&svr_update, state_msg.data(), state_msg.size()); // warning overflow possible
            log("state message contained %u states", svr_update.player_count);
            if(svr_update.player_count > 0)
            {
                log("state 0: position < %f %f %f >",
                    svr_update.updates[0].position[0],
                    svr_update.updates[0].position[1],
                    svr_update.updates[0].position[2]);
                log("state 0: velocity < %f %f %f >",
                    svr_update.updates[0].velocity[0],
                    svr_update.updates[0].velocity[1],
                    svr_update.updates[0].velocity[2]);
            }
        }
    }
    return 0;
}

void log(const char* pattern, ...)
{
    va_list l;
    va_start(l, pattern);
    fprintf(stderr, "[client] : ");
    vfprintf(stderr, pattern, l);
    fprintf(stderr, "\n");
    va_end(l);
}
