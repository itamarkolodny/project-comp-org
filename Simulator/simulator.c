//
// Created by Tamara Bluzer on 07/01/2025.
//
// SIMP Processor Simulator
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
// Global variables
char* register_io_names[] =
    {"irq0enable", "irq1enable", "irq2enable", "irq0status", "irq1status", "irq2status", "irqhandler", "irqreturn",
    "clks", "leds", "display7seg", "timerenable", "timercurrent", "timermax", "diskcmd", "disksector", "diskbuffer",
    "diskstatus", "reserved", "reserved", "monitoraddr", "monitordata", "monitorcmd"};

// Constants
#define IMEM_SIZE 4096
#define DMEM_SIZE 4096
#define NUM_REGISTERS 16
#define SECTOR_SIZE 512
#define NUM_SECTORS 128
#define NUM_IO_REGISTERS 23
#define MONITOR_SIZE 256
#define INSTRUCTION_HEX_LENGTH 13
#define INIT_DATA_HEX_LENGTH 9
#define IRQ2IN_LENGTH 8
#define MAX_CYCLES 8

// Structures
typedef struct {
    int32_t regs[NUM_REGISTERS];  // CPU registers
    char imem[IMEM_SIZE][INSTRUCTION_HEX_LENGTH]; //instruction memory
    char dmem[DMEM_SIZE][INIT_DATA_HEX_LENGTH];      // 32-bit data memory
    int pc;                   // Program counter //FIXME 12 bits long check if valid
    uint32_t io_registers[NUM_IO_REGISTERS];  // I/O registers
    uint8_t disk[NUM_SECTORS][SECTOR_SIZE];   // Disk storage
    int disk_timer;                          // Counter for disk operations
    uint8_t dma_buffer[SECTOR_SIZE];    // DMA buffer for disk operations
    uint8_t monitor[MONITOR_SIZE][MONITOR_SIZE]; // Monitor frame buffer
    bool halt;                     // Halt flag
    bool in_isr;  // ISR flag
    int* irq2_cycles;        // Array of irq2 cycles
    int num_irq2_cycles;     // Number of irq2 cycles
    int current_irq2_index;  // Current index in irq2_cycles array
} CPU;

// Function prototypes
void init_cpu(CPU *cpu);
bool load_instruction_memory(CPU *cpu, const char *filename);
bool load_data_memory(CPU *cpu, const char *filename);
bool load_irq2(CPU* cpu, const char* filename);
void fetch(CPU *cpu, char *instruction);
void decode(char *instruction, char *opcode, int *rd, int *rs,
           int *rt, int *rm, int *imm1, int *imm2);
void execute(CPU *cpu, char *opcode, int *rd, int *rs, int *rt,
            int *rm, int *imm1, int *imm2, FILE *trace_fp, uint32_t* cycle_count,
            FILE* leds_fp, FILE* hw_fp, FILE* display7seg_fp);
void handle_interrupts(CPU *cpu);
void handle_io(CPU *cpu);
void update_peripherals(CPU *cpu);
void update_trace(CPU *cpu, char *opcode, FILE* trace_fp);
int parse_hex_substring(char *str, int start, int end, int *result);
void update_leds(CPU* cpu,  uint32_t* cycle_count, FILE* leds_fp);
const char* get_register_name(int reg_io_num);
void update_display7seg(CPU* cpu, uint32_t* cycle_count, FILE* display7seg_fp);
void update_hwregs(CPU* cpu, uint32_t* cycle_count, bool write, int32_t address, FILE* hw_fp);
void update_monitor_buffer(CPU* cpu);
bool save_disk(CPU *cpu, const char *filename);
bool load_disk(CPU *cpu, const char *filename);
void start_disk_operation(CPU *cpu);



int main(int argc, char *argv[]) {
    if (argc != 15) {
        fprintf(stderr, "Usage: %s imemin.txt dmemin.txt diskin.txt irq2in.txt "
                "dmemout.txt regout.txt trace.txt hwregtrace.txt cycles.txt "
                "leds.txt display7seg.txt diskout.txt monitor.txt monitor.yuv\n", argv[0]);
        return 1;
    }

    CPU cpu;
    char instruction[INSTRUCTION_HEX_LENGTH];
    char opcode[3];
    int rd, rs, rt, rm, imm1, imm2;
    //char imm1[3], imm2[3];
    uint32_t cycle_count = 0;

    //initialize the cpu
    init_cpu(&cpu);

    //load input files
    printf("%s, %s, %s, %s\n", argv[1], argv[2], argv[3], argv[4]);
    if (!load_instruction_memory(&cpu, argv[1]) ||
        !load_data_memory(&cpu, argv[2]) ||
        !load_disk(&cpu, argv[3]) ||
        !load_irq2(&cpu,argv[4])) { //FIXME
        fprintf(stderr, "Error loading input files\n");
        return 1;
    }
    FILE* leds_fp = fopen(argv[10], "w");
    FILE* trace_fp = fopen(argv[7], "w");
    FILE* hw_fp = fopen(argv[8], "w");
    FILE* display7seg_fp = fopen(argv[11], "w");
    FILE* diskout_fp = fopen(argv[12], "w");

    //main execution loop
    while (!cpu.halt) {
        // Reset irq2status at start of cycle
        cpu.io_registers[5] = 0;
        // Check if we need to raise IRQ2 in this cycle
        if (cpu.current_irq2_index < cpu.num_irq2_cycles &&
            cycle_count-1 == cpu.irq2_cycles[cpu.current_irq2_index]) {
            cpu.io_registers[5] = 1;  // Set irq2status
            cpu.current_irq2_index++;
            }
        // Handle interrupts
        handle_interrupts(&cpu);

        //fetch
        fetch(&cpu, instruction);

        //decode
        decode(instruction, opcode, &rd, &rs, &rt, &rm, &imm1, &imm2);

        //execute & update trace.txt
        execute(&cpu, opcode, &rd, &rs, &rt, &rm, &imm1, &imm2 ,trace_fp, &cycle_count

        , leds_fp, hw_fp, display7seg_fp);

        // Handle I/O and update peripherals
        //handle_io(&cpu);
        update_peripherals(&cpu);

        //increment cycle count
        cycle_count++;
        cpu.io_registers[8]++;
        if (cpu.io_registers[8] == 0xFFFFFFFF) {
            cpu.io_registers[8] = 0;
        }
    }
    fclose(trace_fp);
    fclose(leds_fp);
    fclose(hw_fp);
    fclose(display7seg_fp);

    // Write output files

    //write dmemout.txt
    FILE* dmem_fp = fopen(argv[5], "w");
    for(int i = 0; i < DMEM_SIZE; i++) {
        int value;
        sscanf(cpu.dmem[i], "%x", &value);
        fprintf(dmem_fp, "%08X\n", value);
    }
    fclose(dmem_fp);

    //write regout.txt
    FILE* regout_fp = fopen(argv[6], "w");
    for(int i = 3; i < NUM_REGISTERS; i++) {
        fprintf(regout_fp, "%08X\n", cpu.regs[i]);
    }
    fclose(regout_fp);

     //write cycles.txt
    FILE* cycles_fp = fopen(argv[9], "w");
    fprintf(cycles_fp, "%d\n", cycle_count);
    fclose(cycles_fp);

    // diskout output
    if (!save_disk(&cpu, argv[12])) {
        fprintf(stderr, "Error writing disk output\n");
        // Handle error as appropriate
    }

    // Write monitor.txt
    FILE *txt = fopen(argv[13], "w");
    if (txt) {
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                fprintf(txt, "%02X\n", cpu.monitor[y][x]);
            }
        }
        fclose(txt);
    }

    // Write monitor.yuv
    FILE *yuv = fopen(argv[14], "wb");
    if (yuv) {
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                fwrite(&cpu.monitor[y][x], sizeof(uint8_t), 1, yuv);
            }
        }
        fclose(yuv);
    }
    free(cpu.irq2_cycles);
    return 0;
}



void init_cpu(CPU *cpu) {
    memset(cpu, 0, sizeof(CPU));
    cpu->in_isr = false;
}

/*bool load_instruction_memory(CPU *cpu, const char *filename) {
    char line[INSTRUCTION_HEX_LENGTH];
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Error load_instruction_memory\n");
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
}*/
bool load_instruction_memory(CPU *cpu, const char *filename) {
    char line[INSTRUCTION_HEX_LENGTH + 2];  // +2 for newline and null terminator
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Error load_instruction_memory: Unable to open file '%s'\n", filename);
        return false;
    }

    int i = 0;
    while (fgets(line, sizeof(line), file)) {
        // Remove newline and carriage return characters
        line[strcspn(line, "\r\n")] = 0;

        // Verify line length
        size_t line_len = strlen(line);
        if (line_len != 12) {  // Each instruction should be exactly 12 hex chars
            fprintf(stderr, "Invalid instruction length at line %d: got %zu chars, expected 12\n",
                    i + 1, line_len);
            fprintf(stderr, "Instruction: '%s'\n", line);
            fclose(file);
            return false;
        }

        // Copy instruction to memory
        strncpy(cpu->imem[i], line, 12);
        cpu->imem[i][12] = '\0';  // Ensure null termination
        i++;

        if (i >= IMEM_SIZE) {
            fprintf(stderr, "Warning: Instruction memory full\n");
            break;
        }
    }

    // Fill remaining memory with zeros if needed
    while (i < IMEM_SIZE) {
        strcpy(cpu->imem[i], "000000000000");
        i++;
    }

    fclose(file);
    return true;
}

bool load_data_memory(CPU *cpu, const char *filename) {
    char line[INIT_DATA_HEX_LENGTH+2];
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Error load_data_memory\n");
        return false;
    }

    int i = 0;

    while (fgets(line, sizeof(line), file)) { // assuming that the file is smaller then 4096 rows
        line[strcspn(line, "\n")] = 0;
        strcpy(cpu->dmem[i], line);
        printf("dmem[%d]:%s\n", i, cpu->dmem[i]);
        cpu->dmem[i][8] = '\0';
        i= i+1;
    }


    while (i < DMEM_SIZE) { // setting all zeroz after the last line in dmem.in
        strcpy(cpu->dmem[i], "00000000\0");
        i++;
    }
    fclose(file);
    return  true;
}

        /*char mline[9];
        int i = 0;
        while (!feof(filename))
        {
            fscanf(filename, "%8[^\n]\n", mline); //scans the first 8 chars in a line
            strcpy(cpu->dmem[i], mline); //fills array[i]
            i++;
        }
        while (i < SIZE)
        {
            strcpy(cpu->dmem[i], "00000000"); // paddin with zeros all the empty memory fills array[i]
            i++;
        }
    }*/

bool load_irq2(CPU* cpu, const char* filename) {
    FILE* fp;
    char line[500];
    int line_count = 0;

    // First count valid lines
    fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open irq2 file %s\n", filename);
        return false;
    }

    // Count non-empty lines
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] != '\n' && line[0] != '\r' && line[0] != ' ' && line[0] != '\t') {
            line_count++;
        }
    }
    fclose(fp);

    // Allocate memory
    cpu->irq2_cycles = (int*)malloc(line_count * sizeof(int));
    if (!cpu->irq2_cycles) {
        fprintf(stderr, "Error: Memory allocation failed for irq2_cycles\n");
        return false;
    }
    cpu->num_irq2_cycles = line_count;
    cpu->current_irq2_index = 0;

    // Read cycles
    fp = fopen(filename, "r");
    if (!fp) {
        free(cpu->irq2_cycles);
        fprintf(stderr, "Error: Cannot open irq2 file %s\n", filename);
        return false;
    }

    int cycle_count_i = 0;
    while (fgets(line, sizeof(line), fp)) {
        // Skip empty lines and whitespace
        if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t') {
            continue;
        }
        int cycle = atoi(line);
        cpu->irq2_cycles[cycle_count_i] = cycle;
        printf("irq2_cycles[%d] = %d\n", cycle_count_i, cycle);
        cycle_count_i++;
    }
    fclose(fp);
    return true;
}

void fetch(CPU *cpu, char *instruction) {
    if (cpu->pc < 0 || cpu->pc >= IMEM_SIZE) {
        fprintf(stderr, "Error: PC out of bounds: %d\n", cpu->pc);
        strcpy(instruction, "000000000000");  // Safe default
        return;
    }

    printf("Fetching instruction at PC=%d: '%s'\n", cpu->pc, cpu->imem[cpu->pc]);
    strcpy(instruction, cpu->imem[cpu->pc]);

    // Verify instruction length
    if (strlen(instruction) != 12) {
        fprintf(stderr, "Error: Invalid instruction length at PC=%d: '%s'\n",
                cpu->pc, instruction);
    }
}
    //strcpy(instruction, cpu->imem[cpu->pc]);
//}

/*int parse_hex_substring(char *str, int start, int end, int *result) {
    // Validate inputs
    if (!str || !result) {
        printf("Parsing error: null pointer\n");
        return 1;
    }

    // Calculate length and validate
    int len = end - start + 1;
    if (len <= 0) {
        printf("Parsing error: invalid length (start=%d, end=%d)\n", start, end);
        return 1;
    }

    // Add string length check
    int str_len = strlen(str);
    if (start >= str_len || end >= str_len) {
        printf("Parsing error: indices out of bounds (start=%d, end=%d, string_length=%d)\n",
               start, end, str_len);
        return 1;
    }

    // Allocate memory for substring (+1 for null terminator)
    char *temp = (char *)malloc((len + 1) * sizeof(char));
    if (!temp) {
        printf("Parsing error: memory allocation failed\n");
        return 1;
    }

    // Copy substring
    memcpy(temp, str + start, len);
    temp[len] = '\0';

    // Debug print
    printf("Attempting to parse substring: '%s' (len=%d)\n", temp, len);

    // Try to parse
    if (sscanf(temp, "%x", result) != 1) {
        printf("Parsing error: invalid hex value '%s'\n", temp);
        free(temp);
        return 1;
    }

    // Clean up and return
    free(temp);
    return 0;
}
*/
int parse_hex_substring(char *str, int start, int end, int *result) {
    // Validate inputs
    if (!str || !result) {
        printf("Parsing error: null pointer\n");
        return 1;
    }

    // Calculate length and validate
    int len = end - start + 1;
    if (len <= 0) {
        printf("Parsing error: invalid length (start=%d, end=%d)\n", start, end);
        return 1;
    }

    // Verify string length
    int str_len = strlen(str);
    if (str_len != 12) {  // All instructions should be exactly 12 chars
        printf("Parsing error: invalid instruction length %d\n", str_len);
        return 1;
    }

    if (start >= str_len || end >= str_len) {
        printf("Parsing error: indices out of bounds (start=%d, end=%d, string_length=%d)\n",
               start, end, str_len);
        return 1;
    }

    // Allocate memory for substring (+1 for null terminator)
    char *temp = (char *)malloc((len + 1) * sizeof(char));
    if (!temp) {
        printf("Parsing error: memory allocation failed\n");
        return 1;
    }

    // Copy substring and null terminate
    memcpy(temp, str + start, len);
    temp[len] = '\0';

    // Convert to uppercase to handle both cases
    for (int i = 0; i < len; i++) {
        temp[i] = toupper(temp[i]);
    }

    // Try to parse
    int ret = sscanf(temp, "%x", result);
    if (ret != 1) {
        printf("Parsing error: failed to parse hex value '%s'\n", temp);
        free(temp);
        return 1;
    }

    free(temp);
    return 0;
}
/*void decode(char *instruction, char *opcode, int *rd, int *rs,
           int *rt, int *rm, int *imm1, int *imm2) {
    strncpy(opcode, instruction, 2);
    opcode[2] = '\0';
    parse_hex_substring(instruction, 2, 2, rd);
    parse_hex_substring(instruction, 3, 3, rs);
    parse_hex_substring(instruction, 4, 4, rt);
    parse_hex_substring(instruction, 5, 5, rm);
    parse_hex_substring(instruction, 6, 8, imm1);
    parse_hex_substring(instruction,9 ,11, imm2);
    if (parse_hex_substring(instruction, 2, 2, rd) != 0 ||
    parse_hex_substring(instruction, 3, 3, rs) != 0 ||
    parse_hex_substring(instruction, 4, 4, rt) != 0 ||
    parse_hex_substring(instruction, 5, 5, rm) != 0 ||
    parse_hex_substring(instruction, 6, 8, imm1) != 0 ||
    parse_hex_substring(instruction, 9, 11, imm2) != 0) {
        fprintf(stderr, "Error decoding instruction: %s\n", instruction);
    }
}*/
void decode(char *instruction, char *opcode, int *rd, int *rs,
           int *rt, int *rm, int *imm1, int *imm2) {

    //verify instruction length
    if (strlen(instruction) != 12) {
        fprintf(stderr, "Invalid instruction length: %zu\n", strlen(instruction));
        return;
    }

    // Copy opcode (first 2 chars)
    strncpy(opcode, instruction, 2);
    opcode[2] = '\0';

    printf("Decoding instruction: %s\n", instruction);  // Debug print

    // Parse each field
    // rd is at position 2
    if (parse_hex_substring(instruction, 2, 2, rd) != 0) {
        fprintf(stderr, "Error parsing rd\n");
        return;
    }

    // rs is at position 3
    if (parse_hex_substring(instruction, 3, 3, rs) != 0) {
        fprintf(stderr, "Error parsing rs\n");
        return;
    }

    // rt is at position 4
    if (parse_hex_substring(instruction, 4, 4, rt) != 0) {
        fprintf(stderr, "Error parsing rt\n");
        return;
    }

    // rm is at position 5
    if (parse_hex_substring(instruction, 5, 5, rm) != 0) {
        fprintf(stderr, "Error parsing rm\n");
        return;
    }

    // imm1 is positions 6-8
    if (parse_hex_substring(instruction, 6, 8, imm1) != 0) {
        fprintf(stderr, "Error parsing imm1\n");
        return;
    }

    // imm2 is positions 9-11
    if (parse_hex_substring(instruction, 9, 11, imm2) != 0) {
        fprintf(stderr, "Error parsing imm2\n");
        return;
    }

    printf("Decoded: op=%s rd=%x rs=%x rt=%x rm=%x imm1=%x imm2=%x\n",
           opcode, *rd, *rs, *rt, *rm, *imm1, *imm2);  // Debug print
}


void update_trace(CPU *cpu, char *opcode, FILE* trace_fp) {
    fprintf(trace_fp, "%03X ", cpu->pc);
    fprintf(trace_fp, "%s ", cpu->imem[cpu->pc]);
    fprintf(trace_fp, "00000000 ");

   // int32_t imm1_value, imm2_value;
   // parse_hex_substring(cpu->imem[cpu->pc], 6, 8, &imm1_value);
   // parse_hex_substring(cpu->imem[cpu->pc], 9, 11, &imm2_value);

    fprintf(trace_fp, "%08x ", cpu->regs[1]);
    fprintf(trace_fp, "%08x", cpu->regs[2]);

    for(int i = 3; i < NUM_REGISTERS; i++) {
        fprintf(trace_fp, " %08x", cpu->regs[i]);
    }
    fprintf(trace_fp, "\n");
}

void execute(CPU *cpu, char *opcode, int *rd, int *rs, int *rt,
            int *rm, int *imm1, int *imm2, FILE *trace_fp, uint32_t* cycle_count, FILE* leds_fp, FILE* hw_fp, FILE* display7seg_fp) {
    cpu->regs[1] = (*imm1 & 0x800) ? (*imm1 | 0xFFFFF000) : *imm1;
    cpu->regs[2] = (*imm2 & 0x800) ? (*imm2 | 0xFFFFF000) : *imm2;
    update_trace(cpu, opcode, trace_fp);
    printf("reg[imm1] = %d, reg[imm2] = %d\n", cpu->regs[1], cpu->regs[2]);
    int op = 0;
    int32_t x_add = 0;
    int32_t address = 0;
    int32_t addi = 0;
    int32_t vali = 0;
    int32_t addressi = 0;
    sscanf(opcode, "%x", &op);
    switch (op) {
        case 0x00: //ADD
            cpu->regs[*rd] = cpu->regs[*rs] + cpu->regs[*rt] + cpu->regs[*rm];
            printf("rs reg value = %d, rt reg value =%d , rm reg value = %d",cpu->regs[*rs], cpu->regs[*rt], cpu->regs[*rm] );
            cpu->pc ++;
            break;
        case 0x01: //SUB
            cpu->regs[*rd] = cpu->regs[*rs] - cpu->regs[*rt] - cpu->regs[*rm];
            cpu->pc ++;
            break;
        case 0x02: //MAC
            cpu->regs[*rd] = (cpu->regs[*rs] * cpu->regs[*rt])+ cpu->regs[*rm];
            printf("rd after mul = %d", cpu->regs[*rd]);
            cpu->pc ++;
            break;
        case 0x03: //AND
            cpu->regs[*rd] = cpu->regs[*rs] & cpu->regs[*rt] & cpu->regs[*rm]; // need to check if it is & or &&
            cpu->pc ++;
            break;
        case 0x04://OR
            cpu->regs[*rd] = cpu->regs[*rs] | cpu->regs[*rt] | cpu->regs[*rm];
            cpu->pc ++;
            break;
        case 0x05://XOR
            cpu->regs[*rd] = cpu->regs[*rs] ^ cpu->regs[*rt] ^ cpu->regs[*rm];
            cpu->pc ++;
            break;
        case 0x06://SLL
            cpu->regs[*rd] = cpu->regs[*rs] << cpu->regs[*rt];
            cpu->pc ++;
            break;
        case 0x07://SRA
            cpu->regs[*rd] = cpu->regs[*rs] >> cpu->regs[*rt];// need to check
            cpu->pc++;
            break;
        /*case 0x08://SRL
            uint32_t value = (uint32_t) cpu->regs[*rs]; // need to check
            cpu->regs[*rd] = (int32_t) (value >> cpu->regs[*rt]);
            cpu->pc++;
            break;*/
        case 0x08: { //SRL
            uint32_t value = (uint32_t)cpu->regs[*rs];
            cpu->regs[*rd] = (int32_t)(value >> cpu->regs[*rt]);
            cpu->pc++;
            break;
        }
        case 0x09: //BEQ
            if (cpu->regs[*rt] != cpu->regs[*rs]) {
                cpu->pc++;
            }
            else {
                cpu->pc = (cpu->regs[*rm] & 0xFFF);
            }
            break;
        case 0x0a: //BNE
            if (cpu->regs[*rt] == cpu->regs[*rs]) {
                cpu->pc++;
            }
            else {
                cpu->pc = (cpu->regs[*rm] & 0xFFF);
            }
            break;
        case 0x0b: //BLT
            printf("reach BLT");
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
            printf("rd = %d, rm = %d , pc = %d\n", cpu->regs[*rd], cpu->regs[*rm], cpu->pc);
            break;
        case 0x10://LW
            address = cpu->regs[*rs] + cpu->regs[*rt];
            int32_t val = 0;
            if (address >= 0 && address < DMEM_SIZE) {
                sscanf(cpu->dmem[address], "%x", &val);
                printf("LW: address=%d, dmem[address+1]=%s, val=%d\n",
              address, cpu->dmem[address], val);
                cpu->regs[*rd] = val + cpu->regs[*rm];
            } else {
                fprintf(stderr, "Invalid memory address: %d\n", address);
                return; // Handle error
            }
            cpu->pc++;
            break;
        case 0x11: //SW
            addi = cpu->regs[*rs] + cpu->regs[*rt];
            vali = cpu->regs[*rd] + cpu->regs[*rm];
            if (addi >= 0 && addi < DMEM_SIZE) {
                //sscanf(cpu->dmem[addi], "%x", &vali); // incorrect
                sprintf(cpu->dmem[addi], "%08x", cpu->regs[*rm] + cpu->regs[*rd]);
            } else {
                fprintf(stderr, "Invalid array index: %d (rs=%d, rt=%d)\n",
          addi, cpu->regs[*rs], cpu->regs[*rt]);
                cpu->halt = true;
                exit(1);
            }
            //sscanf(cpu->dmem[addi], "%x", &vali);
            cpu->pc++;
            break;
        case 0x12: //RETI
            cpu->pc = cpu->io_registers[7];
            cpu->in_isr = false;  // Clear the ISR flag
            break;
        case 0x13: //IN
            addressi = cpu->regs[*rs] + cpu->regs[*rt];
            cpu->regs[*rd] = cpu->io_registers [addressi];
            cpu->pc ++;
            update_hwregs(cpu, cycle_count, 0, addressi ,hw_fp);
            break;
        case 0x14://OUT
            x_add = cpu->regs[*rs] + cpu->regs[*rt];
            if ((x_add == 9) && (cpu->io_registers[x_add] != cpu->regs[*rm])) { //update leds.txt output file
                cpu->io_registers[x_add] = cpu->regs[*rm];
                update_leds(cpu, cycle_count, leds_fp);
                update_hwregs(cpu, cycle_count, 1, x_add ,hw_fp);
                cpu->pc ++;
                break;
            }
            if ((x_add == 10) && (cpu->io_registers[x_add] != cpu->regs[*rm])) { //update display7seg.txt output file
                cpu->io_registers[x_add] = cpu->regs[*rm];
                update_display7seg(cpu, cycle_count, display7seg_fp);
                update_hwregs(cpu, cycle_count, 1, x_add ,hw_fp);
                cpu->pc ++;
                break;
            }
            if (x_add == 22) { //update monitor buffer
                cpu->io_registers[x_add] = cpu->regs[*rm];
                update_monitor_buffer(cpu);
                update_hwregs(cpu, cycle_count, 1, x_add ,hw_fp);
                cpu->pc ++;
                break;
            }
            cpu->io_registers[x_add] = cpu->regs[*rm];
            update_hwregs(cpu, cycle_count, 1, x_add ,hw_fp);
            cpu->pc ++;
            break;
        case 0x15: //HALT
            cpu->halt = true;
            break;
    }
}
void update_leds(CPU* cpu,  uint32_t* cycle_count, FILE* leds_fp) {
    fprintf(leds_fp, "%u %08x\n", *cycle_count, cpu->io_registers[9]);
}

void update_hwregs(CPU* cpu, uint32_t* cycle_count, bool write, int32_t address, FILE* hw_fp) {
    const char* write_read = write ? "WRITE" : "READ";
    const char* reg_name = get_register_name(address);
    if (address >= 0 && address < NUM_IO_REGISTERS) {
            fprintf(hw_fp, "%u %s %s %08x\n", *cycle_count, write ? "WRITE" : "READ",
                    get_register_name(address), cpu->io_registers[address]);
        } else {
            fprintf(stderr, "Invalid IO register address: %d\n", address);
        }
    //fprintf(hw_fp, "%u %s %s %08x\n", *cycle_count, write_read, reg_name, cpu->io_registers[address]);

    }


const char* get_register_name(int reg_io_num) {
    return register_io_names[reg_io_num];
}

void update_display7seg(CPU* cpu, uint32_t* cycle_count, FILE* display7seg_fp) {
    fprintf(display7seg_fp, "%u %08x\n", *cycle_count, cpu->io_registers[10]);
}

void update_monitor_buffer(CPU* cpu) {
    uint32_t offset = cpu->io_registers[20];
    uint32_t data = cpu->io_registers[21];

    uint32_t y = offset / 256; //line
    uint32_t x = offset % 256; //column

    //update the frame buffer
    cpu->monitor[y][x] = data;
}

void handle_interrupts(CPU *cpu) {
    bool irq =((cpu->io_registers[0] && cpu->io_registers[3]) || (cpu->io_registers[1] && cpu->io_registers[4]) || (cpu->io_registers[2] && cpu->io_registers[5]));
     if (irq && !cpu->in_isr) {
         // Save return address
         cpu->io_registers[7] = cpu->pc;
         // Jump to interrupt handler
         cpu->pc = cpu->io_registers[6];
         // Set ISR flag
         cpu->in_isr = true;
    }
}

void update_peripherals(CPU *cpu) {
    // Start disk operation if a command is pending
    if (  (cpu->io_registers[14] == 1 || cpu->io_registers[14] == 2) && cpu->io_registers[17] == 0) {
        start_disk_operation(cpu);
    }
    // Update timer if enabled
    if (cpu->io_registers[11]) { // if timerenable is 1
        cpu->io_registers[12]++; // increment timercurrent
        if (cpu->io_registers[12] == cpu->io_registers[13]) { // if timercurrent equals timermax
            cpu->io_registers[3] = 1; // Set irq0status
            cpu->io_registers[12] = 0; // Reset timercurrent back to 0
        }
    }

    // Handle disk timer if disk is busy
    if (cpu->io_registers[17] == 1) { // if diskstatus is busy
        cpu->disk_timer++;
        if (cpu->disk_timer >= 1024) { // disk operation completed
            cpu->io_registers[17] = 0; // set diskstatus to ready
            cpu->io_registers[14] = 0; // clear diskcmd
            cpu->io_registers[4] = 1;  // set irq1status (disk interrupt)
            cpu->disk_timer = 0;
        }
    }
}
// Function to start a disk operation
void start_disk_operation(CPU *cpu) {
    uint32_t command = cpu->io_registers[14];  // diskcmd
    uint32_t sector = cpu->io_registers[15];   // disksector
    uint32_t buffer = cpu->io_registers[16];   // diskbuffer (memory address)

    //DELETE
    /*// Validate parameters
    if (sector >= NUM_SECTORS) {
        // Invalid parameters - ignore command
        cpu->io_registers[14] = 0;  // Clear command
        return;
    }*/

    // Start operation if disk is ready
    if (cpu->io_registers[17] == 0) {
        cpu->io_registers[17] = 1;  // Set disk status to busy
        cpu->disk_timer = 0;         // Initialize timer

        // Perform DMA transfer based on command
        if (command == 1) {  // Read operation - disk to memory
            // Copy 128 words (512 bytes) from disk sector to memory buffer
            for (int i = 0; i < SECTOR_SIZE; i += 4) {
                uint32_t word = 0;
                // Construct 32-bit word from 4 bytes
                for (int j = 0; j < 4; j++) {
                    word |= (cpu->disk[sector][i + j] << (j * 8));
                }
                // Convert word to hex string in memory
                sprintf(cpu->dmem[buffer + i/4], "%08X", word);
            }
        }
        else if (command == 2) {  // Write operation - memory to disk
            // Copy 128 words (512 bytes) from memory buffer to disk sector
            for (int i = 0; i < SECTOR_SIZE; i += 4) {
                uint32_t word;
                // Convert hex string from memory to word
                sscanf(cpu->dmem[buffer + i/4], "%x", &word);

                // Decompose word into 4 bytes
                for (int j = 0; j < 4; j++) {
                    cpu->disk[sector][i + j] = (word >> (j * 8)) & 0xFF;
                }
            }
        }
    }
}

// Function to load disk content from file
bool load_disk(CPU *cpu, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return false;
    }

    char line[9];  // Should be changed to hold 8 hex chars + null terminator
    int sector = 0;
    int word_index = 0;  // Track 32-bit words instead of bytes

    // Reset entire disk to zero first
    memset(cpu->disk, 0, sizeof(cpu->disk));

    while (fgets(line, sizeof(line), file) && sector < NUM_SECTORS) {
        uint32_t word;
        line[strcspn(line, "\n")] = 0;

        if (sscanf(line, "%x", &word) == 1) {
            // Need to break the 32-bit word into 4 bytes
            for (int j = 0; j < 4; j++) {
                // Extract each byte from the 32-bit word
                cpu->disk[sector][word_index*4 + j] = (word >> (j * 8)) & 0xFF;
            }
            word_index++;

            // Move to next sector after 128 words (512 bytes)
            if (word_index >= SECTOR_SIZE/4) {  // 512/4 = 128 words per sector
                word_index = 0;
                sector++;
            }
        }
    }

    fclose(file);
    return true;
}


// Function to save disk content to file
bool save_disk(CPU *cpu, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        return false;
    }

    // Write entire disk content
    for (int sector = 0; sector < NUM_SECTORS; sector++) {
        // Process 128 words per sector (512 bytes / 4 bytes per word)
        for (int word_index = 0; word_index < SECTOR_SIZE/4; word_index++) {
            uint32_t word = 0;

            // Combine 4 bytes into a 32-bit word
            for (int j = 0; j < 4; j++) {
                word |= ((uint32_t)cpu->disk[sector][word_index*4 + j] << (j * 8));
            }

            // Write the word as 8 hex digits followed by newline
            fprintf(file, "%08X\n", word);
        }
    }

    fclose(file);
    return true;
}

