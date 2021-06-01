#include <fstream>
#include <stdlib.h>

// Debugging
#include <iostream>
using namespace std;

class chip8 {
    public:

    // Fontset
    unsigned char chip8_fontset[80] = { 
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

    //---------
    //---CPU---
    //---------

    // Opcode responsible for different functions
    unsigned short opcode;

    // Emulator memory
    unsigned char memory[4096]; // Physical memory
    unsigned char V[16]; // registers (16th being a "Carry flag")

    unsigned short I; // Index register
    unsigned short pc; // Program counter

    // Video & Audio
    unsigned char gfx[64 * 32];
    int screenSize = 64 * 32;
    bool drawFlag;
    unsigned char delay_timer;
    unsigned char sound_timer;

    // Stack for jumps
    unsigned short stack[16];
    unsigned short sp; // stack pointer

    // HEX-based keypad to store state of key
    unsigned char key[16];

    //---------------
    //---FUNCTIONS---
    //---------------

    void initialize() {
        // Initialize registers and memory once
        pc = 0x200;
        opcode = 0;
        I = 0;
        sp = 0;
        srand(time(NULL));

        // Load fontset
        for (int i = 0; i < 80; ++i) {
            memory[i] = chip8_fontset[i];
        }

        // Clear display, stack, registers, memory
        for (int i = 0; i < 2048; ++i) {
            gfx[i] = 0;
        }

        for (int i = 0; i < 16; ++i) {
            stack[i] = 0;
            key[i] = 0;
            V[i] = 0;
        }

        for (int i = 80; i < 4096; ++i) {
            memory[i] = 0;
        }

        // Reset timers
        delay_timer = 0;
        sound_timer = 0;
    }

    bool loadGame(char* fileName) {
        FILE* file = fopen(fileName, "rb");

        if (file == NULL) {
            cout << "Invalid file!" << endl;
            return false;
        }

        // Get file size
        fseek(file, 0, SEEK_END);
        long lSize = ftell(file);
        rewind(file);

        // Allocate memory to contain file
        char* buffer = (char*)malloc(sizeof(char) * lSize);

        // Read file
        fread(buffer, 1, lSize, file);

        for (int i = 0; i < lSize; ++i) {
            memory[i + 512] = buffer[i];
        }

        fclose(file);
        free(buffer);

        return true;
    }

    void emulateCycle() {
        // Fetch Opcode
        opcode = memory[pc] << 8 | memory[pc + 1];

        // All needed variables (due to limitations of switch cases)
        int x = (opcode & 0x0F00) >> 8;
        int y = (opcode & 0x00F0) >> 4;
        int N = (opcode & 0x000F);
        int NN = (opcode & 0x00FF);
        int NNN = (opcode & 0x0FFF);
        unsigned char dummy = 0;
        int i = 0;
        bool pressed = false;

        // Decode Opcode
        switch (opcode & 0xF000) {
            case 0x0000:
                switch (opcode & 0x00FF) {
                    case 0x00E0:
                        // TODO: Clear the screen (TEST)
                        for (i = 0; i < screenSize; ++i) {
                            gfx[i] = 0;
                        }

                        drawFlag = true;

                        pc += 2;
                        break;
                    case 0x00EE:
                        // Return from subroutine
                        sp--;
                        pc = stack[sp] + 2;
                        stack[sp] = 0;
                        break;
                    default:
                        printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
                        pc += 2;
                        break;
                }
                break;
            case 0x1000:
                // Code is 0x1NNN --> Jump to address NNN
                pc = NNN;
                break;
            case 0x2000:
                // Code is 0x2NNN --> Calls a subroutine at address NNN
                stack[sp] = pc;
                ++sp;
                pc = NNN;
                break;
            case 0x3000:
                // 3XNN --> Skip next instruction if VX = NN
                if (V[x] == NN) {
                    pc += 2;
                }
                pc += 2;
                break;
            case 0x4000:
                // 4XNN --> Skip next instruction if VX != NN
                if (V[x] != NN) {
                    pc += 2;
                }
                pc += 2;
                break;
            case 0x5000:
                // 5XY0 --> Skip if V[x] == V[y]
                switch (N) {
                    case 0x0000:
                        if (V[x] == V[y]) {
                            pc += 2;
                        }
                        pc += 2;
                        break;
                    default:
                        printf("Unknown opcode [0x5000]: 0x%X\n", opcode);
                        break;
                }
                pc += 2;
                break;
            case 0x6000:
                // 6XNN --> V[x] = NN
                V[x] = NN;
                pc += 2;
                break;
            case 0x7000:
                // 7XNN --> V[x] += NN
                V[x] += NN;
                pc += 2;
                break;
            case 0x8000:
                switch (N) {
                    case 0x0000:
                        // 8XY0 --> V[x] = V[y]
                        V[x] = V[y];
                        break;
                    case 0x0001:
                        // 8XY1 --> V[x] = V[x] | V[y]
                        V[x] |= V[y];
                        break;
                    case 0x0002:
                        // 8XY2 --> V[x] = V[x] & V[y]
                        V[x] &= V[y];
                        break;
                    case 0x0003:
                        // 8XY3 --> V[x] = V[x] ^ V[y]
                        V[x] ^= V[y];
                        break;
                    case 0x0004:
                        // 8XY4 --> V[x] = V[x] + V[y]

                        // Check for carry
                        if ((V[x] & V[y]) > 0) {
                            V[15] = 1;
                        } else {
                            V[15] = 0;
                        }

                        V[x] += V[y];
                        break;
                    case 0x0005:
                        // 8XY5 --> V[x] = V[x] - V[y]

                        // Check for borrow
                        V[15] = ((((V[x] ^ V[y]) | V[y]) ^ V[x]) > 0) ? 0 : 1;

                        V[x] -= V[y];
                        break;
                    case 0x0006:
                        // Store the smallest bit of V[x] in V[0xF] then shift V[x] right 1
                        V[15] = V[x] & 1;

                        V[x] = V[x] >> 1;
                        break;
                    case 0x0007:
                        // 8XY5 --> V[y] = V[y] - V[x]
                        // Check for borrow
                        V[15] = ((((V[y] ^ V[x]) | V[x]) ^ V[y]) > 0) ? 0 : 1;

                        V[y] -= V[x];
                        break;
                    case 0x000E:
                        // Store the largest bit of V[x] in V[0xF] then shift V[x] left 1
                        V[15] = (V[x] >> 7);

                        V[x] = V[x] << 1;
                        break;
                    default:
                        printf("Unknown opcode [0x8000]: 0x%X\n", opcode);
                        break;
                }
                pc += 2;
                break;
            case 0x9000:
                switch (N) {
                    case 0x0000:
                        // 9XY0 --> Skip next if V[x] != V[y]
                        if (V[x] != V[y]) {
                            pc += 2;
                        }
                        break;
                    default: 
                        printf("Unknown opcode [0x9000]: 0x%X\n", opcode);
                        break;
                }
                pc += 2;
                break;
            case 0xA000:
                // ANNN --> Set I to address NNN
                I = NNN;
                pc += 2;
                break;
            case 0xB000:
                // BNNN --> Set PC to NNN + V[0]
                pc = NNN + V[0];
                break;
            case 0xC000:
                // CXNN --> V[x] = rand() & NN
                V[x] = (rand() % 256) & NN;
                pc += 2;
                break;
            case 0xD000:
                // DXYN --> Draw a sprite at (Vx, Vy) with width 8, height N + 1.
                // Each row is read as bit-coded starting from location I. Val of I does not change.
                // V[15] = 1 iff any screen pixels are flipped from set to unset when sprite is drawn.
                {
                unsigned short xPos = V[x];
                unsigned short yPos = V[y];
                unsigned short height = opcode & 0x000F;
                unsigned short pixel;
                V[0xF] = 0;
                for (int offY = 0; offY < height; ++offY) {
                    pixel = memory[I + offY];
                    for (int offX = 0; offX < 8; ++offX) {
                        if ((pixel & (0x80 >> offX)) != 0) {
                            int addr = xPos + offX + ((yPos + offY) * 64);
                            if (gfx[addr] == 1) {
                                V[0xF] = 1;
                            }
                            gfx[addr] ^= 1;
                        }
                    }
                }

                drawFlag = true;
                pc += 2;
                }
                break;
            case 0xE000:
                switch (opcode & 0x00FF) {
                    case 0x009E:
                        // EX9E --> Skip next instruction if V[x] pressed
                        if (key[V[x]] != 0) {
                            pc += 2;
                        }
                        pc += 2;
                        break;
                    case 0x00A1:
                        // EXA1 --> Skip next instruction if V[x] not pressed
                        if (key[V[x]] == 0) {
                            pc += 2;
                        }
                        pc += 2;
                        break;
                    default:
                        printf("Unknown opcode [0xE000]: 0x%X\n", opcode);
                        pc += 2;
                        break;
                }
                break;
            case 0xF000:
                switch (opcode & 0x00FF) {
                    case 0x0007:
                        // FX07 --> Sets V[x] to value of delay timer
                        V[x] = delay_timer;
                        pc += 2;
                        break;
                    case 0x000A:
                        // FX0A --> Wait for a key press, then store it in V[x]
                        for (i = 0; i < 16; ++i) {
                            if (key[i] != 0) {
                                V[x] = i;
                                pressed = true;
                            }
                        }

                        if (!pressed) {
                            return;
                        }

                        pc += 2;
                        break;
                    case 0x0015:
                        // FX15 --> Set delay to VX
                        delay_timer = V[x];
                        pc += 2;
                        break;
                    case 0x0018:
                        // FX18 --> Set sound timer to VX
                        sound_timer = V[x];
                        pc += 2;
                        break;
                    case 0x001E:
                        // FX1E --> Adds I += V[x]. V[15] unaffected (for most games)
                        I += V[x];
                        pc += 2;
                        break;
                    case 0x0029:
                        // FX29 --> Sets I to location of sprite for character in VX
                        I = V[x] * 5;
                        pc += 2;
                        break;
                    case 0x0033:
                        // FX33 --> Get decimal representation and store at I, I + 1, I + 2
                        dummy = V[x];
                        for (i = 2; i >= 0; --i) {
                            memory[I + i] = dummy % 10;
                            dummy /= 10;
                        }
                        pc += 2;
                        break;
                    case 0x0055:
                        // FX55 --> Stores V0 to VX (inclusively) in memory starting at I. Leave I unmodified
                        for (i = 0; i <= x; ++i) {
                            memory[I + i] = V[i];
                        }
                        pc += 2;
                        break;
                    case 0x0065:
                        // FX65 --> Stores memory from I into V0 to VX. Leave I unmodified
                        for (i = 0; i <= x; ++i) {
                            V[i] = memory[I + i];
                        }
                        pc += 2;
                        break;
                    default:
                        printf("Unknown opcode [0xF000]: 0x%X\n", opcode);
                        pc += 2;
                        break;
                }
                break;
            default:
                printf("Unknown opcode: 0x%X\n", opcode);
                pc += 2;
                break;
        }

        // Execute Opcode
        if (delay_timer > 0) {
            --delay_timer;
        }

        if (sound_timer > 0) {
            if (sound_timer == 1) {
                // Play audio
            }
            --sound_timer;
        }

        // Update timers
    }
};