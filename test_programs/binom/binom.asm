.word 0x100 5   # n = 5 (stored at address 0x100)
.word 0x101 2   # k = 2 (stored at address 0x101)

# Initialize stack pointer to top of stack
sll     $sp, $imm1, $imm2, $zero, 1, 11

# Load n and k into $a0 and $a1
lw      $a0, $zero, $imm2, $zero, 0, 0x100
lw      $a1, $zero, $imm2, $zero, 0, 0x101

# Call the binom function
jal     $ra, $zero, $zero, $imm2, 0, binom

# Store the result in memory address 0x102
sw      $zero, $zero, $imm2, $v0, 0, 0x102

# Halt execution
halt    $zero, $zero, $zero, $zero, 0, 0

# Function to calculate binom(n, k)
binom:
    # Allocate space on the stack for 4 items: $ra, $s0, $a0, $a1
    add     $sp, $sp, $imm2, $zero, 0, -4

    # Save caller's registers onto the stack
    sw      $zero, $sp, $imm2, $ra, 0, 3
    sw      $zero, $sp, $imm2, $s0, 0, 2
    sw      $zero, $sp, $imm2, $a0, 0, 1
    sw      $zero, $sp, $imm2, $a1, 0, 0

    # Base case: if k == 0 or n == k, return 1
    beq     $zero, $a1, $zero, $imm2, 0, return_one
    beq     $zero, $a1, $a0, $imm2, 0, return_one

    # Recursive case: calculate binom(n-1, k-1)
    sub     $a0, $a0, $imm2, $zero, 0, 1   # n = n - 1
    sub     $a1, $a1, $imm2, $zero, 0, 1   # k = k - 1
    jal     $ra, $zero, $zero, $imm2, 0, binom
    add     $s0, $v0, $zero, $zero, 0, 0   # $s0 = binom(n-1, k-1)

    # Restore $a0 and $a1 from stack for the second recursive call
    lw      $a0, $sp, $imm2, $zero, 0, 1   # Restore original n
    lw      $a1, $sp, $imm2, $zero, 0, 0   # Restore original k
    sub     $a0, $a0, $imm2, $zero, 0, 1   # n = n - 1
    jal     $ra, $zero, $zero, $imm2, 0, binom

    # Aggregate results
    add     $v0, $v0, $s0, $zero, 0, 0     # $v0 = binom(n-1, k-1) + binom(n-1, k)

    # Jump to return sequence
    beq     $zero, $zero, $zero, $imm2, 0, return

return_zero:
    add     $v0, $zero, $zero, $zero, 0, 0 # $v0 = 0
    beq     $zero, $zero, $zero, $imm2, 0, return

return_one:
    add     $v0, $zero, $imm2, $zero, 0, 1 # $v0 = 1

return:
    # Restore caller's registers from the stack
    lw      $ra, $sp, $imm2, $zero, 0, 3   # Restore $ra
    lw      $s0, $sp, $imm2, $zero, 0, 2   # Restore $s0
    lw      $a0, $sp, $imm2, $zero, 0, 1   # Restore $a0
    lw      $a1, $sp, $imm2, $zero, 0, 0   # Restore $a1

    # Deallocate stack space
    add     $sp, $sp, $imm2, $zero, 0, 4

    # Return to caller
    beq     $zero, $zero, $zero, $ra, 0, 0
