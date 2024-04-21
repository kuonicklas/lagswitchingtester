#include <SDL2/SDL.h>
#include <string.h>
#include <iostream>
#include <enet/enet.h>
#include "shared.h"
#include <vector>
#include <array>
#include <cmath>
#include <map>

//----STRUCTS----
typedef struct{
    int id; //Player id
    int x;
    int y;
    SDL_Color color;
} Player;

typedef struct{
    SDL_Renderer* renderer;
    SDL_Window* window;
    int input[MAX_KEYBOARD_KEYS]; //Stores the state of all keyboard inputs
} App;

App app; //Game window
std::map<int, Player> playerList; //Playerdata of all players

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
void parseUpdatePacket(std::string&);
void updateServer();
int getPacketData(ENetPacket*, std::string&);
void DrawCircle(SDL_Renderer*, int32_t, int32_t, int32_t); //NOT MY CODE; THIS IS A WINDOWS "IMPORT"

unsigned int sendSample(unsigned int, void*);

//----Global Vars----
ENetPeer* peer; //The server-client connection
bool initialized = false;
bool dropPackets = false;
bool drawCircle = false;
bool badConnection = false;
Player* self = nullptr;

int critical_counter; //Accumulator for critical events

//
int main(int argc, char* argv[]){
    //Initialize

    if (enet_initialize() != 0){
        std::cout << "Failed to initialize ENet." << std::endl;
        exit(1);
    }

    atexit(cleanup); //Call this automatically when program closes

    //CONNECT TO SERVER
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

    peer = enet_host_connect(client, &address, 1, 0);
    std::cout << "Connecting to server..." << std::endl;
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

    //Initialization loop
    std::cout << "Awaiting player initialization from server..." << std::endl;
    while (!initialized){
        while(enet_host_service(client, &event, 0) > 0){
            if (event.type == ENET_EVENT_TYPE_RECEIVE){
                //Wait for initialization packet specifically
                std::string data;
                int type = getPacketData(event.packet, data);
                if (type != 0)
                    continue;
                parseInitPacket(data);
            }
        }
    }

    //Init SDL, open game window
    init_SDL(); 

    SDL_TimerID sampleTimer = SDL_AddTimer(1000, sendSample, NULL); //Send critical moment sample

    //GAME LOOP
    while(true){
        //Receive packet(s)
        while(enet_host_service(client, &event, 0) > 0){
            switch(event.type){
                case ENET_EVENT_TYPE_RECEIVE:
                    processPacket(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "Server at " << event.peer->address.host << ":" << event.peer->address.port << " closed." << std::endl;
                    break;
            }
        }

        getInput(); //User Input
        doGameLogic(); //Player Movement
        
        if (!dropPackets){
            int chance = 0;
            if (badConnection)
                chance = random_range(0,3);
            if (chance == 0)
                updateServer();
        }
            
        doDrawing(); //Drawing
        SDL_RenderPresent(app.renderer); //Render
        
        //Cap Frame Rate
        SDL_Delay(16);
    }
}

void init_SDL(){
    //Initialize SDL compoments
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cout << "Failed to initialize SDL Video: " << SDL_GetError() << std::endl;
        exit(1);
    }

    if (SDL_Init(SDL_INIT_TIMER) != 0){
        std::cout << "Failed to initialize SDL Timer: " << SDL_GetError() << std::endl;
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
    if (app.input[SDL_SCANCODE_W])
        self->y -= PLAYER_SPEED;
    if (app.input[SDL_SCANCODE_S])
        self->y += PLAYER_SPEED;
    if (app.input[SDL_SCANCODE_A])
        self->x -= PLAYER_SPEED;
    if (app.input[SDL_SCANCODE_D])
        self->x += PLAYER_SPEED;

    //Toggle dropPackets
    if (app.input[SDL_SCANCODE_SPACE]){
        if (!dropPackets){
            std::cout << "[PACKET SWITCHING ON]" << std::endl;
            dropPackets = true;
        }
    }
    else{
        if (dropPackets){
            std::cout << "[PACKET SWITCHING OFF]" << std::endl;
            dropPackets = false;
        }
    }

    //Toggle drawCircle
    if (app.input[SDL_SCANCODE_P]){
        if (!drawCircle){
            drawCircle = true;
        }
    }
    else{
        if (drawCircle)
            drawCircle = false;
    }
    
    //Toggle badConnection
    if (app.input[SDL_SCANCODE_L]){
        if (!badConnection){
            badConnection = true;
        }
    }
    else{
        if (badConnection)
            badConnection = false;
    }

    //Constrain within window
    self->x = std::max(0, self->x);
    self->y = std::max(0, self->y);
    self->x = std::min(WINDOW_WIDTH - PLAYER_SIZE, self->x);
    self->y = std::min(WINDOW_HEIGHT - PLAYER_SIZE, self->y);

    //Check critical events
    double distance;
    for (auto& p : playerList){
        if (p.second.id == self->id)
            continue;

        //Get distance
        double dx = self->x + (PLAYER_SIZE / 2) - p.second.x + (PLAYER_SIZE / 2);
        double dy = self->y + (PLAYER_SIZE / 2) - p.second.y + (PLAYER_SIZE / 2);
        distance = std::hypot(dx, dy);
        if (distance < CRITICAL_ZONE_RADIUS){
            critical_counter++;
            break; //We only need at least one other player within critical distance
        }
    }
}

void doDrawing(){
    //Background
    SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, 255); //White
    SDL_RenderClear(app.renderer);

    if (!initialized)
        return;

    //Draw critical zone
    if (drawCircle){
        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
        DrawCircle(app.renderer, self->x + (PLAYER_SIZE / 2), self->y + (PLAYER_SIZE / 2), CRITICAL_ZONE_RADIUS);
    }
    
    //Draw all players
    for (auto& p : playerList){
        SDL_SetRenderDrawColor(app.renderer, p.second.color.r, p.second.color.g, p.second.color.b, 255); //We don't actually receive the colors of other players!!
        SDL_Rect rect = {p.second.x, p.second.y, PLAYER_SIZE, PLAYER_SIZE};
        SDL_RenderFillRect(app.renderer, &rect);
    }
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
    std::string data;
    int type = getPacketData(packet, data);

    switch (type){
        case 0:
            //INITIALIZE
            parseInitPacket(data);
            break;
        case 1:
            //UPDATE
            if (!dropPackets)
                parseUpdatePacket(data);
            break;
    }
}

void parseInitPacket(std::string& packetData){
    std::string initData[6]; //id, x, y, r, g, b

    grabStrings(packetData, initData);
    
    //Init self
    //Also, initialize everybody else---------
    Player newPlayer;
    newPlayer.id = stoi(initData[0]);
    newPlayer.x = stoi(initData[1]);
    newPlayer.y = stoi(initData[2]);
    newPlayer.color.r = stoi(initData[3]);
    newPlayer.color.g = stoi(initData[4]);
    newPlayer.color.b = stoi(initData[5]);

    playerList[newPlayer.id] = newPlayer;
    self = &playerList[newPlayer.id]; //"Self" points to its position in playerList

    initialized = true;
    std::cout << "Initialized with ID[" << self->id << "] Pos[" << self->x << "," << self->y << "] Color[" << static_cast<int>(self->color.r) << "," << static_cast<int>(self->color.g) << "," << static_cast<int>(self->color.b) << "]" << std::endl; 
}

void updateServer(){
    std::string packetData;

    packetData += std::to_string(static_cast<int>(clientPacket::UPDATE)); //Packet category
    packetData += std::to_string(self->id); //id
    packetData += ';';
    packetData += std::to_string(self->x); //x
    packetData += ';';
    packetData += std::to_string(self->y); //y

    ENetPacket* packet = enet_packet_create(packetData.c_str(), packetData.length() + 1, 0);
    enet_peer_send(peer, 0, packet);
}

void parseUpdatePacket(std::string& data){
    //Parse string
    std::vector<std::array<std::string, 3>> playerData;

    for (int i=1; i < data.length(); i++){
        std::array<std::string, 3> pData;

        for (int j=0; j < 3 && i < data.length(); i++){
            if (data[i] == ';'){
                ++j;
                continue;
            }

            pData[j] += data[i];
        }

        playerData.push_back(pData);
        --i;
    }

    //Update player data
    int index;
    for (std::array<std::string, 3>& p : playerData){
        int id = stoi(p[0]);

        if (id == self->id)
            continue; //Skip own entry
        
        //Unknown ID -> Add new player
        auto iter = playerList.find(self->id);
        if (iter == playerList.end()){
            Player newPlayer = {};
            newPlayer.id = id;
            playerList[id] = newPlayer;
            std::cout << "Added player entry: [" << id << "]." << std::endl;
        }

        //Update position
        playerList[id].x = stoi(p[1]);
        playerList[id].y = stoi(p[2]);
    }
}

unsigned int sendSample(unsigned int a, void* b){
    std::cout << "Counter: " << critical_counter << std::endl;
    critical_counter = 0;
    return 1000;
}

int getPacketData(ENetPacket* packet, std::string& container){
    container = reinterpret_cast<char const*>(packet->data);
    return container[0] - '0'; //Returns packet type
}

void DrawCircle(SDL_Renderer * renderer, int32_t centreX, int32_t centreY, int32_t radius)
{
   const int32_t diameter = (radius * 2);

   int32_t x = (radius - 1);
   int32_t y = 0;
   int32_t tx = 1;
   int32_t ty = 1;
   int32_t error = (tx - diameter);

   while (x >= y)
   {
      //  Each of the following renders an octant of the circle
      SDL_RenderDrawPoint(renderer, centreX + x, centreY - y);
      SDL_RenderDrawPoint(renderer, centreX + x, centreY + y);
      SDL_RenderDrawPoint(renderer, centreX - x, centreY - y);
      SDL_RenderDrawPoint(renderer, centreX - x, centreY + y);
      SDL_RenderDrawPoint(renderer, centreX + y, centreY - x);
      SDL_RenderDrawPoint(renderer, centreX + y, centreY + x);
      SDL_RenderDrawPoint(renderer, centreX - y, centreY - x);
      SDL_RenderDrawPoint(renderer, centreX - y, centreY + x);

      if (error <= 0)
      {
         ++y;
         error += ty;
         ty += 2;
      }

      if (error > 0)
      {
         --x;
         tx += 2;
         error += (tx - diameter);
      }
   }
}