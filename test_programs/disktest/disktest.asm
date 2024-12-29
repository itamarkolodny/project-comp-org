# Disk test program
# Moves contents of sectors 0-7 forward by one position
# Uses DMA and interrupt handling
# Final state: sector 1 contains original sector 0, sector 2 contains original sector 1, etc.

# Initialize interrupt vector and enable interrupts
        add     $sp, $zero, $imm1, $zero, 0x400, 0    # Initialize stack pointer
        add     $t0, $zero, $imm1, $zero, isr, 0      # Load ISR address
        out     $zero, $imm1, $zero, $t0, 6, 0        # Set irqhandler to ISR address
        out     $zero, $imm1, $zero, $imm1, 1, 1      # Enable disk interrupt (irq1enable = 1)

        # Initialize buffer address and sector counter
        add     $s0, $zero, $imm1, $zero, 0x100, 0    # Buffer at 0x100
        add     $s1, $zero, $imm1, $zero, 7, 0        # Start with sector 7 (move backwards)

main_loop:
        # Check if disk is ready
wait_disk:
        in      $t0, $zero, $imm1, $zero, 17, 0       # Read diskstatus
        bne     $zero, $t0, $imm1, $zero, wait_disk, 0 # If busy, keep waiting

        # Read current sector
        out     $zero, $imm1, $zero, $s0, 16, 0       # Set diskbuffer
        out     $zero, $imm1, $zero, $s1, 15, 0       # Set disksector to current sector
        out     $zero, $imm1, $zero, $imm1, 14, 1     # Start read command

wait_read:
        in      $t0, $zero, $imm1, $zero, 17, 0       # Check diskstatus
        bne     $zero, $t0, $imm1, $zero, wait_read, 0 # Wait until read complete

        # Write to next sector
        out     $zero, $imm1, $zero, $s0, 16, 0       # Set diskbuffer (same buffer)
        add     $t0, $s1, $imm1, $zero, 1, 0          # Calculate next sector (current + 1)
        out     $zero, $imm1, $zero, $t0, 15, 0       # Set disksector to next sector
        out     $zero, $imm1, $zero, $imm1, 14, 2     # Start write command

wait_write:
        in      $t0, $zero, $imm1, $zero, 17, 0       # Check diskstatus
        bne     $zero, $t0, $imm1, $zero, wait_write, 0 # Wait until write complete

        # Move to previous sector
        sub     $s1, $s1, $imm1, $zero, 1, 0          # Decrement sector counter
        bge     $zero, $s1, $imm1, $zero, main_loop, 0 # Continue if sector >= 0

        halt    $zero, $zero, $zero, $zero, 0, 0       # End program

# Interrupt Service Routine
isr:
        # Clear the disk interrupt status
        out     $zero, $imm1, $zero, $zero, 4, 0      # Clear irq1status
        reti    $zero, $zero, $zero, $zero, 0, 0      # Return from interrupt