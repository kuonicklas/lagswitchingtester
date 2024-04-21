#ifndef SHARED_H
#define SHARED_H

#include <SDL2/SDL.h>

//----DEFS----
#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600
#define MAX_KEYBOARD_KEYS 350 //350 possible keyboard inputs
#define PLAYER_SPEED 5
#define PLAYER_SIZE 64 //Width of square. Origin at top left (0,0).
#define MAX_PLAYERS 4
#define THRESHOLD 60 //Number of samples taken for analysis
#define CRITICAL_ZONE_RADIUS 100

enum class serverPacket{INITIALIZE, UPDATE, DISCONNECT};
enum class clientPacket{UPDATE};

//----SHARED STRUCTS----

//----SHARED FUNCS----
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

int random_range(int min, int max){
    return min + (std::rand() % (max - min + 1));
}

#endif