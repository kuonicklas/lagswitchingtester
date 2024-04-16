#include <iostream>
#include <enet/enet.h>

void cleanup();

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

    ENetHost* server;
    server = enet_host_create(&address, 32, 1, 0, 0);

    if (server == NULL){
        std::cout << "Failed to create an ENet server." << std::endl;
        exit(1);
    }

    char myip[32];
    
    enet_address_get_host_ip(&address, myip, strlen(myip));
    std::cout << "Server created at " << std::to_string(myip) << ":" << address.port << ". Ready to connect." << std::endl;

    //Game loop start
    ENetEvent event;

    while(true){
        while(enet_host_service(server, &event, 1000) > 0){
            switch(event.type){
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "A client connected from " << event.peer->address.host << ":" << event.peer->address.port << "." << std::endl;
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    std::cout << "A packet of length " << event.packet->dataLength << " containing " << event.packet->data << " was received from " << event.peer->address.host << ":" << event.peer->address.port << " on channel" << event.channelID << "." << std::endl;
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << event.peer->address.host << ":" << event.peer->address.port << " disconnected." << std::endl;
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