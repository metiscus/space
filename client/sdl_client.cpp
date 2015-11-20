#include "Messages.h"
#include "Ship.h"

#include <zmq.hpp>
#include <SDL2/SDL.h>
#include <memory>
#include <cstdarg>
#include <cmath>
#include <sys/time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Window *g_window = nullptr;
SDL_Renderer *g_renderer = nullptr;
zmq::context_t* g_context = nullptr;

void log(const char* pattern, ...);
SDL_Texture* loadTexture(const std::string &file, SDL_Renderer *ren);

inline float DegToRad(float deg)
{
    return deg * M_PI / 180.0f;
}

//For now just connect to the server on localhost for testing
//Eventually need to allow a gui to select which server and set player name etc
int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_VIDEO);
    g_window                 = SDL_CreateWindow("Space!", 100, 100, 1000, 1000, SDL_WINDOW_SHOWN);
    SDL_Renderer *g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    if(0 != SDL_RenderSetLogicalSize(g_renderer, WorldSize, WorldSize))
    {
        log("couldn't set resolution\n");
    }

    std::shared_ptr<Ship> pShip;
    char name [1000];
    if(argc == 2)
    {
        log("Ship name is :%s", argv[1]);
        pShip = std::shared_ptr<Ship>(new Ship(argv[1]));
    }
    else {
        pShip = std::shared_ptr<Ship>(new Ship("A Ship"));
    }

    // command connection
    g_context = new zmq::context_t(1);
    zmq::socket_t command (*g_context, ZMQ_REQ);
    command.connect("tcp://localhost:5556");

    // game state connection
    zmq::socket_t gamestate (*g_context, ZMQ_SUB);
    gamestate.connect("tcp://localhost:5555");
    gamestate.setsockopt(ZMQ_SUBSCRIBE, &PlayerUpdateMagic, sizeof(PlayerUpdateMagic));

    SDL_Event e;
    bool run = true;
    // Main game loop
    //- check for input
    //- process input from user (update ship)
    //- send update to server (if needed)
    //- get an update
    //- draw
    SDL_Texture *shipTex = loadTexture("data/ships/0.png", g_renderer);
    while(run && shipTex)
    {
        // check for and process input
        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT)
            {
                run = false;
            }
            else if(e.type == SDL_KEYDOWN)
            {
                bool add_force = false;
                Vector3f force (0.0, 0.0, 0.0);
                switch (e.key.keysym.sym){
                    case SDLK_a:
                        force += Vector3f(-100.0, 0.0, 0.0);
                        break;
                    case SDLK_d:
                        force += Vector3f(100.0, 0.0, 0.0);
                        break;
                    case SDLK_w:
                        force += Vector3f(0.0, -100.0, 0.0);
                        break;
                    case SDLK_s:
                        force += Vector3f(0.0, 100.0, 0.0);
                        break;
                    case SDLK_e:
                        pShip->AddTorque(100.0);
                        break;
                    case SDLK_q:
                        pShip->AddTorque(-100.0);
                        break;
                }

                //if(add_force == true)
                {
                    // the user inputs forces relative to the ship
                    // we need to transform them in to world axes
                    float orientation = DegToRad(pShip->GetOrientation());
                    Vector3f world_force( force[0] * cos(orientation) + force[1] * sin(orientation), -force[0] * sin(orientation) + force[1] * cos(orientation), 0.0);
                    pShip->AddForce(world_force);
                    log("force %f %f", world_force[0], world_force[1]);
                }
            }
            //TODO: implement keyboard callbacks
        }

        // send update to server
        ClientMessage update_message;
        update_message.type = PlayerUpdateMsg;
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        update_message.timestamp = tv.tv_sec * 1e6 + tv.tv_usec;
        pShip->GetClientUpdateMessage(update_message.update);
        zmq::message_t message(sizeof(update_message));
        memcpy(message.data(), &update_message, sizeof(update_message));
        command.send(message);

        zmq::message_t recvd_msg(0);
        command.recv(&recvd_msg);

        // Get an update message from the server
        ServerPlayerUpdateMsg server_update_message;
        zmq::message_t state_msg(sizeof(ServerPlayerUpdateMsg));
        gamestate.recv(&state_msg, 0);
        int64_t more = 1;
        size_t more_size = sizeof(more);
        while(more_size > 0 && more == 1)
        {
            gamestate.recv(&state_msg, 0);
            gamestate.getsockopt(ZMQ_RCVMORE, &more, &more_size);
        }

        if(state_msg.size() == sizeof(ServerPlayerUpdateMsg))
        {
            memcpy(&server_update_message, state_msg.data(), state_msg.size());
        }

        // draw
        SDL_RenderClear(g_renderer);
        //log("player count: %d", server_update_message.player_count);
        for(int i=0; i<server_update_message.player_count; ++i)
        {
            SDL_Rect dst;
            dst.x = server_update_message.updates[i].position[0];
            dst.y = server_update_message.updates[i].position[1];
            dst.w = 55;
            dst.h = 70;
            //log("ori: %f", server_update_message.updates[i].orientation);
            SDL_RenderCopyEx(g_renderer, shipTex, nullptr, &dst, server_update_message.updates[i].orientation, nullptr, SDL_FLIP_NONE);
        }

        SDL_RenderPresent(g_renderer);

        pShip->Update(0.01);
    }
}

SDL_Texture* loadTexture(const std::string &file, SDL_Renderer *ren)
{
    SDL_Texture *texture = nullptr;

    int width, height, channels;
    uint8_t *image = stbi_load(file.c_str(), &width, &height, &channels, 4);
    log("%s %d %d", file.c_str(), width, height);
    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(image, width, height, 32, width * 4, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
    if (surface != nullptr)
    {
        texture = SDL_CreateTextureFromSurface(ren, surface);
        SDL_FreeSurface(surface);
        if (texture == nullptr)
        {
            fprintf(stderr, "Failed to convert image\n");
        }
    }
    else
    {
        fprintf(stderr, "failed to load image.\n");
    }
    free(image);
    return texture;
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
