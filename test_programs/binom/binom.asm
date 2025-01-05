# Binomial coefficient calculator (n choose k)
# Input: n at address 0x100, k at address 0x101
# Output: result stored at address 0x102

# Main program
        lw      $a0, $zero, $imm1, $zero, 0x100, 0     # Load n from memory
        lw      $a1, $zero, $imm1, $zero, 0x101, 0     # Load k from memory
        add     $sp, $zero, $imm1, $zero, 0x400, 0     # Initialize stack pointer
        jal     $ra, $zero, $zero, $imm1, binom, 0     # Call binom(n,k)
        sw      $zero, $imm1, $zero, $v0, 0x102, 0     # Store result
        halt    $zero, $zero, $zero, $zero, 0, 0       # End program

# Function binom(n,k)
# Parameters: n in $a0, k in $a1
# Returns: result in $v0
binom:
        # Push return address and parameters to stack
        sub     $sp, $sp, $imm1, $zero, 3, 0          # Allocate stack space
        sw      $sp, $imm1, $zero, $ra, 0, 0          # Save return address
        sw      $sp, $imm1, $zero, $a0, 1, 0          # Save n
        sw      $sp, $imm1, $zero, $a1, 2, 0          # Save k

        # Check base cases: if k == 0 or n == k return 1
        beq     $zero, $a1, $zero, $imm1, return_one, 0  # if k == 0, return 1
        bne     $zero, $a0, $a1, $imm1, recurse, 0      # if n != k, continue

return_one:
        add     $v0, $zero, $imm1, $zero, 1, 0        # Return value = 1
        beq     $zero, $zero, $zero, $imm1, return, 0  # Jump to return

recurse:
        # First recursive call: binom(n-1, k-1)
        sub     $a0, $a0, $imm1, $zero, 1, 0          # n-1
        sub     $a1, $a1, $imm1, $zero, 1, 0          # k-1
        jal     $ra, $zero, $zero, $imm1, binom, 0    # Call binom(n-1,k-1)
        add     $s0, $v0, $zero, $zero, 0, 0          # Save first result

        # Restore n,k for second call
        lw      $a0, $sp, $imm1, $zero, 1, 0          # Restore n
        lw      $a1, $sp, $imm1, $zero, 2, 0          # Restore k

        # Second recursive call: binom(n-1, k)
        sub     $a0, $a0, $imm1, $zero, 1, 0          # n-1
        jal     $ra, $zero, $zero, $imm1, binom, 0    # Call binom(n-1,k)

        # Add results
        add     $v0, $v0, $s0, $zero, 0, 0            # $v0 = binom(n-1,k-1) + binom(n-1,k)

return:
        # Restore stack and return
        lw      $ra, $sp, $imm1, $zero, 0, 0          # Restore return address
        add     $sp, $sp, $imm1, $zero, 3, 0          # Deallocate stack space
        beq     $zero, $zero, $zero, $ra, 0, 0        # Return

