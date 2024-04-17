#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string.h>
#include <iostream>
#include <enet/enet.h>
#include "shared.h"

//----STRUCTS----
typedef struct{
    SDL_Renderer* renderer;
    SDL_Window* window;
    int input[MAX_KEYBOARD_KEYS]; //Stores the state of all keyboard inputs
} App;

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
void processPacket(ENetPacket*);
void parseInitPacket(std::string&);
void grabStrings(std::string&, std::string[]);

//----Global Vars----
ENetPeer* peer; //The server-client connection
bool initialized = false;

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
    
    client = enet_host_create(NULL, 1, 1, 0, 0);
    if (client == NULL){
        std::cout << "Failed to create an ENet client." << std::endl;
        exit(1);
    }

    enet_address_set_host(&address, "127.0.0.1");
    address.port = 4450;

    std::cout << "Connecting to server..." << std::endl;

    peer = enet_host_connect(client, &address, 1, 0);
    if (peer == NULL){
        std::cout << "Failed to connect with the server." << std::endl;
    }

    if (enet_host_service(client, &event, 5000) > 0
        && event.type == ENET_EVENT_TYPE_CONNECT){
            std::cout << "Connected to " << event.peer->address.host << ":" << event.peer->address.port << "." << std::endl;
        }
    else{
        enet_peer_reset(peer);
        std::cout << "Connection failed." << std::endl;
        system("pause");
        exit(0);
    }

    //GAME LOOP
    player = new Player;
    
    while(true){
        //Receive packet
        while(enet_host_service(client, &event, 0) > 0){ //Was 1000
            switch(event.type){
                case ENET_EVENT_TYPE_RECEIVE:
                    std::cout << "Received packet: [" << event.packet->data << "]" << std::endl;
                    processPacket(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "Server at " << event.peer->address.host << ":" << event.peer->address.port << " closed." << std::endl;
                    break;
            }
        }

        getInput(); //User Input
        doGameLogic(); //Player Movement
        doDrawing(); //Drawing
        
        //Cap Frame Rate
        SDL_Delay(16);
    }
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
    //int prev_x(player->x), prev_y(player->y);

    if (app.input[SDL_SCANCODE_W])
        player->y -= PLAYER_SPEED;
    if (app.input[SDL_SCANCODE_S])
        player->y += PLAYER_SPEED;
    if (app.input[SDL_SCANCODE_A])
        player->x -= PLAYER_SPEED;
    if (app.input[SDL_SCANCODE_D])
        player->x += PLAYER_SPEED;

    //Constrain within window
    player->x = std::max(0, player->x);
    player->y = std::max(0, player->y);
    player->x = std::min(WINDOW_WIDTH - PLAYER_SIZE, player->x);
    player->y = std::min(WINDOW_HEIGHT - PLAYER_SIZE, player->y);
    
    //if (prev_x != player->x || prev_y != player->y)
        //std::cout << "Player position: (" << player->x << "," << player->y << ")" << std::endl;
}

void doDrawing(){
    //Background
    SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, 255); //White
    SDL_RenderClear(app.renderer);

    if (initialized){
        SDL_SetRenderDrawColor(app.renderer, player->color.r, player->color.g, player->color.b, 255);
        SDL_Rect rect = {player->x, player->y, PLAYER_SIZE, PLAYER_SIZE};
        SDL_RenderFillRect(app.renderer, &rect);
    }
    
    SDL_RenderPresent(app.renderer); //Render
}

void cleanup(){
    if (peer != NULL)
        enet_peer_disconnect_now(peer, 0); //Forcefully disconnect
    enet_deinitialize();
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
}

void processPacket(ENetPacket* packet){
    std::string data(reinterpret_cast<char const*>(packet->data));

    int type = data[0] - '0'; //Get packet category
    switch (type){
        case 0:
            //INITIALIZE
            parseInitPacket(data);
    }
}

void parseInitPacket(std::string& data){
    std::string values[6]; //id, x, y, r, g, b

    grabStrings(data, values);
    
    //Assign values
    player->id = stoi(values[0]);
    player->x = stoi(values[1]);
    player->y = stoi(values[2]);
    player->color.r = stoi(values[3]);
    player->color.g = stoi(values[4]);
    player->color.b = stoi(values[5]);

    initialized = true;
    std::cout << "Initialized: " << player->id << "-" << player->x << "-" << player->color.r << std::endl;
}

void grabStrings(std::string& str, std::string data[]){
    //Store semicolon-separated strings in an array
    int j = 0; //Element of array

    for (int i=1; i < str.length(); i++){
        if (str[i] == ';'){
            ++j;
            continue;
        }

        data[j] += str[i];
    }
}