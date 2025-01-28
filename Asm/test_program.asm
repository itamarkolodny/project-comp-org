# test_program.asm
# Test basic arithmetic operations and memory operations
start:
    add $t0, $t1, $t2, $zero, 5, 3      # Basic arithmetic
    sub $t1, $t0, $t2, $zero, 10, 0     # Subtraction
    lw $t2, $zero, $zero, $zero, 100, 0  # Load from memory
    sw $t2, $zero, $zero, $zero, 200, 0  # Store to memory
    beq $t0, $t1, done, $zero, 2, 0     # Branch test
done:
    halt $zero, $zero, $zero, $zero, 0, 0  # End program

# Memory initialization
.word 100, 42    # Store value 42 at address 100
.word 200, 0     # Initialize address 200 with 0