#include "Messages.h"
#include "Ship.h"

#include <zmq.hpp>
#include <SDL2/SDL.h>
#include <memory>
#include <cstdarg>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Window *g_window = nullptr;
SDL_Renderer *g_renderer = nullptr;
zmq::context_t* g_context = nullptr;

void log(const char* pattern, ...);
SDL_Texture* loadTexture(const std::string &file, SDL_Renderer *ren);

//For now just connect to the server on localhost for testing
//Eventually need to allow a gui to select which server and set player name etc
int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_window                 = SDL_CreateWindow("Space!", 100, 100, 1000, 1000, SDL_WINDOW_SHOWN);
    SDL_Renderer *g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    if(0 != SDL_RenderSetLogicalSize(g_renderer, WorldSize, WorldSize))
    {
        log("couldn't set resolution\n");
    }

    ClientMessage updateMessage;
    updateMessage.type = PlayerUpdateMsg;

    std::shared_ptr<Ship> pShip  (new Ship("A Ship"));
    pShip->AddForce(Vector3f(0.0, 0.0, 10.0));
    pShip->GetClientUpdateMessage(updateMessage.update);

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
    ServerPlayerUpdateMsg server_update_message;
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
                switch (e.key.keysym.sym){
                    case SDLK_a:
                        pShip->AddForce(Vector3f(-10.0, 0.0, 0.0));
                        break;
                    case SDLK_d:
                        pShip->AddForce(Vector3f(10.0, 0.0, 0.0));
                        break;
                    case SDLK_w:
                        pShip->AddForce(Vector3f(0.0, 10.0, 0.0));
                        break;
                    case SDLK_s:
                        pShip->AddForce(Vector3f(0.0, -10.0, 0.0));
                        break;
                }
            }
            //TODO: implement keyboard callbacks
        }
        // update ship

        // send update
        ClientMessage update_message;
        update_message.type = PlayerUpdateMsg;
        pShip->GetClientUpdateMessage(update_message.update);
        zmq::message_t message(sizeof(update_message));
        memcpy(message.data(), &updateMessage, sizeof(update_message));
        command.send(message);

        zmq::message_t recvd_msg;
        command.recv(&recvd_msg);

        // get an update
        for(int ii=0; ii<2; ++ii)
        {
            zmq::message_t state_msg(2 * sizeof(ServerPlayerUpdateMsg));
            gamestate.recv(&state_msg);
            if(state_msg.size() == sizeof(ServerPlayerUpdateMsg))
            {
                memcpy(&server_update_message, state_msg.data(), state_msg.size());
                break;
            }
        }

        // draw
        SDL_RenderClear(g_renderer);
        for(int i=0; i<server_update_message.player_count; ++i)
        {
            SDL_Rect dst;
            dst.x = server_update_message.updates[i].position[0];
            dst.y = server_update_message.updates[i].position[1];
            dst.w = 55;
            dst.h = 70;
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
