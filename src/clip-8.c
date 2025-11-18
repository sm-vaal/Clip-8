#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "raylib.h"

/* #######  KEYBOARD LAYOUT  #######
 *
 *              1 2 3 C
 *              4 5 6 D
 *              7 8 9 E
 *              A 0 B F
 *
 */

// ##########  DEFINITIONS  ##########
#define byte uint8_t

#define PROGRAM_START 0x200
#define RAM_SIZE 4096
#define MEMORY_END 0xFFF
#define STACK_SIZE 0x10

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_SIZE_MULTIPLIER 12

#define TARGET_FPS 60
#define INSTRUCTIONS_PER_FRAME 12

// #define DEBUG_SCREEN
// #define DEBUG_INSTRUCTIONS
// #define DEBUG


// ##########  DATA STRUCTURES FOR INSTRUCTIONS  ###########
typedef enum {
    ROM_END = -2,
    ILLEGAL = -1,
    SYS     = 0,    // 0nnn - Jump to a machine code routine at nnn.
    CLS     = 1,    // 00E0 - Clear the display.
    RET     = 2,    // 00EE - Return from a subroutine. [pc = 0(sp), sp--]
    JPADR   = 3,    // 1nnn - Jump to location nnn. [pc = nnn]
    CALL    = 4,    // 2nnn - Call subroutine at nnn. [sp++, 0(sp) = pc]
    SEIMM   = 5,    // 3xkk - Skip next instruction if Vx = kk. [pc+=2]
    SNE     = 6,    // 4xkk - Skip next instruction if Vx != kk. [pc+=2]
    SEREG   = 7,    // 5xy0 - Skip next instruction if Vx = Vy. [pc+=2]
    LDIMM   = 8,    // 6xkk - Put the value kk into register Vx.
    ADDIMM  = 9,    // 7xkk - Adds kk to register Vx, and stores the result in Vx.
    LDI     = 10,   // 8xy0 - Stores the value of Vy in Vx.
    OR      = 11,   // 8xy1 - Set Vx = Vx OR Vy. (bitwise)
    AND     = 12,   // 8xy2 - Set Vx = Vx AND Vy. (bitwise)
    XOR     = 13,   // 8xy3 - Set Vx = Vx XOR Vy. (bitwise)
    ADDREG  = 14,   // 8xy4 - Set Vx = Vx + Vy, set VF = carry.
    SUB     = 15,   // 8xy5 - Set Vx = Vx - Vy, set VF = NOT borrow. (Vx > Vy => VF = 1, otherwise 0)
    SHR     = 16,   // 8xy6 - If bit 0 of Vx = 1, VF = 1, otherwise 0., then Vx >> 1
    SUBN    = 17,   // 8xy7 - If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx = Vy-Vx
    SHL     = 18,   // 8xyE - If the bit 15 of Vx is 1, then VF = 1, otherwise 0. Then Vx *= 2.
    SNEREG  = 19,   // 9xy0 - Skip next instruction if Vx != Vy.
    LD      = 20,   // Annn - Set I = nnn.
    JPREG   = 21,   // Bnnn - Jump to location nnn + V0 [PC = V0 + nnn]
    RND     = 22,   // Cxkk - Set Vx = random byte AND kk (use random and a modulo operator)
    DRW     = 23,   // Dxyn - Disp n-byte sprite starting at ptr I at (Vx, Vy), VF = collision (****)
    SKP     = 24,   // Ex9E - Skip next instruction if key with the value of Vx is pressed.
    SKNP    = 25,   // ExA1 - Skip next instruction if key with the value of Vx is not pressed.
    LDREGDT = 26,   // Fx07 - Set Vx = delay timer value.
    LDK     = 27,   // Fx0A - WAIT for a key press, store the value of the key in Vx.
    LDDTVX  = 28,   // Fx15 - Set Vx = delay timer value. [DT = Vx]
    LDSTVX  = 29,   // Fx18 - Set sound timer = Vx
    ADDIVX  = 30,   // Fx1E - Set I = I + Vx.
    LDFVX   = 31,   // Fx29 - Set I = location of sprite for digit Vx.
    LDBVX   = 32,   // Fx33 - Store BCD representation of Vx in memory locations I, I+1,I+2. (****)
    LDIVX   = 33,   // Fx55 - Store registers V0 through Vx in memory starting at location I
    LDVXI   = 34,   // Fx65 - Read registers V0 through Vx from memory starting at location I.
} opcode_t;


// ##########  GLOBAL VARIABLES  ##########
uint8_t  ram[RAM_SIZE];           // CPU address space, loaded at startup
uint8_t  frameBuffer[SCREEN_WIDTH][SCREEN_HEIGHT] = {0};
uint16_t stack[STACK_SIZE] = {0};
uint8_t  v[0x10] = {0};           // general purpose registers V0-VE, VF = flag register
uint8_t  SP = 0, DT = 0, ST = 0;  // stack pointer, delay timer, sound timer
uint16_t I = 0, PC = 0x200;       // 16-bit program counter and I register to store memory addresses
uint16_t currentInstruction;


// ##########  FUNCTION PROTOTYPES  ##########

// loads the input rom file (with its parameter name) into memory from 0x200
int loadRom(char path[]);

// gets the instruction enum from its opcode, -1 if it can't be decoded (non-existent instruction)
// just a huge switch statement. Could be implemented with a table for better performance
opcode_t decodeInstruction(unsigned short codedInstruction);

// does whatever the instruction has to do
int runInstruction(opcode_t opcode, uint16_t instruction);

// runs a cycle, doing fetch, decode, execute, and pc add
void clockCycle();

// opens a window
void initializeDisplay();

// draws rectangles on screen according to the framebuffer value
void drawScreen();

// the core loop of the emulator. Contains both logic and drawing
void mainLoop();

// implements the instruction for drawing on the screen
void drawSpriteToFramebuffer(uint16_t instruction);

// adds the hex character bitmaps to the interpreter area of memory (0x000 - 0x200)
void addHex();

// returns the current key pressed in the hex format, following the keypad pattern
uint8_t obtainKey();


//  ···  just for debugging  ···
// prints the whole RAM contents into console, for debugging
void printRam();
// prints the whole RAM, but as decoded instructions
void printDecoded();
// prints the current CPU state 
void printCPU();
// displays a grid pattern on the screen
void testScreen();
// used for debugging calls and rets
void printStack();



// ######################################
// ########## FUNCTION BODIES ###########
// ######################################
int main(int argc, char *argv[]) {

    SetTraceLogLevel(LOG_NONE);

    // rom loading
    printf("Enter a rom name\n");
    char romFilepath[255];
    fscanf(stdin,"%s", romFilepath);

    if (loadRom(romFilepath)) {
        printf("###    ROM with that name not found!    ###\n"
               "Make sure the rom is in the same folder as the .exe\n"
               "and that you included the extension (.ch8) in the name\n\n"
               "press any key and enter to exit!");
        scanf("%s", romFilepath);
        return 1;
    }
    printf("ROM LOADED SUCCESSFULLY!!!\n");

    addHex();

    initializeDisplay();

    mainLoop();

    CloseWindow();
    CloseAudioDevice();
    return 0;
}

int loadRom(char path[]) {
    FILE* romFile = fopen(path, "rb"); // open the ROM file
    if (romFile == NULL) { // if the file isn't found, exit with an error
        perror("Error opening file");
        return -1;
    }

    fseek(romFile, 0L, SEEK_END); // search for EOF to determine size
    int size = ftell(romFile);
    rewind(romFile); // go to start of file
    size_t result = fread(&ram[PROGRAM_START], 1, size, romFile); // copies ROM file to memory

    if (result != size) { // if the size returned by fread() and the manual check don't match, there is an issue
        perror("Error reading file");
        fclose(romFile);
        return -1;
    }

    fclose(romFile); // close the ROM file
    return 0;
}

opcode_t decodeInstruction(unsigned short codedInstruction) {
    uint16_t hi   = (codedInstruction & 0xF000) >> 12;
    uint8_t  lo8  =  codedInstruction & 0x00FF;
    uint8_t  lo4  =  codedInstruction & 0x000F;

    if (codedInstruction == 0x0000) return ROM_END;

    switch (hi) {
        case 0x0: {
            if      (codedInstruction == 0x00E0) return CLS;   // 00E0
            else if (codedInstruction == 0x00EE) return RET;   // 00EE
            else                                 return SYS;   // 0NNN
        }

        case 0x1:  return JPADR;  // 1nnn
        case 0x2:  return CALL;   // 2nnn
        case 0x3:  return SEIMM;  // 3xkk
        case 0x4:  return SNE;    // 4xkk

        case 0x5: return lo4 == 0 ? SEREG : ILLEGAL; // 5xy0

        case 0x6: return LDIMM; //6xkk
        case 0x7: return ADDIMM; //7xkk

        case 0x8: {
            switch (lo4) {
                case 0:    return LDI;      // 8xy0
                case 1:    return OR;       // 8xy1
                case 2:    return AND;      // 8xy2
                case 3:    return XOR;      // 8xy3
                case 4:    return ADDREG;   // 8xy4
                case 5:    return SUB;      // 8xy5
                case 6:    return SHR;      // 8xy6
                case 7:    return SUBN;     // 8xy7
                case 0xE:  return SHL;      // 8xyE
                default:   return ILLEGAL;
            }
        }

        case 0x9:  return lo4 == 0 ? SNEREG : ILLEGAL;

        case 0xA:  return LD;       // Annn
        case 0xB:  return JPREG;    // Bnnn
        case 0xC:  return RND;      // Cxkk
        case 0xD:  return DRW;      // Dxyn

        case 0xE: {
            if      (lo8 == 0x9E) return SKP;       // Ex9E
            else if (lo8 == 0xA1) return SKNP;      // ExA1
            else                  return ILLEGAL;
        }

        //THIS ONE TOO
        case 0xF: {
            unsigned short byte2 = (codedInstruction & 0x00FF);
            switch (byte2) {
                case 0x07:  return LDREGDT;     // Fx07
                case 0x0A:  return LDK;         // Fx0A
                case 0x15:  return LDDTVX;      // Fx15
                case 0x18:  return LDSTVX;      // Fx18
                case 0x1E:  return ADDIVX;      // Fx1E
                case 0x29:  return LDFVX;       // Fx29
                case 0x33:  return LDBVX;       // Fx33
                case 0x55:  return LDIVX;       // Fx55
                case 0x65:  return LDVXI;       // Fx65
                default:    return ILLEGAL;
            }
        }

        default:
            return ILLEGAL;
    }
}

int runInstruction(opcode_t opcode, uint16_t instruction) {
    switch (opcode) {
        case ROM_END: printf("ROM END READ\n");         return -1;
        case ILLEGAL: printf("Illegal instruction\n");  return -1;
        case SYS:     printf("Syscall (illegal)\n");    return -1;

        case CLS: {  for (int i = 0; i < SCREEN_WIDTH; i++) {
                     for (int j = 0; j < SCREEN_HEIGHT; j++) {
                     frameBuffer[i][j] = 0; } }          break;}

        case RET:    PC = stack[SP]; SP--;               break;

        case JPADR:  PC = (instruction & 0x0FFF) - 2;    break;

        case CALL:   SP++; stack[SP] = PC; PC = (instruction & 0x0FFF) - 2;  break;

        case SEIMM:  if (v[(instruction & 0x0F00) >> 8] ==
                       (instruction & 0x00FF)) PC += 2;  break;

        case SNE:    if (v[(instruction & 0x0F00) >> 8] !=
                       (instruction & 0x00FF)) PC += 2;       break;

        case SEREG:  if (v[(instruction & 0x0F00) >> 8] ==
                     v[(instruction & 0x00F0) >> 4]) PC += 2; break;

        case LDIMM:  v[(instruction & 0x0F00) >> 8] = instruction & 0x00FF;  break;

        case ADDIMM: v[(instruction & 0x0F00) >> 8] += instruction & 0x00FF; break;

        case LDI:    v[(instruction & 0x0F00) >> 8] = v[(instruction & 0x00F0) >> 4];  break;

        case OR:     v[(instruction & 0x0F00) >> 8] |= v[(instruction & 0x00F0) >> 4]; break;

        case AND:    v[(instruction & 0x0F00) >> 8] &= v[(instruction & 0x00F0) >> 4]; break;

        case XOR:    v[(instruction & 0x0F00) >> 8] ^= v[(instruction & 0x00F0) >> 4]; break;

        case ADDREG: uint32_t res1 = v[(instruction & 0x0F00) >> 8] + v[(instruction & 0x00F0) >> 4];
                     v[(instruction & 0x0F00) >> 8] = res1 & 0x00FF;
                     v[0xF] = (res1 & 0xFF00) != 0 ? 1 : 0;    break;

        case SUB:    uint32_t res2 = v[(instruction & 0x0F00) >> 8] - v[(instruction & 0x00F0) >> 4];
                     v[(instruction & 0x0F00) >> 8] = res2 & 0x00FF;
                     v[0xF] = (res2 & 0xFF00) == 0 ? 1 : 0;    break;

        case SHR:    byte f1 = v[(instruction & 0x0F00) >> 8] & 0x01;
                     v[(instruction & 0x0F00) >> 8] >>= 1; v[0xF] = f1;  break;

        case SUBN:   v[(instruction & 0x0F00) >> 8] = v[(instruction & 0x00F0) >> 4]
                     - v[(instruction & 0x0F00) >> 8];
                     v[0xF] = v[(instruction & 0x00F0) >> 4] > v[(instruction & 0x0F00) >> 8]; break;

        case SHL:    byte f2 = (v[(instruction & 0x0F00) >> 8] & 0x80) == 0 ? 0 : 1;
                     v[(instruction & 0x0F00) >> 8] <<= 1; v[0xF] = f2;  break;

        case SNEREG: PC += v[(instruction & 0x0F00) >> 8] !=
                     v[(instruction & 0x00F0) >> 4] ? 2 : 0;   break;

        case LD:     I = instruction & 0x0FFF;                 break;

        case JPREG:  PC = (instruction & 0x0FFF) + v[0x0] - 2; break;

        case RND:    v[(instruction & 0x0F00) >> 8] = rand() % 0x100
                     & (instruction & 0x00FF);                 break;

        case DRW:    drawSpriteToFramebuffer(instruction);     break;

        case SKP:    if (obtainKey() == v[(instruction & 0x0F00) >> 8]) PC += 2;  break;

        case SKNP:   if (obtainKey() != v[(instruction & 0x0F00) >> 8]) PC += 2;  break;

        case LDREGDT:v[(instruction & 0x0F00) >> 8] = DT;      break;

        case LDK:    v[((instruction & 0x0F00) >> 8)] = obtainKey();              break;

        case LDDTVX: DT = v[(instruction & 0x0F00) >> 8];      break;

        case LDSTVX: ST = v[(instruction & 0x0F00) >> 8];      break;

        case ADDIVX: I += v[(instruction & 0x0F00) >> 8];      break;

        case LDFVX:  I = v[((instruction & 0x0F00) >> 8)] * 5 ;    break;

        case LDBVX:  uint8_t value = v[((instruction & 0x0F00) >> 8)];
                     ram[I]   = value / 100;
                     ram[I+1] = (value / 10) % 10;
                     ram[I+2] = value % 10;                 break;

        case LDIVX: {for (int i = 0; i <= (instruction & 0x0F00) >> 8; i++) {
                     ram[(I & 0xFFF) + i] = v[i]; }         break; }

        case LDVXI: {for (int i = 0; i <= (instruction & 0x0F00) >> 8; i++) {
                     v[i] = ram[(I & 0x0FFF) + i]; }        break; }

        default:      printf("OPCODE UNDEFINED\n");      return -1;
    }
    return 0;

}

void clockCycle() {
    uint16_t instruction = ram[PC] << 8 | ram[PC + 1];
    opcode_t opcode = decodeInstruction(instruction);
    runInstruction(opcode, instruction);
    PC += 2;
}

void initializeDisplay() {
    InitWindow(SCREEN_WIDTH * SCREEN_SIZE_MULTIPLIER, SCREEN_HEIGHT * SCREEN_SIZE_MULTIPLIER, "Chip8");
    SetTargetFPS(TARGET_FPS);
    ClearBackground(BLACK);
    InitAudioDevice();
}

void drawScreen() {
    // plot each pixel as a square. A texture is more efficient, but this is simple
    Color sqr;
    BeginDrawing();
    for (int i = 0; i < (SCREEN_WIDTH); i++) {
        for (int j = 0; j < (SCREEN_HEIGHT); j++) {

            if (frameBuffer[i][j] != 0) {
                sqr = WHITE;
            } else {
                sqr = BLACK;
            }
            DrawRectangle(i * SCREEN_SIZE_MULTIPLIER, j * SCREEN_SIZE_MULTIPLIER,
                    SCREEN_SIZE_MULTIPLIER, SCREEN_SIZE_MULTIPLIER, sqr);


        }
    }
    EndDrawing();
}

void mainLoop() {
    while (!WindowShouldClose()) {
        for (int i = 0; i < INSTRUCTIONS_PER_FRAME; i++) {
            clockCycle();
        }

        #ifdef DEBUG_SCREEN
            testScreen();
        #endif

        #ifndef DEBUG_SCREEN
            drawScreen();

        #endif
        DT -= DT == 0 ? 0 : 1;
        ST -= ST == 0 ? 0 : 1;

    }
}

void drawSpriteToFramebuffer(uint16_t instruction) {
    uint8_t height = instruction & 0x000F;
    byte sprite[height];

    for (int i = 0; i < height; i++) {
        sprite[i] = ram[I + i]; //copy sprite from ram
    }

    v[0xF] = 0; //init collision reg
    uint8_t x = v[((instruction & 0x0F00) >> 8)];
    uint8_t y = v[((instruction & 0x00F0) >> 4)];

    for (int i = 0; i < height; i++) { // i = row, j = bit
        for (int j = 0; j < 8; j++) { // 8 bits in a byte

            if (x + j >= SCREEN_WIDTH  ||  y + i >= SCREEN_HEIGHT) continue;

            if (((sprite[i] >> (7-j) & 0x0001) && frameBuffer[x+j][y+i])) {
                v[0xF] = 1;
            }

            frameBuffer[x+j][y+i] ^= (sprite[i] >> (7-j)) & 0x0001;

        }
    }
}

void addHex() {
    //character '0'
    ram[0] = 0b11110000;
    ram[1] = 0b10010000;
    ram[2] = 0b10010000;
    ram[3] = 0b10010000;
    ram[4] = 0b11110000;

    //character '1'
    ram[5] = 0b00100000;
    ram[6] = 0b01100000;
    ram[7] = 0b00100000;
    ram[8] = 0b00100000;
    ram[9] = 0b01110000;

    //character '2'
    ram[10] = 0b11110000;
    ram[11] = 0b00010000;
    ram[12] = 0b11110000;
    ram[13] = 0b10000000;
    ram[14] = 0b11110000;

    //character '3'
    ram[15] = 0b11110000;
    ram[16] = 0b00010000;
    ram[17] = 0b11110000;
    ram[18] = 0b00010000;
    ram[19] = 0b11110000;

    //character '4'
    ram[20] = 0b10010000;
    ram[21] = 0b10010000;
    ram[22] = 0b11110000;
    ram[23] = 0b00010000;
    ram[24] = 0b00010000;

    //character '5'
    ram[25] = 0b11110000;
    ram[26] = 0b10000000;
    ram[27] = 0b11110000;
    ram[28] = 0b00010000;
    ram[29] = 0b11110000;

    //character '6'
    ram[30] = 0b11110000;
    ram[31] = 0b10000000;
    ram[32] = 0b11110000;
    ram[33] = 0b10010000;
    ram[34] = 0b11110000;

    //character '7'
    ram[35] = 0b11110000;
    ram[36] = 0b00010000;
    ram[37] = 0b00100000;
    ram[38] = 0b01000000;
    ram[39] = 0b01000000;

    //character '8'
    ram[40] = 0b11110000;
    ram[41] = 0b10010000;
    ram[42] = 0b11110000;
    ram[43] = 0b10010000;
    ram[44] = 0b11110000;

    //character '9'
    ram[45] = 0b11110000;
    ram[46] = 0b10010000;
    ram[47] = 0b11110000;
    ram[48] = 0b00010000;
    ram[49] = 0b00010000;

    //character 'A'
    ram[50] = 0b11110000;
    ram[51] = 0b10010000;
    ram[52] = 0b11110000;
    ram[53] = 0b10010000;
    ram[54] = 0b10010000;

    //character 'B'
    ram[55] = 0b11100000;
    ram[56] = 0b10010000;
    ram[57] = 0b11100000;
    ram[58] = 0b10010000;
    ram[59] = 0b11100000;

    //character 'C'
    ram[60] = 0b11110000;
    ram[61] = 0b10000000;
    ram[62] = 0b11110000;
    ram[63] = 0b10000000;
    ram[64] = 0b11110000;

    //character 'D'
    ram[65] = 0b11100000;
    ram[66] = 0b10010000;
    ram[67] = 0b10010000;
    ram[68] = 0b10010000;
    ram[69] = 0b11100000;

    //character 'E'
    ram[70] = 0b11110000;
    ram[71] = 0b10000000;
    ram[72] = 0b11110000;
    ram[73] = 0b10000000;
    ram[74] = 0b11110000;

    //character 'F'
    ram[75] = 0b11110000;
    ram[76] = 0b10000000;
    ram[77] = 0b11110000;
    ram[78] = 0b10000000;
    ram[79] = 0b10000000;
}

byte obtainKey() {
    if (IsKeyDown(KEY_ONE))   return 0x1;
    if (IsKeyDown(KEY_TWO))   return 0x2;
    if (IsKeyDown(KEY_THREE)) return 0x3;
    if (IsKeyDown(KEY_FOUR))  return 0xC;

    if (IsKeyDown(KEY_Q))     return 0x4;
    if (IsKeyDown(KEY_W))     return 0x5;
    if (IsKeyDown(KEY_E))     return 0x6;
    if (IsKeyDown(KEY_R))     return 0xD;

    if (IsKeyDown(KEY_A))     return 0x7;
    if (IsKeyDown(KEY_S))     return 0x8;
    if (IsKeyDown(KEY_D))     return 0x9;
    if (IsKeyDown(KEY_F))     return 0xE;

    if (IsKeyDown(KEY_Z))     return 0xA;
    if (IsKeyDown(KEY_X))     return 0x0;
    if (IsKeyDown(KEY_C))     return 0xB;
    if (IsKeyDown(KEY_V))     return 0xF;

    return 0xFF; // No key pressed
}


//######## DEBUGGING FUNCTIONS #######
void printRam() {
    for (int i = 0; i < RAM_SIZE; i++) {
        printf("%c", ram[i]);
    }
}

void printDecoded() {
    for (int i = 0; i < RAM_SIZE; i += 2) {
        uint16_t instruction = ram[i] << 8 | ram[i + 1];
        if (i == PROGRAM_START) {
            printf("PROM_START: ");
        }
        printf("%d  ", decodeInstruction(instruction));
    }
}

void printCPU() {
    for (int i = 0; i < 0x10; i++) {
        printf("Register v[%d] = 0x%x\n", i, v[i]);
    }
    printf("PC = 0x%x\n", PC);
    printf("I = 0x%x\n", I);
    printf("SP = 0x%x\n", SP);
    printf("DT = 0x%x\n", DT);
    printf("ST = 0x%x\n", ST);
}

void testScreen() {
    Color sqr;
    BeginDrawing();
    for (int i = 0; i < (SCREEN_WIDTH); i++) {
        for (int j = 0; j < (SCREEN_HEIGHT); j++) {
            sqr = BLACK;
            if ((i + j) % 2) {
                sqr = WHITE;
            }
            DrawRectangle(i * SCREEN_SIZE_MULTIPLIER, j * SCREEN_SIZE_MULTIPLIER,
                    SCREEN_WIDTH, SCREEN_HEIGHT, sqr);
        }
    }
    EndDrawing();
}

void printStack() {
    for (int i = 0; i < STACK_SIZE / 2; i++) {
        printf("stack[%d] : %x \n", i, stack[i]);
    }
}