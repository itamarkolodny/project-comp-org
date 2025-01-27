    .word 0x100 10                               # radius = 10

    lw $t2, $zero, $imm1, $zero, 0x100, 0         # load radius value
    beq $zero, $t2, $zero, $imm2, 0, END_PROG     # exit if no radius specified
    mac $t2, $t2, $t2, $zero, 0, 0                # square the radius


    add $t0, $zero, $zero, $zero, 0, 0            # start row counter

    add $t1, $zero, $zero, $zero, 0, 0            # init column value

PROCESS_SCREEN:
    blt $zero, $t0, $imm1, $imm2, 256, PIXEL_LOOP # check row bounds

    beq $zero, $zero, $zero, $imm2, 0, END_PROG   # screen completed


PIXEL_LOOP:
    blt $zero, $t1, $imm1, $imm2, 256, DISTANCE_CALC # check column bounds
    add $t0, $t0, $imm1, $zero, 1, 0              # next row
    add $t1, $zero, $zero, $zero, 0, 0            # reset column
    beq $zero, $zero, $zero, $imm2, 0, PROCESS_SCREEN # return to row check

DISTANCE_CALC:
    sub $a1, $t0, $imm1, $zero, 127, 0            # center offset for row
    sub $a2, $t1, $imm1, $zero, 127, 0            # center offset for column
    add $s0, $zero, $zero, $zero, 0, 0            # clear distance register
    mac $a1, $a1, $a1, $zero, 0, 0                # row offset squared
    mac $s0, $a2, $a2, $a1, 0, 0                  # total distance squared

    jal $ra, $zero, $zero, $imm2, 0, DRAW_POINT   # handle pixel drawing
    add $t1, $t1, $imm1, $zero, 1, 0              # move to next column
    beq $zero, $zero, $zero, $imm2, 0, PIXEL_LOOP # continue pixel processing

DRAW_POINT:
    ble $zero, $s0, $t2, $imm2, 0, WHITE_PIXEL    # check if inside circle
    add $s2, $zero, $zero, $zero, 0, 0            # set black color
    beq $zero, $zero, $zero, $imm2, 0, OUTPUT_PIXEL # proceed to drawing

WHITE_PIXEL:
    add $s2, $imm1, $zero, $zero, 255, 0          # set white color

OUTPUT_PIXEL:
    mac $s1, $t0, $imm1, $t1, 256, 0             # compute pixel address
    out $zero, $imm1, $zero, $s1, 20, 0          # set pixel location
    out $zero, $imm1, $zero, $s2, 21, 0          # assign pixel color
    out $zero, $imm1, $zero, $imm2, 22, 1        # draw the pixel
    beq $zero, $zero, $zero, $ra, 0, 0           # return to main loop

END_PROG:
    halt $zero, $zero, $zero, $zero, 0, 0         # terminate program
