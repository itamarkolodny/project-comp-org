# Load the radius from memory and set up initial variables
lw $t0, $zero, $imm1, $zero, 0x100, 0    # Load radius from memory address 0x100
add $s0, $zero, $imm1, $zero, 128, 0      # Center x = 128 (256/2)
add $s1, $zero, $imm1, $zero, 128, 0      # Center y = 128 (256/2)
add $s2, $zero, $imm1, $zero, 255, 0      # White color value

# Initialize drawing variables
add $t1, $zero, $zero, $zero, 0, 0        # x = 0
add $t2, $t0, $zero, $zero, 0, 0          # y = radius

# Initialize decision parameter
mac $t3, $t0, $imm1, $zero, 3, -2         # d = 3 - 2r

DrawCircleLoop:
    # Draw 8 symmetric points for current (x,y)
    # Point (x+cx, y+cy)
    add $a0, $s0, $t1, $zero, 0, 0        # x1 = cx + x
    add $a1, $s1, $t2, $zero, 0, 0        # y1 = cy + y
    jal $ra, $zero, $zero, $imm1, DrawPoint, 0

    # Point (x+cx, -y+cy)
    add $a0, $s0, $t1, $zero, 0, 0        # x1 = cx + x
    sub $a1, $s1, $t2, $zero, 0, 0        # y1 = cy - y
    jal $ra, $zero, $zero, $imm1, DrawPoint, 0

    # Point (-x+cx, y+cy)
    sub $a0, $s0, $t1, $zero, 0, 0        # x1 = cx - x
    add $a1, $s1, $t2, $zero, 0, 0        # y1 = cy + y
    jal $ra, $zero, $zero, $imm1, DrawPoint, 0

    # Point (-x+cx, -y+cy)
    sub $a0, $s0, $t1, $zero, 0, 0        # x1 = cx - x
    sub $a1, $s1, $t2, $zero, 0, 0        # y1 = cy - y
    jal $ra, $zero, $zero, $imm1, DrawPoint, 0

    # Point (y+cx, x+cy)
    add $a0, $s0, $t2, $zero, 0, 0        # x1 = cx + y
    add $a1, $s1, $t1, $zero, 0, 0        # y1 = cy + x
    jal $ra, $zero, $zero, $imm1, DrawPoint, 0

    # Point (y+cx, -x+cy)
    add $a0, $s0, $t2, $zero, 0, 0        # x1 = cx + y
    sub $a1, $s1, $t1, $zero, 0, 0        # y1 = cy - x
    jal $ra, $zero, $zero, $imm1, DrawPoint, 0

    # Point (-y+cx, x+cy)
    sub $a0, $s0, $t2, $zero, 0, 0        # x1 = cx - y
    add $a1, $s1, $t1, $zero, 0, 0        # y1 = cy + x
    jal $ra, $zero, $zero, $imm1, DrawPoint, 0

    # Point (-y+cx, -x+cy)
    sub $a0, $s0, $t2, $zero, 0, 0        # x1 = cx - y
    sub $a1, $s1, $t1, $zero, 0, 0        # y1 = cy - x
    jal $ra, $zero, $zero, $imm1, DrawPoint, 0

    # Fill horizontal line
    add $t4, $zero, $t1, $zero, 0, 0      # temp x = x
FillLoop:
    # Draw points from -x to x at current y
    sub $a0, $s0, $t4, $zero, 0, 0        # x1 = cx - temp_x
    add $a1, $s1, $t2, $zero, 0, 0        # y1 = cy + y
    jal $ra, $zero, $zero, $imm1, DrawPoint, 0

    sub $a0, $s0, $t4, $zero, 0, 0        # x1 = cx - temp_x
    sub $a1, $s1, $t2, $zero, 0, 0        # y1 = cy - y
    jal $ra, $zero, $zero, $imm1, DrawPoint, 0

    add $t4, $t4, $imm1, $zero, -1, 0     # temp_x--
    bge $t4, $zero, $zero, $imm1, FillLoop, 0  # continue if temp_x >= 0

    # Update decision parameter
    blt $t3, $zero, $imm1, Label1, 0

    # d >= 0
    add $t3, $t3, $imm1, $zero, 4, 0      # d += 4
    mac $t3, $t1, $imm1, $t3, 4, 0        # d += 4*x
    sub $t2, $t2, $imm1, $zero, 1, 0      # y--
    beq $zero, $zero, $zero, $imm1, Label2, 0

Label1:    # d < 0
    add $t3, $t3, $imm1, $zero, 4, 0      # d += 4
    mac $t3, $t1, $imm1, $t3, 4, 0        # d += 4*x
    mac $t3, $t2, $imm1, $t3, -4, 0       # d -= 4*y

Label2:
    add $t1, $t1, $imm1, $zero, 1, 0      # x++
    ble $t1, $t2, $zero, $imm1, DrawCircleLoop, 0  # continue if x <= y

    halt $zero, $zero, $zero, $zero, 0, 0  # End program

# Subroutine to draw a point
DrawPoint:
    # Calculate pixel address (y * 256 + x)
    mac $t7, $a1, $imm1, $a0, 256, 0      # t7 = y * 256 + x

    # Write address to monitoraddr
    out $zero, $zero, $imm1, $t7, 20, 0   # Write to monitoraddr

    # Write color to monitordata
    out $zero, $zero, $imm1, $s2, 21, 0   # Write white color to monitordata

    # Set monitorcmd to 1 to write pixel
    out $zero, $zero, $imm1, $imm2, 22, 1 # Write 1 to monitorcmd

    beq $zero, $zero, $zero, $ra, 0, 0    # Return from subroutine