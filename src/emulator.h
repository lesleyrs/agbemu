#ifndef EMULATOR_H
#define EMULATOR_H

// #include <SDL2/SDL.h>

#include "gba.h"
#include "types.h"

typedef struct {
    bool running;
    char* romfile;
    char* romfilenodir;
    char* biosfile;
    bool uncap;
    bool bootbios;
    bool filter;
    bool pause;
    bool mute;
    bool debugger;

    GBA* gba;
    Cartridge* cart;
    byte* bios;

} EmulatorState;

extern EmulatorState agbemu;

int emulator_init(int argc, char** argv);
void emulator_quit();

void read_args(int argc, char** argv);
void hotkey_press(int key, int code);
void update_input_keyboard(GBA* gba, uint8_t *keys);
// void update_input_controller(GBA* gba, SDL_GameController* controller);
void init_color_lookups();
void gba_convert_screen(hword* gba_screen, uint32_t* screen);

#endif
