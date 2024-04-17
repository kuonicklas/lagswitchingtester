#ifndef SHARED_H
#define SHARED_H

#include <SDL2/SDL.h>

//----DEFS----
#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600
#define MAX_KEYBOARD_KEYS 350 //350 possible keyboard inputs
#define PLAYER_SPEED 5
#define PLAYER_SIZE 64 //Width of square. Origin at top left (0,0).

enum class packetType{INITIALIZE};

//----SHARED STRUCTS----
typedef struct{
    int id; //Player id
    int x;
    int y;
    SDL_Color color;
} Player;

#endif