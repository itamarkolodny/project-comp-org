# Matrix multiplication of two 4x4 matrices
# Result[i][j] = sum(Matrix1[i][k] * Matrix2[k][j]) for k=0 to 3

# Initialize outer loop counter (i) - row of first matrix
add $t0, $zero, $imm1, $zero, 0, 0      # i = 0

outer_loop:
    # Initialize middle loop counter (j) - column of second matrix
    add $t1, $zero, $imm1, $zero, 0, 0  # j = 0

middle_loop:
    # Initialize accumulator for dot product
    add $v0, $zero, $zero, $zero, 0, 0  # sum = 0

    # Initialize inner loop counter (k)
    add $t2, $zero, $imm1, $zero, 0, 0  # k = 0

inner_loop:
    # Calculate address of Matrix1[i][k]
    mac $s0, $t0, $imm1, $zero, 4, 0    # i * 4
    add $s0, $s0, $t2, $zero, 0, 0      # i * 4 + k
    add $s0, $s0, $imm1, $zero, 0x100, 0 # base address + offset

    # Calculate address of Matrix2[k][j]
    mac $s1, $t2, $imm1, $zero, 4, 0    # k * 4
    add $s1, $s1, $t1, $zero, 0, 0      # k * 4 + j
    add $s1, $s1, $imm1, $zero, 0x110, 0 # base address + offset

    # Load values from both matrices
    lw $a0, $zero, $zero, $s0, 0, 0     # Load Matrix1[i][k]
    lw $a1, $zero, $zero, $s1, 0, 0     # Load Matrix2[k][j]

    # Multiply and accumulate
    mac $v0, $a0, $a1, $v0, 0, 0        # sum += Matrix1[i][k] * Matrix2[k][j]

    # Increment k and check if inner loop is done
    add $t2, $t2, $imm1, $zero, 1, 0    # k++
    blt $zero, $t2, $imm1, inner_loop, 4 # continue if k < 4

    # Store result in Matrix3[i][j]
    # Calculate result address
    mac $s0, $t0, $imm1, $zero, 4, 0    # i * 4
    add $s0, $s0, $t1, $zero, 0, 0      # i * 4 + j
    add $s0, $s0, $imm1, $zero, 0x120, 0 # base address + offset

    # Store the accumulated value
    sw $zero, $zero, $zero, $v0, $s0, 0

    # Increment j and check if middle loop is done
    add $t1, $t1, $imm1, $zero, 1, 0    # j++
    blt $zero, $t1, $imm1, middle_loop, 4 # continue if j < 4

    # Increment i and check if outer loop is done
    add $t0, $t0, $imm1, $zero, 1, 0    # i++
    blt $zero, $t0, $imm1, outer_loop, 4 # continue if i < 4

# End program
halt $zero, $zero, $zero, $zero, 0, 0