#include <iostream>
#include <enet/enet.h>
#include <string>
#include <cstdlib>
#include "shared.h"
#include <vector>
#include <SDL2/SDL.h>
#include <map>

void cleanup();
void printPlayerCount();
void initializePlayer();
int random_range(int, int);
void processPacket(ENetPacket*);
void parseUpdatePacket(std::string&);
unsigned int sendUpdatePackets(unsigned int, void*);
unsigned int analyzePackets(unsigned int, void*);

//Packet switching detection
std::map<int, int> packet_counter; //Accumulator for each player
std::map<int,std::vector<int>> packetCount; //Vector of samples for each player
std::map<int,std::vector<int>> criticalEvents; //Vector of samples for each player
int getPValue();

ENetHost* server;
ENetEvent event;
std::vector<Player> playerList;

int main(int argc, char* argv[]){
    if (SDL_Init(SDL_INIT_TIMER) < 0){
        std::cout << "Failed to initialize SDL Timer: " << SDL_GetError() << std::endl;
        exit(1);
    }

    //Initialize
    if (enet_initialize() != 0){
        std::cout << "Failed to initialize ENet." << std::endl;
        exit(1);
    }
    atexit(cleanup);

    //Create server
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 4450;

    server = enet_host_create(&address, MAX_PLAYERS, 1, 0, 0);

    if (server == NULL){
        std::cout << "Failed to create an ENet server." << std::endl;
        exit(1);
    }
    
    std::cout << "Server created at port " << address.port << ". Ready to connect." << std::endl;
    printPlayerCount();

    //Game loop start
    std::srand(time(nullptr)); //Seed the RNG
    SDL_TimerID updateTimer = SDL_AddTimer(16, sendUpdatePackets, NULL); //Call update automatically
    SDL_TimerID analysisTimer = SDL_AddTimer(1000, analyzePackets, NULL); //Sample rate (also analysis)

    while(true){
        //Receive packet(s)
        while(enet_host_service(server, &event, 0) > 0){
            switch(event.type){
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "A client connected from " << event.peer->address.host << ":" << event.peer->address.port << "." << std::endl;
                    initializePlayer();
                    printPlayerCount();
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << event.peer->address.host << ":" << event.peer->address.port << " disconnected." << std::endl;
                    //Ideally, handle disconnects. Currently, the playerList just grows. So we track which peer has which id. Every update received, the client checks the list of ids. If one is not present, remove from their playerList.
                    printPlayerCount();
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    processPacket(event.packet);
                    break;
            }
        }

        //Send packets
        //while() 
    }
}

void cleanup(){
    if (server != NULL)
        enet_host_destroy(server);
    enet_deinitialize();
}

void printPlayerCount(){
    std::cout << "There are [" << server->connectedPeers << "] players connected." << std::endl;
}

void initializePlayer(){
    static int id; //Unique id assigned to players
    int x, y, r, g, b;

    //Construct packet, semicolon separated
    std::string packetData;
    packetData += std::to_string(static_cast<int>(serverPacket::INITIALIZE));
    packetData += std::to_string(id); //id
    packetData += ";";
    packetData += std::to_string(random_range(0,WINDOW_WIDTH - PLAYER_SIZE)); //x
    packetData += ";";
    packetData += std::to_string(random_range(0,WINDOW_HEIGHT - PLAYER_SIZE)); //y
    packetData += ";";
    packetData += std::to_string(random_range(0,255)); //r value
    packetData += ";";
    packetData += std::to_string(random_range(0,255)); //g value
    packetData += ";";
    packetData += std::to_string(random_range(0,255)); //b value

    ENetPacket* packet = enet_packet_create(packetData.c_str(), packetData.length() + 1, ENET_PACKET_FLAG_RELIABLE);
    
    enet_peer_send(event.peer, 0, packet);

    //Add to player list
    Player newPlayer = {id, x, y};
    newPlayer.color.r = r;
    newPlayer.color.g = g;
    newPlayer.color.b = b;

    playerList.push_back(newPlayer);
    std::cout << "Initialized player [" << id << "]" << std::endl;

    ++id;
}

int random_range(int min, int max){
    return min + (std::rand() % (max - min + 1));
}

void processPacket(ENetPacket* packet){
    std::string data(reinterpret_cast<char const*>(packet->data));

    int type = data[0] - '0'; //Get packet category
    switch (type){
        case 0:
            //UPDATE
            parseUpdatePacket(data);
    }
}

void parseUpdatePacket(std::string& data){
    std::string values[3]; //id, x, y
    grabStrings(data, values);
    
    //Update player position
    int id = stoi(values[0]);

    for (Player& p : playerList){
        if (p.id != id) //Find vector element corresponding to player id (I should change vector to map instead)
            continue;
        
        p.x = stoi(values[1]);
        p.y = stoi(values[2]);

        packet_counter[p.id]++; //Increment packet count
        //std::cout << "Player " << id << ": [" << p.x << "," << p.y << "]" << std::endl;
    }
}

unsigned int sendUpdatePackets(unsigned int a, void* b){
    if (server->connectedPeers == 0)
        return 1;

    std::string packetData;

    packetData += std::to_string(static_cast<int>(serverPacket::UPDATE));

    for (Player& p : playerList){
        packetData += std::to_string(p.id);
        packetData += ";";
        packetData += std::to_string(p.x);
        packetData += ";";
        packetData += std::to_string(p.y);
        packetData += ";";
    }
    packetData.pop_back(); //Remove last semicolon

    //Send player positions to each player
    ENetPacket* packet = enet_packet_create(packetData.c_str(), packetData.length() + 1, 0);

    for (int i=0; i < playerList.size(); i++){
        enet_peer_send(&(server->peers[i]), 0, packet);
    }

    return 16;
}

unsigned int analyzePackets(unsigned int a, void* b){
    static int threshold_counter = 0;
    threshold_counter++;

    //Get Sample
    for (Player& p : playerList){
        packetCount[p.id].push_back(packet_counter[p.id]);
        std::cout << "Sample for Player [" << p.id << "]: " << packet_counter[p.id] << std::endl;
        packet_counter[p.id] = 0;
    }

    if (threshold_counter == THRESHOLD){
        //Perform analysis

        //Clear data
        for (Player& p : playerList){
            packetCount[p.id].clear();
        }
    }

    return 1000;
}