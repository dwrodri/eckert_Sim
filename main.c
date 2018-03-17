#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum rom_flags {
    IP,
    LP,
    EP,
    LM,
    R,
    W,
    LD,
    ED,
    LI,
    EI,
    LA,
    EA,
    A,
    S,
    EU,
    LB,
    CD,
    MAP,
    HLT,
};

const char* opcode_names[] = {
        "fet",
        "lda",
        "sta",
        "add",
        "sub",
        "mba",
        "jmp",
        "jn",
        "hlt",
        "hlt",
        "hlt",
        "hlt",
        "hlt",
        "hlt",
        "hlt",
        "hlt"
};

static unsigned int   CTRL_ROM[32]; //this is the control ROM accessed to run through each µroutine
static unsigned char  ADDR_ROM[16]; //maps µroutines to CTRL_ROM
static unsigned short RAM[256];//"main memory" each short is a row of 12 bits
static unsigned char PC = 0; //Program Counter (only use 8 bits!)
static unsigned char MAR = 0; //Address register
static unsigned short MDR = 0; //Data register
static unsigned short RBX = 0; //Register B
static unsigned short ACC = 0; //Accumulator
static unsigned short IR  = 0; //Instruction register
static unsigned short UR  = 0; //µ-instr register
static unsigned short ALU = 0; //output location of the ALU
static unsigned short BUS = 0; //BUS for moving data around
static unsigned char HALTED = false;
static unsigned char NEG = false;


void dump_ram_table(){
    unsigned char* opcodes = malloc(sizeof(char) * 256); //truncate useless bits to fit my willing
    unsigned char* references = malloc(sizeof(char) * 256);
    for (short i = 0; i < 256; i++) {
        references[i] = (unsigned char)(RAM[i]);
        opcodes[i] = (unsigned char) (RAM[i] >> 8);

    }
    puts("=============================================");
    puts("ADDR\t|\tOPCODE\t|\tREF\t\t|\tRAW\t\t|");
    puts("=============================================");
    for (short j = 0; j < 256; ++j){
        printf("%03x:\t|\t%3s\t\t|\t %2x\t\t|\t%03x\t\t|\n", j, opcode_names[opcodes[j]], references[j], RAM[j]);
        if(RAM[j] == 0x800) break;
    }
    puts("=============================================");
    free(opcodes);
    free(references);
}

void dump_addr_rom_table(){
    puts("=========================");
    puts("OPC\t|\tCTRL ADDR\t|");
    puts("=========================");
    for (int i = 0; i < 16; ++i) {
        printf("%3s\t|\t%02x\t\t\t|\n", opcode_names[i], ADDR_ROM[i]);
    }
    puts("=========================");
}

void dump_ctrl_rom_table(){
    puts("=============================================================================");
    puts("    \t|\t      \t|\t                                  M H\t|\t    \t|");
    puts("    \t|\t      \t|\tI L E L     L E L E L E     E L C A L\t|\t    \t|");
    puts("ADDR\t|\tRAW   \t|\tP P P M R W D D I I A A A S U B D P T\t|\tCRJA\t|");
    puts("=============================================================================");
    for (int i = 0; i < 32; ++i) {
        printf("%02x\t\t|\t%06x\t|\t", i, CTRL_ROM[i]);
        int j;
        for (j=1; j < 20; ++j) printf("%s", ((CTRL_ROM[i] >> (24-j)) & 1) ? "X " : "  ");
        printf("\t|\t");
        for(; j <= 24; ++j) printf("%u", (CTRL_ROM[i] >> (24-j)) & 1);
        puts("\t|");
    }
    puts("=============================================================================");

}

void bootstrap(char *ram_filepath, char *addr_filepath, char *uprog_filepath){

    // parse ram.txt here and load into array
    FILE* ram_file;
    char ram_line[4] = ""; //each line is no longer than 12 bits and forth char is newline
    if(!(ram_file = fopen(ram_filepath, "rt"))){
        fprintf(stderr, "RAM FILE NOT FOUND");
        exit(11); //return bad mem error
    }

    for (int i = 0; fgets(ram_line, 4, ram_file) != NULL;) { //read file and filter out newlines
        if(ram_line[0] != '\n') RAM[i++] = (unsigned short) strtoul(ram_line, NULL, 16);
        if(!strncmp("800",ram_line, 3)) break;
    }

    puts("HERE BE THE RAM");
    dump_ram_table();

    //now do same for address ROM (addr.txt)
    FILE* addr_file;
    char addr_line[3] = ""; //5bit field to hex means range of 0-1f
    if(!(addr_file = fopen(addr_filepath, "rt"))){
        fprintf(stderr, "ADDRESS ROM FILE NOT FOUND");
        exit(11);
    }

    for (int j = 0; fgets(addr_line, 3, addr_file) != NULL;) {
        if(addr_line[0] != '\n') ADDR_ROM[j++] = (unsigned char) strtoul(addr_line, NULL, 16);
    }

    puts("\n\nHERE BE THE ADDRESS ROM");
    dump_addr_rom_table();

    //and lastly the control ROM (uprog.txt)
    FILE* uprog_file;
    char uprog_line[7];

    if(!(uprog_file = fopen(uprog_filepath, "rt"))){
        fprintf(stderr, "µROUTINE CONTROL ROM FILE NOT FOUND");
        exit(11);
    }

    for (int k = 0; fgets(uprog_line, 7, uprog_file) != NULL;) {
        if(uprog_line[0] != '\n') CTRL_ROM[k++] = (unsigned int) strtoul(uprog_line, NULL, 16);
    }

    puts("\n\nHERE BE THE CONTROL ROM");
    dump_ctrl_rom_table();

}

void register_print(const unsigned int *flags, unsigned int size, unsigned char ja){
    printf("%3s\t%02x\t%02x\t%2x\t%3x\t%3x\t%3x\t%3x\t%3x\t%3x\t", opcode_names[IR>>8], ja, UR, PC, MAR, MDR & 0xfff, ACC & 0xfff, ALU & 0xfff, RBX, IR);
    for (int i = 0; i < size; ++i) {
        switch (flags[i]){
            default:
                break;
            case EP:
                printf(" EP\t");
                break;
            case EA:
                printf(" EA\t");
                break;
            case ED:
                printf(" ED\t");
                break;
            case EI:
                printf(" EI\t");
                BUS = IR;
                break;
            case EU:
                printf(" EU\t");
                BUS = ALU;
                break;
            case R:
                printf("  R\t");
                break;
            case W:
                printf("  W\t");
                break;
            case LP:
                printf(" LP\t");
                break;
            case LM:
                printf(" LM\t");
                break;
            case LD:
                printf(" LD\t");
                break;
            case LI:
                printf(" LI\t");
                break;
            case LA:
                printf(" LA\t");
                break;
            case LB:
                printf(" LB\t");
                break;
            case A:
                printf("  A\t");
                break;
            case S:
                printf("  S\t");
                break;
            case IP:
                printf(" IP\t");
                break;
        }
    }
}

void clock_tick(){


    //decode instruction assign names ot different bits and what have you
    unsigned int fetched_inst = CTRL_ROM[UR] & 0xffffff00; //don't need CRJA field here
    unsigned int flag_count;
    for (flag_count = 0; fetched_inst; flag_count++) fetched_inst &= fetched_inst - 1; // get amt of enabled flags
    unsigned int flags[flag_count];
    //booleans here
    unsigned char increment = false;
    unsigned char crja = (unsigned char)(CTRL_ROM[UR]&0b11111);
    unsigned char hlt = (unsigned char)(CTRL_ROM[UR]&0x20);
    unsigned char map = (unsigned char)(CTRL_ROM[UR]&0x40);
    unsigned char cd = (unsigned char)(CTRL_ROM[UR]&0x80);
    unsigned short *receiver = NULL;

    //get indices of flags which will be matched to enum
    unsigned char index = 0;
    for (unsigned char i = 0; i < 16; ++i)(CTRL_ROM[UR] >> (23 - i) & 1) ? flags[index++] = i : 0;

    //get the party started
    for (int k = 0; k < flag_count; ++k) {
        switch (flags[k]){
            default:
                break;
            case EP:
                BUS = PC;
                break;
            case EA:
                BUS = ACC;
                break;
            case ED:
                BUS = MDR;
                break;
            case EI:
                BUS = IR;
                break;
            case EU:
                BUS = ALU;
                break;
            case R:
                MDR = RAM[MAR];
                break;
            case W:
                RAM[MAR] = MDR;
                break;
            case LP:
                receiver = (unsigned short *) &PC;
                break;
            case LM:
                receiver = (unsigned short *) &MAR;
                break;
            case LD:
                receiver = &MDR;
                break;
            case LI:
                receiver = &IR;
                break;
            case LA:
                receiver = &ACC;
                break;
            case LB:
                receiver = &RBX;
                break;
            case A:
                ALU = ACC + RBX;
                NEG = (unsigned char) (ALU >> 11 & 1);
                break;
            case S:
                ALU = ACC - RBX;
                NEG = (unsigned char) (ALU >> 11 & 1);
                break;
            case IP:
                increment = true; //using boolean to delay operation
                break;
            case CD:
                break;
        }
    }


    //perform register transfer if indicated
    if(receiver != NULL){
        if(receiver == (unsigned short *) &PC) *receiver = (unsigned short) (BUS & 0b11111111);
        else *receiver = BUS;
    }

    //adjust µ-instr counter accordingly
    if(map){
        UR = ADDR_ROM[IR>>8];
    }
    else if (cd) {
        UR = (unsigned short) (NEG ? crja : UR + 1);
    } else {
        UR = (unsigned short) crja;
    }

    if (increment) PC++;

    if (hlt){
        HALTED = true;
    }

    //print out debug contents
    register_print(flags, flag_count, crja);
    if (cd)  printf(" CD\t");
    if (map) printf("MAP\t");
    if (hlt) printf("HLT\t");
    if (NEG) printf("NEG\t");

    puts("");


}

int main(int argc, char** argv) {

    if(argc != 4){
        printf("Usage: ./eckert_sim [path to ram.txt] [path to addr.txt] [path to uprog.txt]\n");
        return 11;
    }

    bootstrap(argv[1], argv[2], argv[3]);
    printf("opc\tja\tµc\tpc\tmar\tmdr\tacc\talu\tb\tir\tflags\n");
    while (!HALTED){
        clock_tick();
    }

}
