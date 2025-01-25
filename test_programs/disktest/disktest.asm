# Initialize
sll   $sp,   $imm1, $imm2, $zero, 1, 11
add   $t0,   $zero, $imm1,  $zero, 1, 0
out   $zero, $zero, $imm2,  $t0,   0, 1
out   $zero, $zero, $imm1,  $imm2, 6, IRQ

add   $s0,   $zero, $imm1, $zero, 1024, 0     # Buffer address
add   $s1,   $zero, $imm1,  $zero, 7, 0       # Start from sector 7
add   $s2,   $zero, $imm1,  $zero, 8, 0       # Loop count

LOOP:
    # Read current sector
    out   $zero, $zero, $imm2,  $s1,   0, 15
    out   $zero, $zero, $imm2,  $s0,   0, 16
    add   $t0,   $zero, $imm1,  $zero, 1, 0
    out   $zero, $zero, $imm2,  $t0,   0, 14

WAIT_READ:
    in    $t0,   $zero, $imm2,  $zero, 0, 17
    bne   $zero, $t0,   $zero,  $imm2, 0, WAIT_READ

    # Write to next sector
    add   $t0,   $s1,   $imm1,  $zero, 1, 0
    out   $zero, $zero, $imm2,  $t0,   0, 15
    out   $zero, $zero, $imm2,  $s0,   0, 16
    add   $t0,   $zero, $imm1,  $zero, 2, 0
    out   $zero, $zero, $imm2,  $t0,   0, 14

WAIT_WRITE:
    in    $t0,   $zero, $imm2,  $zero, 0, 17
    bne   $zero, $t0,   $zero,  $imm2, 0, WAIT_WRITE

    sub   $s1,   $s1,   $imm1,  $zero, 1, 0    # Previous sector
    sub   $s2,   $s2,   $imm1,  $zero, 1, 0    # Decrement counter
    bne   $zero, $s2,   $zero,  $imm2, 0, LOOP

    halt  $zero, $zero, $zero,  $zero, 0, 0

IRQ:
    out   $zero, $zero, $imm2,  $zero, 0, 4
    reti  $zero, $zero, $zero,  $zero, 0, 0