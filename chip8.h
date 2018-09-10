#ifndef CHIP8_CHIP8_H
#define CHIP8_CHIP8_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

typedef struct chip8 chip8;
struct chip8 {
    u8* memory;
    u8* V;
    u16 I;
    u16 pc;
    u16 sp;
    u16* stack;
    u8* screen;

    int sound_timer;
    int delay_timer;

    bool* key_pressed;
    bool draw_flag;
};

int chip8_init(chip8* const m);
void chip8_quit(chip8* const m);
int chip8_load_file_in_memory(chip8* const m, const char* filename);
void chip8_step(chip8* const m);

#endif  // CHIP8_CHIP8_H
