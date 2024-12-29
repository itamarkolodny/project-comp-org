// Created by Itamar Kolodny on 12/29/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 500
#define MAX_LABEL_LENGTH 50
#define MAX_LINES 4096
#define MEMORY_SIZE 4096
#define MAX_INST 4
#define NUM_INST 22
#define NUM_REG 16

// Structure to store labels and their addresses
typedef struct {
    char name[MAX_LABEL_LENGTH];
    int address;
} Label;

typedef struct {
    char name[MAX_INST];
    int value;
} Instruction_dict;

typedef struct {
    char name[MAX_INST];
    int value;
} Reg_dict;

// Structure to store instruction fields
typedef struct {
    int opcode;
    int rd;
    int rs;
    int rt;
    int rm;
    int imm1;
    int imm2;
} Instruction;

// Global variables
Label labels[MAX_LINES];
int label_count = 0;
char* register_names[] = {
    "$zero", "$imm1", "$imm2", "$v0", "$a0", "$a1", "$a2", "$t0",
    "$t1", "$t2", "$s0", "$s1", "$s2", "$gp", "$sp", "$ra"
};
char* inst_names[] = {
    "add", "sub", "$imm2", "$v0", "$a0", "$a1", "$a2", "$t0",
    "$t1", "$t2", "$s0", "$s1", "$s2", "$gp", "$sp", "$ra"
};
// Function prototypes
int get_register_number(char* reg_name);
int parse_immediate(char* imm_str, Label* labels, int label_count);
void first_pass(FILE* fp, Label* labels, int* label_count);
void second_pass(FILE* input_fp, FILE* imem_fp, FILE* dmem_fp, Label* labels, int label_count);
int is_number(char* str);
void remove_whitespace(char* str);
void remove_comments(char* line);
int parse_data_directive(char* line, FILE* dmem_fp);
int countInstructions(FILE* fp);

// Function to count valid instructions in the assembly file
int countInstructions(FILE* fp) {
    char line[MAX_LINE_LENGTH];
    int count = 0;
    long initial_pos = ftell(fp);  // Save current file position

    while (fgets(line, sizeof(line), fp)) {
        char cleaned_line[MAX_LINE_LENGTH];
        strcpy(cleaned_line, line);

        // Remove comments
        remove_comments(cleaned_line);

        // Remove whitespace
        remove_whitespace(cleaned_line);

        // Skip empty lines
        if (strlen(cleaned_line) == 0) continue;

        // Skip .word directives
        if (strncmp(cleaned_line, ".word", 5) == 0) continue;

        // Skip labels (lines ending with ':')
        int isLabel = 0;
        for (int i = 0; cleaned_line[i]; i++) {
            if (cleaned_line[i] == ':') {
                isLabel = 1;
                break;
            }
        }
        if (isLabel) continue;

        // If we got here, it's an instruction
        count++;

        // Safety check for instruction memory size
        if (count > MEMORY_SIZE) {
            printf("Error: Program exceeds maximum instruction memory size of %d\n", MEMORY_SIZE);
            fseek(fp, initial_pos, SEEK_SET);
            return -1;
        }
    }

    // Reset file position to where it was
    fseek(fp, initial_pos, SEEK_SET);
    return count;
}

void remove_comments(char* line) {
    char* comment = strchr(line, '#');
    if (comment) {
        *comment = '\0';
    }
}

void remove_whitespace(char* str) {
    char* d = str;
    do {
        while (isspace(*str)) str++;
    } while ((*d++ = *str++));
}

int is_number(char* str) {
    if (*str == '-' || *str == '+') str++;  // Skip sign if present
    if (strncmp(str, "0x", 2) == 0) {       // Check for hex number
        str += 2;
        while (*str) {
            if (!isxdigit(*str)) return 0;
            str++;
        }
    } else {                                // Decimal number
        while (*str) {
            if (!isdigit(*str)) return 0;
            str++;
        }
    }
    return 1;
}

int get_register_number(char* reg_name) {
    for (int i = 0; i < 16; i++) {
        if (strcmp(reg_name, register_names[i]) == 0) {
            return i;
        }
    }
    return -1;  // Register not found
}

int parse_immediate(char* imm_str, Label* labels, int label_count) {
    // Check if immediate is a label
    for (int i = 0; i < label_count; i++) {
        if (strcmp(imm_str, labels[i].name) == 0) {
            return labels[i].address;
        }
    }

    // Not a label, must be a number
    if (strncmp(imm_str, "0x", 2) == 0) {
        // Hexadecimal number
        int value;
        sscanf(imm_str + 2, "%x", &value);
        return value;
    } else {
        // Decimal number
        return atoi(imm_str);
    }
}

int parse_data_directive(char* line, FILE* dmem_fp) {
    char* token;
    int address, value;

    // Skip .word
    token = strtok(line, " ,");

    // Get address
    token = strtok(NULL, " ,");
    if (!token) return 0;
    address = parse_immediate(token, NULL, 0);

    // Get value
    token = strtok(NULL, " ,");
    if (!token) return 0;
    value = parse_immediate(token, NULL, 0);

    // Write to memory file
    fprintf(dmem_fp, "%08X\n", value);
    return 1;
}

void first_pass(FILE* fp, Label* labels, int* label_count) {
    // Get instruction count first
    int instruction_count = countInstructions(fp);
    if (instruction_count < 0) {
        // Handle error - program too large
        return;
    }

    char line[MAX_LINE_LENGTH];
    int current_address = 0;
    *label_count = 0;

    while (fgets(line, sizeof(line), fp)) {
        char cleaned_line[MAX_LINE_LENGTH];
        strcpy(cleaned_line, line);
        remove_comments(cleaned_line);
        remove_whitespace(cleaned_line);

        if (strlen(cleaned_line) == 0) continue;

        // Check for .word directive
        if (strncmp(cleaned_line, ".word", 5) == 0) continue;

        // Check for label (ends with ':')
        char* colon = strchr(cleaned_line, ':');
        if (colon) {
            *colon = '\0';  // Remove colon
            if (*label_count >= MAX_LINES) {
                printf("Error: Too many labels\n");
                return;
            }
            strncpy(labels[*label_count].name, cleaned_line, MAX_LABEL_LENGTH - 1);
            labels[*label_count].address = current_address;
            (*label_count)++;
            continue;
        }

        // If not a label or .word, must be an instruction
        current_address++;
    }
}

void second_pass(FILE* input_fp, FILE* imem_fp, FILE* dmem_fp, Label* labels, int label_count) {
    char line[MAX_LINE_LENGTH];
    Instruction_dict inst_dict[NUM_INST];
    Reg_dict reg_dict[NUM_REG];
    make_inst_dict(*inst_dict);
    make_reg_dict(*reg_dict);
    while (fgets(line, sizeof(line), input_fp)) {
        char cleaned_line[MAX_LINE_LENGTH];
        strcpy(cleaned_line, line);
        remove_comments(cleaned_line);
        remove_whitespace(cleaned_line);

        if (strlen(cleaned_line) == 0) continue;

        // Handle .word directive
        if (strncmp(cleaned_line, ".word", 5) == 0) {
            parse_data_directive(cleaned_line, dmem_fp);
            continue;
        }

        // Skip labels
        if (strchr(cleaned_line, ':')) continue;

        // Parse instruction
        Instruction inst;
        char* token = strtok(cleaned_line, " ,");
        

        // Parse opcode
        // ... (implement opcode parsing)

        // Parse registers and immediates
        // ... (implement register and immediate parsing)

        // Write instruction to imem file
        fprintf(imem_fp, "%02X%01X%01X%01X%01X%03X%03X\n",
                inst.opcode, inst.rd, inst.rs, inst.rt, inst.rm,
                inst.imm1 & 0xFFF, inst.imm2 & 0xFFF);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <program.asm> <imemin.txt> <dmemin.txt>\n", argv[0]);
        return 1;
    }

    FILE* input_fp = fopen(argv[1], "r");
    if (!input_fp) {
        printf("Error: Cannot open input file %s\n", argv[1]);
        return 1;
    }

    // First pass - collect all labels
    first_pass(input_fp, labels, &label_count);
    rewind(input_fp);

    // Second pass - generate code
    FILE* imem_fp = fopen(argv[2], "w");
    FILE* dmem_fp = fopen(argv[3], "w");
    if (!imem_fp || !dmem_fp) {
        printf("Error: Cannot open output files\n");
        fclose(input_fp);
        return 1;
    }

    second_pass(input_fp, imem_fp, dmem_fp, labels, label_count);

    fclose(input_fp);
    fclose(imem_fp);
    fclose(dmem_fp);
    return 0;
}