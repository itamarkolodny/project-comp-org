//
// Created by Tamara Bluzer on 07/01/2025.
//
// SIMP Processor Simulator
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// Constants
#define IMEM_SIZE 4096
#define DMEM_SIZE 4096
#define NUM_REGISTERS 16
#define SECTOR_SIZE 512
#define NUM_SECTORS 128
#define NUM_IO_REGISTERS 23
#define MONITOR_SIZE 256
#define INSTRUCTION_HEX_LENGTH 13
#define INIT_DATA_HEX_LENGTH 8
#define IRQ2IN_LENGTH 3
// Structures
typedef struct {
    int32_t regs[NUM_REGISTERS];  // CPU registers
    char imem[IMEM_SIZE][INSTRUCTION_HEX_LENGTH]; //instruction memory
    char dmem[DMEM_SIZE][INIT_DATA_HEX_LENGTH];      // 32-bit data memory
    int pc;                   // Program counter //FIXME 12 bits long check if valid
    uint32_t io_registers[NUM_IO_REGISTERS];  // I/O registers
    uint8_t disk[NUM_SECTORS][SECTOR_SIZE];   // Disk storage
    uint8_t monitor[MONITOR_SIZE][MONITOR_SIZE]; // Monitor frame buffer
    bool halt;                     // Halt flag
} CPU;

// Function prototypes
void init_cpu(CPU *cpu);
bool load_instruction_memory(CPU *cpu, const char *filename);
bool load_data_memory(CPU *cpu, const char *filename);
//bool load_disk(CPU *cpu, const char *filename);
bool load_irq2(const char *filename);

char* fetch(CPU *cpu, int* pc);
void decode(char instruction, char *opcode, int *rd, int *rs,
           int *rt, int *rm, int *imm1, int *imm2);
void execute(CPU *cpu, char *opcode, int *rd, int *rs, int *rt,
            int *rm, int *imm1, int *imm2, FILE* trace_fp);

void handle_interrupts(CPU *cpu);
void handle_io(CPU *cpu);
void update_peripherals(CPU *cpu);
void read_write_regs(rd , value);
void update_trace(CPU *cpu, char *opcode, FILE* trace_fp);
int parse_hex_substring(char *str, int start, int end);
bool write_output_files(CPU *cpu, const char *dmemout, const char *regout,
                       const char *hwregtrace, const char *cycles,
                       const char *leds, const char *display7seg, const char *diskout,
                       const char *monitor_txt, const char *monitor_yuv);

int main(int argc, char *argv[]) {
    if (argc != 15) {
        fprintf(stderr, "Usage: %s imemin.txt dmemin.txt diskin.txt irq2in.txt "
                "dmemout.txt regout.txt trace.txt hwregtrace.txt cycles.txt "
                "leds.txt display7seg.txt diskout.txt monitor.txt monitor.yuv\n", argv[0]);
        return 1;
    }

    CPU cpu;
    char instruction[12];
    char opcode[3];
    int rd, rs, rt, rm;
    char imm1[3], imm2[3];
    uint32_t cycle_count = 0;

    //initialize the cpu
    init_cpu(&cpu);

    //load input files
    if (!load_instruction_memory(&cpu, argv[1]) ||
        !load_data_memory(&cpu, argv[2]) ||
       // !load_disk(&cpu, argv[3]) ||
        !load_irq2(argv[4])) { //FIXME
        fprintf(stderr, "Error loading input files\n");
        return 1;
    }

    // Main execution loop
    while (!cpu.halt) {
        // Handle interrupts
        handle_interrupts(&cpu);

        // Fetch
        fetch(&cpu, instruction);

        // Decode
        decode(instruction, &opcode, &rd, &rs, &rt, &rm, &imm1, &imm2);

        // Execute
        FILE* trace_fp = fopen(argv[7], "w");
        execute(&cpu, opcode, rd, rs, rt, rm, imm1, imm2 ,trace_fp);
        fclose(trace_fp);

        // Handle I/O and update peripherals
        handle_io(&cpu);
        update_peripherals(&cpu);

        // Increment cycle count
        cycle_count++;
    }

    // Write output files

    //write dmemout.txt
    FILE* dmem_fp = fopen(argv[5], "w");
    for(int i = 0; i <= DMEM_SIZE; i++) {
        fprintf(dmem_fp, "%s\n", cpu.dmem[i]);
    }
    fclose(dmem_fp);

    //write regout.txt
    FILE* regout_fp = fopen(argv[6], "w");
    for(int i = 3; i <= NUM_REGISTERS; i++) {
        fprintf(regout_fp, "%08X\n", cpu.regs[i]);
    }
    fclose(dmem_fp);

    if (!write_output_files(&cpu, argv[5], argv[6], argv[7], argv[8], argv[9],
                           argv[10], argv[11], argv[12], argv[13], argv[14])) {
        fprintf(stderr, "Error writing output files\n");
        return 1;
    }

    return 0;
}

void init_cpu(CPU *cpu) {
    memset(cpu, 0, sizeof(CPU));
}

bool load_instruction_memory(CPU *cpu, const char *filename) {
    char line[INSTRUCTION_HEX_LENGTH];
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        return false;
    }

    int i = 0;
    while (fgets(line, sizeof(line), file)) { // assuming that the file is smaller then 4096 rows
        line[strcspn(line, "\n")] = 0;
        strcpy(cpu->imem[i], line);
        i ++;
    }

    while (i < IMEM_SIZE) { // setting all zeroz after the last line in imem.in
        strcpy(cpu->imem[i], "000000000000");
        i++;
    }
    fclose(file);
    return true;
}

bool load_data_memory(CPU *cpu, const char *filename) {
    char line[INIT_DATA_HEX_LENGTH];
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        return false;
    }

    int i = 0;
    while (fgets(line, sizeof(line), file)) { // assuming that the file is smaller then 4096 rows
        line[strcspn(line, "\n")] = 0;
        strcpy(cpu->dmem[i], line);
        i ++;
    }

    while (i < DMEM_SIZE) { // setting all zeroz after the last line in dmem.in
        strcpy(cpu->dmem[i], "00000000\0");
        i++;
    }
    fclose(file);
    return true;
}

bool load_irq2(const char *filename){ //FIXME
    char line[IRQ2IN_LENGTH];
    FILE *file = fopen (filename, "r");

    if (file == NULL) {
        return false;
    }

    int i = 0;
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        strcpy(cpu->imem[i], line);
        i ++;
    }

    while (i < DMEM_SIZE) { // setting all zeroz after the last line in dmem.in
        strcpy(cpu->imem[i], "00000000");
        i++;
    }
    fclose(file);
    return true;
}
char* fetch(CPU *cpu, char *instruction) {
    *instruction = cpu->imem[cpu->pc];
}

int parse_hex_substring(char *str, int start, int end, int *result) {
    int len = end - start + 1;
    char c[len +1];
    strncpy(c, str+start,len);
    c[len] = '\0';
    if (sscanf(c, "%x", &result) == 1) {
        return 0;
    }
    else {
        printf("Parsing error");
        return 1;
    }
}

void decode(char instruction, char *opcode, int *rd, int *rs,
           int *rt, int *rm, int *imm1, int *imm2) {
    strncpy(opcode, instruction, 2);
    opcode[2] = '\0';
    parse_hex_substring(instruction, 2, 2, rd);
    parse_hex_substring(instruction, 3, 3, rs);
    parse_hex_substring(instruction, 4, 4, rt);
    parse_hex_substring(instruction, 5, 5, rm);
    parse_hex_substring(instruction, 6, 8, imm1);
    parse_hex_substring(instruction,9 ,11, imm2);
}
void read_write_regs(char *rd , value);
void update_trace(CPU *cpu, char *opcode, FILE* trace_fp) {
    fprintf(trace_fp, "%03X ", cpu->pc);
    fprintf(trace_fp, "%s ", cpu->imem[cpu->pc]);
    fprintf(trace_fp, "00000000 ");

    int32_t imm1_value, imm2_value;
    parse_hex_substring(cpu->imem[cpu->pc], 6, 8, &imm1_value);
    parse_hex_substring(cpu->imem[cpu->pc], 9, 11, &imm2_value);

    fprintf(trace_fp, "%08X ", (int32_t)imm1_value);
    fprintf(trace_fp, "%08X ", (int32_t)imm2_value);

    for(int i = 3; i < NUM_REGISTERS; i++) {
        fprintf(trace_fp, "%08X ", cpu->regs[i]);
    }
    fprintf(trace_fp, "\n");
}

void execute(CPU *cpu, char *opcode, int *rd, int *rs, int *rt,
            int *rm, int *imm1, int *imm2, FILE *trace_fp) {
    update_trace(cpu, opcode, trace_fp);
    switch (opcode) {
        case "00": //ADD
            cpu->regs[*rd] = cpu->regs[*rs] + cpu->regs[*rt] + cpu->regs[*rm];
            cpu->pc ++;
            break;
        case "01": //SUB
            cpu->regs[*rd] = cpu->regs[*rs] - cpu->regs[*rt] - cpu->regs[*rm];
            cpu->pc ++;
            break;
        case "02": //MAC
            cpu->regs[*rd] = cpu->regs[*rs] * cpu->regs[*rt] + cpu->regs[*rm];
            cpu->pc ++;
            break;
        case "03": //AND
            cpu->regs[*rd] = cpu->regs[*rs] & cpu->regs[*rt] & cpu->regs[*rm]; // need to check if it is & or &&
            cpu->pc ++;
            break;
        case "04"://OR
            cpu->regs[*rd] = cpu->regs[*rs] | cpu->regs[*rt] | cpu->regs[*rm];
            cpu->pc ++;
            break;
        case "05"://XOR
            cpu->regs[*rd] = cpu->regs[*rs] ^ cpu->regs[*rt] ^ cpu->regs[*rm];
            cpu->pc ++;
            break;
        case "06"://SLL
            cpu->regs[*rd] = cpu->regs[*rs] << cpu->regs[*rt];
            cpu->pc ++;
            break;
        case "07"://SRA
            cpu->regs[*rd] = cpu->regs[*rs] >> cpu->regs[*rt];// need to check
            cpu->pc ++;
            break;
        case "08"://SRL
            uint32_t value = (uint32_t) cpu->regs[*rs]; // need to check
            cpu->regs[*rd] = (int32_t) (value >> cpu->regs[*rt]);
            cpu->pc ++;
            break;
        case 0x09: //BEQ
            if (cpu->regs[*rt] != cpu->regs[*rs]) {
                cpu->pc ++;
            }
            else {
                cpu->pc = (cpu->regs[*rm] & 0xFFF);
            }
            break;
        case 0x0a: //BNE
            if (cpu->regs[*rt] == cpu->regs[*rs]) {
                cpu->pc ++;
            }
            else {
                cpu->pc = (cpu->regs[*rm] & 0xFFF);
            }
            break;
        case 0x0b: //BLT
            if (cpu->regs[*rs] >= cpu->regs[*rt]) {
                cpu->pc ++;
            }
            else {
                cpu->pc = (cpu->regs[*rm] & 0xFFF);
            }
            break;
        case 0x0c: //BGT
            if (cpu->regs[*rs] <= cpu->regs[*rt]) {
                cpu->pc ++;
            }
            else {
                cpu->pc = (cpu->regs[*rm] & 0xFFF);
            }
            break;
        case 0x0d://BLE
            if (cpu->regs[*rs] > cpu->regs[*rt]) {
                cpu->pc ++;
            }
            else {
                cpu->pc = (cpu->regs[*rm] & 0xFFF);
            }
            break;
        case 0x0e: //BGE
            if (cpu->regs[*rs] < cpu->regs[*rt]) {
                cpu->pc ++;
            }
            else {
                cpu->pc = (cpu->regs[*rm] & 0xFFF);
            }
            break;
        case 0x0f://JAL
            cpu->regs[*rd] = (cpu->pc) + 1 ;
            cpu->pc = (cpu->regs[*rm] & 0xFFF);
            break;
        case 0x10://LW
            int32_t address = cpu->regs[*rs] + cpu->regs[*rt];
            int32_t val;
            sscanf(cpu->dmem[address], "%x", &val);
            cpu->regs[*rd] = val + cpu->regs[*rm];
            cpu->pc ++;
            break;
        case 0x11: //SW
            int32_t addi = cpu->regs[*rs] + cpu->regs[*rt];
            int32_t vali = cpu->regs[*rd] + cpu->regs[*rm];
            sscanf(cpu->dmem[addi], "%x", &vali);
            cpu->pc ++;
            break;
        case 0x12: //RETI
            cpu->pc = cpu->io_registers[7];
            break;
        case 0x13: //IN
            int32_t addressi = cpu->regs[*rs] + cpu->regs[*rt];
            cpu->regs[*rd] = cpu-> io_registers [addressi];
            cpu->pc ++;
            break;
        case 0x14://OUT
            int32_t x_add = cpu->regs[*rs] + cpu->regs[*rt];
            cpu->io_registers[x_add] = cpu->regs[*rm];
            cpu->pc ++;
            break;
        case 0x15: //HALT
            break;

    }
}

// Add other function implementations as needed