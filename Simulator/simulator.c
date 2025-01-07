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
#define INSTRUCTION_HEX_LENGTH 12
#define INIT_DATA_HEX_LENGTH 8
#define IRQ2IN_LENGTH 3
// Structures
typedef struct {
    uint32_t regs[NUM_REGISTERS];  // CPU registers
    char imem[IMEM_SIZE][INSTRUCTION_HEX_LENGTH]; //instruction memory
    char dmem[DMEM_SIZE][INIT_DATA_HEX_LENGTH];      // 32-bit data memory
    uint32_t pc;                   // Program counter
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

char* fetch(CPU *cpu, int* pc;
void decode(uint64_t instruction, uint8_t *opcode, uint8_t *rd, uint8_t *rs,
           uint8_t *rt, uint8_t *rm, int16_t *imm1, int16_t *imm2);
void execute(CPU *cpu, uint8_t opcode, uint8_t rd, uint8_t rs, uint8_t rt,
            uint8_t rm, int16_t imm1, int16_t imm2);

void handle_interrupts(CPU *cpu);
void handle_io(CPU *cpu);
void update_peripherals(CPU *cpu);

bool write_output_files(CPU *cpu, const char *dmemout, const char *regout,
                       const char *trace, const char *hwregtrace, const char *cycles,
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
    uint64_t instruction;
    uint8_t opcode, rd, rs, rt, rm;
    int16_t imm1, imm2;
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
        fetch(&cpu, );

        // Decode
        decode(instruction, &opcode, &rd, &rs, &rt, &rm, &imm1, &imm2);

        // Execute
        execute(&cpu, opcode, rd, rs, rt, rm, imm1, imm2);

        // Handle I/O and update peripherals
        handle_io(&cpu);
        update_peripherals(&cpu);

        // Increment cycle count
        cycle_count++;
    }

    // Write output files
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
char* fetch(CPU *cpu, uint64_t *instruction) {
    *instruction = cpu->imem[cpu->pc];
    // TODO: Implement fetch stage
}

void decode(uint64_t instruction, uint8_t *opcode, uint8_t *rd, uint8_t *rs,
           uint8_t *rt, uint8_t *rm, int16_t *imm1, int16_t *imm2) {
    // TODO: Implement decode stage
}

void execute(CPU *cpu, uint8_t opcode, uint8_t rd, uint8_t rs, uint8_t rt,
            uint8_t rm, int16_t imm1, int16_t imm2) {
    // TODO: Implement execute stage
}

// Add other function implementations as needed