# Initialize
add $sp, $zero, $imm1, $zero, 0x100, 0     # Set stack pointer to 0x100

# Load radius from memory
lw $a0, $zero, $imm1, $zero, 0x100, 0      # Load radius from memory address 0x100

# Set up screen center coordinates (128,128)
add $s0, $zero, $imm1, $zero, 128, 0       # Center x = 128
add $s1, $zero, $imm1, $zero, 128, 0       # Center y = 128
add $s2, $zero, $imm1, $zero, 255, 0       # White color value

# Loop through y coordinates
add $t0, $zero, $imm1, $zero, 0, 0         # y = -radius
sub $t0, $zero, $a0, $zero, 0, 0

yloop:
    # Loop through x coordinates
    add $t1, $zero, $imm1, $zero, 0, 0     # x = -radius
    sub $t1, $zero, $a0, $zero, 0, 0

xloop:
    # Calculate (x^2 + y^2)
    # Store x^2 in $t2
    mac $t2, $t1, $t1, $zero, 0, 0         # x^2
    # Add y^2 to get x^2 + y^2
    mac $v0, $t0, $t0, $t2, 0, 0           # x^2 + y^2

    # Calculate r^2
    mac $t2, $a0, $a0, $zero, 0, 0         # r^2

    # If (x^2 + y^2) <= r^2, plot the point
    bgt $zero, $v0, $t2, $imm1, skip_pixel, 0

    # Calculate actual screen coordinates
    add $a1, $t1, $s0, $zero, 0, 0         # screen_x = x + center_x
    add $a2, $t0, $s1, $zero, 0, 0         # screen_y = y + center_y

    # Calculate pixel address (y * 256 + x)
    mac $t2, $a2, $imm1, $zero, 256, 0     # y * 256
    add $t2, $t2, $a1, $zero, 0, 0         # y * 256 + x

    # Write pixel address to monitoraddr
    out $zero, $zero, $zero, $t2, 20, 0    # Set monitor address
    # Write white color to monitordata
    out $zero, $zero, $zero, $s2, 21, 0    # Set pixel color (255 = white)
    # Write command to draw pixel
    out $zero, $zero, $zero, $imm1, 22, 1  # Write pixel command

skip_pixel:
    # Increment x
    add $t1, $t1, $imm1, $zero, 1, 0
    ble $zero, $t1, $a0, $imm1, xloop, 0   # Continue if x <= radius

    # Increment y
    add $t0, $t0, $imm1, $zero, 1, 0
    ble $zero, $t0, $a0, $imm1, yloop, 0   # Continue if y <= radius

# Halt execution
halt $zero, $zero, $zero, $zero, 0, 0