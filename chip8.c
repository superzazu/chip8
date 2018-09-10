#include "chip8.h"

static const u8 chip8_fontset[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

int chip8_init(chip8* const m) {
    m->memory = calloc(0x1000, sizeof(u8));
    m->V = calloc(16, sizeof(u8));
    m->I = 0;
    m->pc = 0x200;
    m->sp = 0;
    m->stack = calloc(16, sizeof(u8));
    m->screen = calloc(0x800, sizeof(u8));

    m->sound_timer = 0;
    m->delay_timer = 0;

    m->key_pressed = calloc(16, sizeof(u8));
    m->draw_flag = false;

    memcpy(&m->memory[0x50], chip8_fontset, sizeof(chip8_fontset));

    return 0;
}

void chip8_quit(chip8* const m) {
    free(m->memory);
    free(m->V);
    free(m->stack);
    free(m->screen);
    free(m->key_pressed);
}

int chip8_load_file_in_memory(chip8* const m, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        printf("error: can't open ROM file.\n");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    unsigned long file_size = ftell(f);
    rewind(f);

    if (file_size > 0x1000 - 0x200) {
        printf("error: ROM file too big to fit in memory\n");
        return 1;
    }

    if (fread(&m->memory[0x200], 1, file_size, f) != file_size) {
        printf("error: while reading ROM file\n");
        return 1;
    }

    fclose(f);

    return 0;
}

void chip8_step(chip8* const m) {
    const u16 opcode = (m->memory[m->pc] << 8) | m->memory[m->pc + 1];
    m->pc += 2;

    const u8 x = (opcode & 0x0F00) >> 8;
    const u8 y = (opcode & 0x00F0) >> 4;
    const u8 n = opcode & 0x000F;
    const u8 nn = opcode & 0x00FF;
    const u16 nnn = opcode & 0x0FFF;

    switch (opcode & 0xF000)
    {
    case 0x0000:
        switch (n) {
        case 0x0000:
            // 00E0	Clears the screen.
            memset(m->screen, 0, 0x800);
            m->draw_flag = true;
        break;

        case 0x000E:
            // 00EE	Returns from a subroutine.
            m->sp -= 1;
            m->pc = m->stack[m->sp];
        break;
        }
    break;

    case 0x1000:
        // 1NNN	Jumps to address NNN.
        m->pc = nnn;
    break;

    case 0x2000:
        // 2NNN	Calls subroutine at NNN.
        m->stack[m->sp] = m->pc;
        m->sp += 1;
        m->pc = nnn;
    break;

    case 0x3000:
        // 3XNN	Skips the next instruction if VX equals NN.
        if (m->V[x] == nn) {
            m->pc += 2;
        }
    break;

    case 0x4000:
        // 4XNN	Skips the next instruction if VX doesn't equal NN.
        if (m->V[x] != nn) {
            m->pc += 2;
        }
    break;

    case 0x5000:
        // 5XY0	Skips the next instruction if VX equals VY.
        if (m->V[x] == m->V[y]) {
            m->pc += 2;
        }
    break;

    case 0x6000:
        // 6XNN	Sets VX to NN.
        m->V[x] = nn;
    break;

    case 0x7000:
        // 7XNN	Adds NN to VX.
        m->V[x] += nn;
    break;

    case 0x8000: {
        switch (n) {
        case 0x0000:
            // 8XY0	Sets VX to the value of VY.
            m->V[x] = m->V[y];
        break;

        case 0x0001:
            // 8XY1	Sets VX to VX or VY.
            m->V[x] |= m->V[y];
        break;

        case 0x0002:
            // 8XY2	Sets VX to VX and VY.
            m->V[x] &= m->V[y];
        break;

        case 0x0003:
            // 8XY3	Sets VX to VX xor VY.
            m->V[x] ^= m->V[y];
        break;

        case 0x0004:
            // 8XY4	Adds VY to VX. VF is set to 1 when there's
            // a carry, and to 0 when there isn't.
            m->V[0xF] = m->V[x] + m->V[y] > 0xFF;
            m->V[x] += m->V[y];
        break;

        case 0x0005:
            // 8XY5	VY is subtracted from VX. VF is set to 0 when
            // there's a borrow, and 1 when there isn't.
            m->V[0xF] = m->V[x] >= m->V[y];
            m->V[x] -= m->V[y];
        break;

        case 0x0006:
            // 8XY6	Shifts VX right by one. VF is set to the value
            // of the least significant bit of VX before the shift.
            // On the original interpreter, the value of VY is
            // shifted, and the result is stored into VX. On current
            // implementations, Y is ignored
            m->V[0xF] = m->V[x] & 1;
            m->V[x] >>= 1;
        break;

        case 0x0007:
            // 8XY7	Sets VX to VY minus VX. VF is set to 0 when
            // there's a borrow, and 1 when there isn't.
            m->V[0xF] = m->V[x] <= m->V[y];
            m->V[x] = m->V[y] - m->V[x];
        break;

        case 0x000E:
            // 8XYE	Shifts VX left by one. VF is set to the value
            // of the most significant bit of VX before the shift.
            // On the original interpreter, the value of VY is
            // shifted, and the result is stored into VX. On current
            // implementations, Y is ignored
            m->V[0xF] = m->V[x] >> 7;
            m->V[x] <<= 1;
        break;
        }
    } break;

    case 0x9000:
        // 9XY0	Skips the next instruction if VX doesn't equal VY.
        if (m->V[x] != m->V[y]) {
            m->pc += 2;
        }
    break;

    case 0xA000:
        // ANNN	Sets I to the address NNN.
        m->I = nnn;
    break;

    case 0xB000:
        // BNNN	Jumps to the address NNN plus V0.
        m->pc = nnn + m->V[0x0];
    break;

    case 0xC000:
        // CXNN	Sets VX to a random number, masked by NN.
        m->V[x] = rand() & nn;
    break;

    case 0xD000: {
        // DXYN	Sprites stored in memory at location in index register,
        // maximum 8bits wide. Wraps around the screen. If when drawn,
        // clears a pixel, register VF is set to 1 otherwise it is zero.
        // All drawing is XOR drawing (i.e. it toggles the screen pixels)

        m->V[0xF] = 0;
        for (int yline = 0; yline < n; yline++) {
            u8 sprite = m->memory[m->I + yline];
            for (int xline = 0; xline < 8; xline++) {
                int px = (m->V[x] + xline) & 63;
                int py = (m->V[y] + yline) & 31;
                int pos = 64 * py + px;

                int pixel = (sprite >> (7 - xline)) & 0x1;

                m->V[0xF] |= (m->screen[pos] & pixel);
                m->screen[pos] ^= pixel;
            }
        }
        m->draw_flag = true;
    } break;

    case 0xE000:
        switch (n) {
        case 0x000E:
            // EX9E	Skips the next instruction if the key stored in VX
            // is pressed.
            if (m->key_pressed[m->V[x]]) {
                m->pc += 2;
            }
        break;

        case 0x001:
            // EXA1	Skips the next instruction if the key stored in VX
            // isn't pressed.
            if (!m->key_pressed[m->V[x]]) {
                m->pc += 2;
            }
        break;
        }
    break;

    case 0xF000:
        switch (nn) {
        case 0x0007:
            // FX07	Sets VX to the value of the delay timer.
            m->V[x] = m->delay_timer;
        break;

        case 0x000A: {
            // FX0A	A key press is awaited, and then stored in VX.
            bool key_pressed = false;
            for (int i = 0; i < 16; i++) {
                if (m->key_pressed[i] == true) {
                    m->V[x] = i;
                    key_pressed = true;
                    break;
                }
            }

            // continue executing this opcode until a key is pressed:
            if (!key_pressed) {
                m->pc -= 2;
                return;
            }
        } break;

        case 0x0015:
            // FX15	Sets the delay timer to VX.
            m->delay_timer = m->V[x];
        break;

        case 0x0018:
            // FX18	Sets the sound timer to VX.
            m->sound_timer = m->V[x];
        break;

        case 0x001E:
            // FX1E	Adds VX to I.
            //  (VF is set to 1 when range overflow (I+VX>0xFFF),
            //  and 0 when there isn't. This is an undocumented feature
            //  of the CHIP-8 and used by Spacefight 2091! game.)
            m->V[0xF] = m->I + m->V[x] > 0xFFF;
            m->I += m->V[x];
        break;

        case 0x0029:
            // FX29	Sets I to the location of the sprite for the
            // character in VX. Characters 0-F (in hexadecimal)
            // are represented by a 4x5 font.
            m->I = 0x50 + (m->V[x] & 0xF) * 5;
        break;

        case 0x0033:
            // FX33	Stores the Binary-coded decimal representation
            // of VX, with the most significant of three digits
            // at the address in I, the middle digit at I plus 1,
            // and the least significant digit at I plus 2.
            // (In other words, take the decimal representation of VX,
            // place the hundreds digit in memory at location in I,
            // the tens digit at location I+1, and the ones digit
            // at location I+2.)
            m->memory[m->I + 2] = m->V[x] % 10;
            m->memory[m->I + 1] = (m->V[x] / 10) % 10;
            m->memory[m->I] = (m->V[x] / 100);
        break;

        case 0x0055:
            // FX55	Stores V0 to VX in memory starting at address I
            for (int i = 0; i <= x; ++i) {
                m->memory[m->I + i] = m->V[i];
            }

            // I += x + 1;
        break;

        case 0x0065:
            // FX65	Fills V0 to VX with values from memory
            // starting at address I.
            for (int i = 0; i <= x; ++i) {
                m->V[i] = m->memory[m->I + i];
            }

            // I += x + 1;
        break;
        }
    break;

    default:
        printf("Unknown opcode: %d\n", opcode);
        break;
    }

    if (m->delay_timer > 0) {
        m->delay_timer--;
    }

    if (m->sound_timer > 0) {
        if (m->sound_timer == 1) {
            printf("beep"); // '\a'
        }
        m->sound_timer--;
    }
}
