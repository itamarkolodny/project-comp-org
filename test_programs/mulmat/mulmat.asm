# Setup base addresses for Matrix A, B, and C
add $a0, $zero, $zero, $zero, 0x100, 0  # Base address of Matrix A
add $a1, $zero, $zero, $zero, 0x110, 0  # Base address of Matrix B
add $a2, $zero, $zero, $zero, 0x120, 0  # Base address of Result Matrix C

# Initialize loop counters
add $t0, $zero, $zero, $zero, 0, 0       # i = 0, row counter for A and C
add $t1, $zero, $zero, $zero, 0, 0       # j = 0, column counter for B and C

# Outer loop label for iterating over rows of A
loop_i:
    # Inner loop for iterating over columns of B
    loop_j:
        add $t2, $zero, $zero, $zero, 0, 0  # sum = 0

        # k loop for dot product calculation
        add $t3, $zero, $zero, $zero, 0, 0  # k = 0
        dot_product_loop:
            # Calculate addresses for A[i][k], B[k][j], and C[i][j]
            lw $s0, $a0, $t0, $t3, 0, 0    # Load A[i][k]
            lw $s1, $a1, $t3, $t1, 0, 0    # Load B[k][j]
            mac $t2, $s0, $s1, $t2         # sum += A[i][k] * B[k][j]

            # Increment k
            add $t3, $t3, $imm1, $zero, 1, 0  # k++
            blt $t3, $imm1, $zero, dot_product_loop, 4, 0  # if k < 4 continue dot_product_loop

        # Store result in C[i][j]
        sw $a2, $t0, $t1, $t2, 0, 0    # C[i][j] = sum

        # Increment j
        add $t1, $t1, $imm1, $zero, 1, 0  # j++
        blt $t1, $imm1, $zero, loop_j, 4, 0  # if j < 4 continue loop_j

    # Increment i
    add $t0, $t0, $imm1, $zero, 1, 0  # i++
    blt $t0, $imm1, $zero, loop_i, 4, 0  # if i < 4 continue loop_i

# End program
halt
