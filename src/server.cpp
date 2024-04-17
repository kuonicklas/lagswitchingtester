#include <iostream>
#include <enet/enet.h>
#include <string>
#include <cstdlib>
#include "shared.h"

void cleanup();
void printPlayerCount();
void initializePlayer();
int random_range(int, int);

ENetHost* server;
ENetEvent event;

int main(int argc, char* argv[]){
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

    server = enet_host_create(&address, 32, 1, 0, 0);

    if (server == NULL){
        std::cout << "Failed to create an ENet server." << std::endl;
        exit(1);
    }
    
    std::cout << "Server created at port " << address.port << ". Ready to connect." << std::endl;
    printPlayerCount();

    //Game loop start
    std::srand(time(nullptr)); //Seed the RNG

    while(true){
        while(enet_host_service(server, &event, 1000) > 0){
            switch(event.type){
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "A client connected from " << event.peer->address.host << ":" << event.peer->address.port << "." << std::endl;
                    initializePlayer();
                    printPlayerCount();
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << event.peer->address.host << ":" << event.peer->address.port << " disconnected." << std::endl;
                    printPlayerCount();
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    std::cout << "A packet of length " << event.packet->dataLength << " containing " << event.packet->data << " was received from " << event.peer->address.host << ":" << event.peer->address.port << " on channel" << event.channelID << "." << std::endl;
                    break;
            }
        }
    }

    //Game loop end
    enet_host_destroy(server);

    exit(0);
}

void cleanup(){
    enet_deinitialize();
}

void printPlayerCount(){
    std::cout << "There are [" << server->connectedPeers << "] players connected." << std::endl;
}

void initializePlayer(){
    static int id; //Unique id assigned to players

    //Construct packet, semicolon separated
    std::string packetData;
    packetData += std::to_string(static_cast<int>(packetType::INITIALIZE));
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
    ++id;

    ENetPacket* packet = enet_packet_create(packetData.c_str(), packetData.length() + 1, ENET_PACKET_FLAG_RELIABLE);
    
    enet_peer_send(event.peer, 0, packet);
}

int random_range(int min, int max){
    return min + (std::rand() % (max - min + 1));
}