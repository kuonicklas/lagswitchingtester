#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string.h>
#include <iostream>
#include <enet/enet.h>
#include <cstdint>

//----DEFS----
#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600
#define MAX_KEYBOARD_KEYS 350 //350 possible keyboard inputs
#define PLAYER_SPEED 5
#define PLAYER_SIZE 64 //Width of square. Origin at top left (0,0).

//----STRUCTS----
typedef struct{
    SDL_Renderer* renderer;
    SDL_Window* window;
    int input[MAX_KEYBOARD_KEYS]; //Stores the state of all keyboard inputs
} App;

typedef struct{
    int x;
    int y;
    std::uint8_t id; //Player id
} Player;

App app; //Game window
Player* player;

//----FUNCTIONS----

void init_SDL();
void cleanup();
void getInput();
void registerPress(SDL_KeyboardEvent*);
void registerRelease(SDL_KeyboardEvent*);
void doGameLogic();
void doDrawing();

//

int main(int argc, char* argv[]){
    //Initialize
    init_SDL();

    if (enet_initialize() != 0){
        std::cout << "Failed to initialize ENet." << std::endl;
        exit(1);
    }

    atexit(cleanup); //Call this automatically when program closes

    //Create a client
    ENetHost* client;
    ENetAddress address; //Holds server IP and port
    ENetEvent event; //Holds events from server (i.e. data)
    ENetPeer* peer; //The server-client connection
    
    client = enet_host_create(NULL, 1, 1, 0, 0);
    if (client == NULL){
        std::cout << "Failed to create an ENet client." << std::endl;
        exit(1);
    }

    enet_address_set_host(&address, "127.0.0.1");
    address.port = 4450;

    peer = enet_host_connect(client, &address, 1, 0);
    if (peer == NULL){
        std::cout << "Failed to connect with the server." << std::endl;
    }

    if (enet_host_service(client, &event, 5000) > 0
        && event.type == ENET_EVENT_TYPE_CONNECT){
            std::cout << "Connection succeeded." << std::endl;
        }
    else{
        enet_peer_reset(peer);
        std::cout << "Connection failed." << std::endl;
        system("pause");
        exit(0);
    }

    //Game loop start
    player = new Player;
    player->x=0;//TEMP
    player->y=0;//TEMP
    
    while(true){
        //Receive packet
        while(enet_host_service(client, &event, 0) > 0){ //Was 1000
            switch(event.type){
                case ENET_EVENT_TYPE_RECEIVE:
                    std::cout << "A packet of length " << event.packet->dataLength << " containing " << event.packet->data << " was received from " << event.peer->address.host << ":" << event.peer->address.port << " on channel" << event.channelID << "." << std::endl;
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "Server at " << event.peer->address.host << ":" << event.peer->address.port << " closed." << std::endl;
                    enet_peer_disconnect(peer, 0); //DO I NEED THIS LINE?
                    break;
            }
        }

        getInput(); //User Input
        doGameLogic(); //Player Movement
        doDrawing(); //Drawing
        
        //Cap Frame Rate
        SDL_Delay(16);
    }
    
    //UNNECESSARY PROBS
    // while(enet_host_service(client, &event, 3000) > 0){
    //     switch(event.type){
    //         case ENET_EVENT_TYPE_RECEIVE:
    //             enet_packet_destroy(event.packet);
    //             break;
    //         case ENET_EVENT_TYPE_DISCONNECT:
    //             std::cout << "Disconnection succeeded." << std::endl;
    //             break;
    //     }
    // }
}

void init_SDL(){
    //Initialize SDL's renderer
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cout << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        exit(1);
    }

    //Create window
    app.window = SDL_CreateWindow("LagSwitchClient", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!app.window){
        std::cout << "Failed to open a window: " << SDL_GetError() << std::endl;
        exit(1);
    }

    //Create renderer
    app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED);
    if (!app.renderer){
        std::cout << "Failed to create a renderer: " << SDL_GetError() << std::endl;
        exit(1);
    }
}

void getInput(){
    SDL_Event event; //For storing input events

    //Clear input buffer
    while (SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_KEYDOWN:
                registerPress(&event.key);
                break;
            case SDL_KEYUP:
                registerRelease(&event.key);
                break;
            case SDL_QUIT:
                exit(0);
                break;
        }
    }
}

void registerPress(SDL_KeyboardEvent* event){
    if (!event->repeat && event->keysym.scancode < MAX_KEYBOARD_KEYS)
        app.input[event->keysym.scancode] = 1;
}

void registerRelease(SDL_KeyboardEvent* event){
    if (!event->repeat && event->keysym.scancode < MAX_KEYBOARD_KEYS)
        app.input[event->keysym.scancode] = 0;
}

void doGameLogic(){
    //Movement
    int prev_x(player->x), prev_y(player->y);

    if (app.input[SDL_SCANCODE_UP])
        player->y -= PLAYER_SPEED;
    if (app.input[SDL_SCANCODE_DOWN])
        player->y += PLAYER_SPEED;
    if (app.input[SDL_SCANCODE_LEFT])
        player->x -= PLAYER_SPEED;
    if (app.input[SDL_SCANCODE_RIGHT])
        player->x += PLAYER_SPEED;

    //Constrain within window
    player->x = std::max(0, player->x);
    player->y = std::max(0, player->y);
    player->x = std::min(WINDOW_WIDTH - PLAYER_SIZE, player->x);
    player->y = std::min(WINDOW_HEIGHT - PLAYER_SIZE, player->y);
    
    if (prev_x != player->x || prev_y != player->y)
        std::cout << "Player position: (" << player->x << "," << player->y << ")" << std::endl;
}

void doDrawing(){
    //Background
    SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, 255);
    SDL_RenderClear(app.renderer);

    SDL_SetRenderDrawColor(app.renderer, 255, 0, 0, 255);
    SDL_Rect rect = {player->x, player->y, PLAYER_SIZE, PLAYER_SIZE};
    SDL_RenderFillRect(app.renderer, &rect);
    
    SDL_RenderPresent(app.renderer); //Render
}

void cleanup(){
    enet_deinitialize();
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
}