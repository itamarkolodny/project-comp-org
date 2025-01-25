# enable interrupts
    out $t0, $zero, $imm, 1               # enable irq1
    add $t2, $zero, $imm, IRQ          # set interrupt handler address
    out $t2, $zero, $imm, 6               # set irqhandler = IRQ

    # read sectors 0-7 into memory starting at 1024
    add $t1, $imm, $zero, 8               # number of sectors to read
    add $t2, $zero, $zero, 0              # current sector
    add $s0, $zero, $imm, 1024         # memory address for sector 0

L1:
    out $t2, $imm, $zero, 15           # set sector number
    out $s0, $imm, $zero, 16           # set buffer address
    out $a0, $imm, $zero, 14           # read command
    jal $ra, $imm, $zero, PAUSE        # wait for disk ready
    add $t2, $t2, $imm, 1             # next sector
    add $s0, $s0, $imm, 128           # next buffer address
    blt $imm, $t2, $t1, L1            # loop for all sectors

    # init memory pointers for sectors
    add $s0, $zero, $imm, 1024        # sector 0 address
    add $s1, $zero, $imm, 1152        # sector 1 address
    add $s2, $zero, $imm, 1280        # sector 2 address
    add $a0, $zero, $imm, 1408        # sector 3 address
    add $a1, $zero, $imm, 1536        # sector 4 address
    add $a2, $zero, $imm, 1664        # sector 5 address
    add $a3, $zero, $imm, 1792        # sector 6 address
    add $gp, $zero, $imm, 1920        # sector 7 address

    # sum up sector values
    add $t0, $zero, $imm, 2048        # output buffer address
    add $t1, $zero, $zero, 0          # index
    add $t2, $zero, $imm, 128         # max index
    add $v0, $zero, $zero, 0          # sum register

L2:
    lw $sp, $s0, $t1, 0              # load from sector 0
    add $v0, $v0, $sp, 0             # add to sum
    lw $sp, $s1, $t1, 0              # load from sector 1
    add $v0, $v0, $sp, 0             # add to sum
    lw $sp, $s2, $t1, 0              # load from sector 2
    add $v0, $v0, $sp, 0             # add to sum
    lw $sp, $a0, $t1, 0              # load from sector 3
    add $v0, $v0, $sp, 0             # add to sum
    lw $sp, $a1, $t1, 0              # load from sector 4
    add $v0, $v0, $sp, 0             # add to sum
    lw $sp, $a2, $t1, 0              # load from sector 5
    add $v0, $v0, $sp, 0             # add to sum
    lw $sp, $a3, $t1, 0              # load from sector 6
    add $v0, $v0, $sp, 0             # add to sum
    lw $sp, $gp, $t1, 0              # load from sector 7
    add $v0, $v0, $sp, 0             # add to sum

    sw $v0, $t0, $t1, 0              # store sum
    add $t1, $t1, $imm, 1            # increment index
    add $v0, $zero, $zero, 0          # reset sum
    blt $imm, $t1, $t2, L2           # loop if not done

    # write result to sector 8
    add $s0, $zero, $imm, 8
    out $s0, $imm, $zero, 15         # set sector number
    out $t0, $imm, $zero, 16         # set buffer address
    add $t1, $imm, $zero, 2
    out $t1, $zero, $imm, 14         # write command
    jal $ra, $imm, $zero, PAUSE      # wait for disk ready
    halt $zero, $zero, $zero, 0       # halt

PAUSE:
    in $t0, $imm, $zero, 17          # get disk status
    bne $imm, $t0, $zero, PAUSE      # loop until ready
    beq $ra, $zero, $zero            # return

IRQ:
    out $zero, $imm, $zero, 4        # clear irq
    reti $zero, $zero, $zero, 0      # return from interrupt