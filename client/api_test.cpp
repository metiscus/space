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

#include <stdarg.h>

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
    strcpy(updateMessage.update.name, "api test player");

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
        zmq::message_t state_msg;
        gamestate.recv(&state_msg);
        if(state_msg.size() == sizeof(ServerPlayerUpdateMsg))
        {
            ServerPlayerUpdateMsg svr_update;
            memcpy(&svr_update, state_msg.data(), state_msg.size()); // warning overflow possible
            log("state message contained %u states", svr_update.player_count);
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
