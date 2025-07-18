#include "emulator.h"

// #include <SDL2/SDL.h>
// #include <zlib.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "arm_isa.h"
#include "gba.h"
#include "thumb_isa.h"

EmulatorState agbemu;

const char usage[] = "agbemu [options] <romfile>\n"
                     "-b <biosfile> -- specify bios file path\n"
                     "-f -- apply color filter\n"
                     "-u -- run at uncapped speed\n"
                     "-d -- run the debugger\n";

int emulator_init(int argc, char** argv) {
    read_args(argc, argv);
    if (!agbemu.romfile) {
        printf(usage);
#ifndef __wasm
        return -1;
#endif
    }
    if (!agbemu.biosfile) {
        agbemu.biosfile = "bios.bin";
    }

    agbemu.gba = malloc(sizeof *agbemu.gba);
    agbemu.cart = create_cartridge(agbemu.romfile);
    if (!agbemu.cart) {
        agbemu.cart = create_cartridge_from_picker(&agbemu.romfile);
        if (!agbemu.cart) {
            free(agbemu.gba);
            printf("Invalid rom file\n");
            return -1;
        }
    }

    agbemu.bios = load_bios(agbemu.biosfile);
    if (!agbemu.bios) {
        free(agbemu.gba);
        destroy_cartridge(agbemu.cart);
        printf("Invalid or missing bios file.\n");
        return -1;
    }

    arm_generate_lookup();
    thumb_generate_lookup();
    init_color_lookups();
    init_gba(agbemu.gba, agbemu.cart, agbemu.bios, agbemu.bootbios);

    agbemu.romfilenodir = strrchr(agbemu.romfile, '/');
    if (agbemu.romfilenodir) agbemu.romfilenodir++;
    else agbemu.romfilenodir = agbemu.romfile;
    return 0;
}

void emulator_quit() {
    destroy_cartridge(agbemu.cart);
    free(agbemu.bios);
    free(agbemu.gba);
}

void read_args(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (char* f = &argv[i][1]; *f; f++) {
                switch (*f) {
                    case 'u':
                        agbemu.uncap = true;
                        break;
                    case 'b':
                        agbemu.bootbios = true;
                        if (!*(f + 1) && i + 1 < argc) {
                            agbemu.biosfile = argv[i + 1];
                        }
                        break;
                    case 'f':
                        agbemu.filter = true;
                        break;
                    case 'd':
                        agbemu.debugger = true;
                        break;
                    default:
                        printf("Invalid flag\n");
                }
            }
        } else {
            agbemu.romfile = argv[i];
        }
    }
}

void save_state() {
    gba_clear_ptrs(agbemu.gba);

    // gzFile fp = gzopen(agbemu.cart->sst_filename, "wb");
    // gzfwrite(agbemu.gba, sizeof *agbemu.gba, 1, fp);
    // gzfwrite(&agbemu.cart->st, sizeof agbemu.cart->st, 1, fp);
    // gzclose(fp);

    gba_set_ptrs(agbemu.gba, agbemu.cart, agbemu.bios);
}

void load_state() {
    gba_clear_ptrs(agbemu.gba);

    // gzFile fp = gzopen(agbemu.cart->sst_filename, "rb");
    // gzfread(agbemu.gba, sizeof *agbemu.gba, 1, fp);
    // gzfread(&agbemu.cart->st, sizeof agbemu.cart->st, 1, fp);
    // gzclose(fp);

    gba_set_ptrs(agbemu.gba, agbemu.cart, agbemu.bios);
}

#include <js/dom_pk_codes.h>
void hotkey_press(int key, int code) {
    switch (key) {
        case 'p':
            agbemu.pause = !agbemu.pause;
            break;
        case 'm':
            agbemu.mute = !agbemu.mute;
            break;
        case 'f':
            agbemu.filter = !agbemu.filter;
            break;
        case 'r':
            init_gba(agbemu.gba, agbemu.cart, agbemu.bios, agbemu.bootbios);
            agbemu.pause = false;
            break;
        case '9':
            save_state();
            break;
        case '0':
            load_state();
            break;
        default:
            break;
    }
    switch (code) {
        case DOM_PK_TAB:
            agbemu.uncap = !agbemu.uncap;
            break;
    }
}

void update_input_keyboard(GBA* gba, uint8_t *keys) {
    gba->io.keyinput.a = ~keys[DOM_PK_Z];
    gba->io.keyinput.b = ~keys[DOM_PK_X];
    gba->io.keyinput.start = ~keys[DOM_PK_ENTER];
    gba->io.keyinput.select = ~keys[DOM_PK_SHIFT_RIGHT];
    gba->io.keyinput.left = ~keys[DOM_PK_ARROW_LEFT];
    gba->io.keyinput.right = ~keys[DOM_PK_ARROW_RIGHT];
    gba->io.keyinput.up = ~keys[DOM_PK_ARROW_UP];
    gba->io.keyinput.down = ~keys[DOM_PK_ARROW_DOWN];
    gba->io.keyinput.l = ~keys[DOM_PK_A];
    gba->io.keyinput.r = ~keys[DOM_PK_S];
}

// void update_input_controller(GBA* gba, SDL_GameController* controller) {
//     gba->io.keyinput.a &=
//         ~SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);
//     gba->io.keyinput.b &=
//         ~SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X);
//     gba->io.keyinput.start &=
//         ~SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START);
//     gba->io.keyinput.select &=
//         ~SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK);
//     gba->io.keyinput.left &= ~SDL_GameControllerGetButton(
//         controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
//     gba->io.keyinput.right &= ~SDL_GameControllerGetButton(
//         controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
//     gba->io.keyinput.up &=
//         ~SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
//     gba->io.keyinput.down &= ~SDL_GameControllerGetButton(
//         controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
//     gba->io.keyinput.l &= ~SDL_GameControllerGetButton(
//         controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
//     gba->io.keyinput.r &= ~SDL_GameControllerGetButton(
//         controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
// }

byte color_lookup[32];
byte color_lookup_filter[32];

void init_color_lookups() {
    for (int i = 0; i < 32; i++) {
        float c = (float) i / 31;
        color_lookup[i] = c * 255;
        color_lookup_filter[i] = pow(c, 1.7) * 255;
    }
}

void gba_convert_screen(hword* gba_screen, uint32_t* screen) {
    for (int i = 0; i < GBA_SCREEN_W * GBA_SCREEN_H; i++) {
        int r = gba_screen[i] & 0x1f;
        int g = (gba_screen[i] >> 5) & 0x1f;
        int b = (gba_screen[i] >> 10) & 0x1f;
        if (agbemu.filter) {
            screen[i] = color_lookup_filter[r] << 16 |
                        color_lookup_filter[g] << 8 |
                        color_lookup_filter[b] << 0;
        } else {
            screen[i] = color_lookup[r] << 16 | color_lookup[g] << 8 |
                        color_lookup[b] << 0;
        }
    }
}
