//
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

// Structure to store labels and their addresses
typedef struct {
    char name[MAX_LABEL_LENGTH];
    int address;
} Label;

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

// Function prototypes
int get_register_number(char* reg_name);
int parse_immediate(char* imm_str, Label* labels, int label_count);
void first_pass(FILE* fp, Label* labels, int* label_count);
void second_pass(FILE* input_fp, FILE* imem_fp, FILE* dmem_fp, Label* labels, int label_count);
int is_number(char* str);
void remove_whitespace(char* str);
void remove_comments(char* line);
int parse_data_directive(char* line, FILE* dmem_fp);

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