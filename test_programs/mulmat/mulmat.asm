# Matrix multiplication of two 4x4 matrices
#matrix a:
.word 0x100 1
.word 0x101 0
.word 0x102 0
.word 0x103 0
.word 0x104 0
.word 0x105 1
.word 0x106 0
.word 0x107 0
.word 0x108 0
.word 0x109 0
.word 0x10A 1
.word 0x10B 0
.word 0x10C 0
.word 0x10D 0
.word 0x10E 0
.word 0x10F 1
#matrix b:
.word 0x110 1
.word 0x111 0
.word 0x112 0
.word 0x113 0
.word 0x114 0
.word 0x115 1
.word 0x116 0
.word 0x117 0
.word 0x118 0
.word 0x119 0
.word 0x11A 1
.word 0x11B 0
.word 0x11C 0
.word 0x11D 0
.word 0x11E 0
.word 0x11F 1

sll $sp, $imm1, $imm2, $zero, 1, 11
add $sp, $sp, $imm2, $zero, 0, -4
sw $zero, $sp, $imm2, $s0, 0, 3
sw $zero, $sp, $imm2, $s1, 0, 2
sw $zero, $sp, $imm2, $a0, 0, 1
sw $zero, $sp, $imm2, $a1, 0, 0

add $t0, $zero, $zero, $zero, 0, 0     # i = 0

outer_loop:
 add $t1, $zero, $zero, $zero, 0, 0  # j = 0

middle_loop:
 add $v0, $zero, $zero, $zero, 0, 0  # sum = 0
 add $t2, $zero, $zero, $zero, 0, 0  # k = 0

inner_loop:
 # Matrix1[i][k]
 add $s0, $t0, $imm1, $zero, 256, 0   # base + i
 add $s0, $s0, $t2, $zero, 0, 0         # + k

 # Matrix2[k][j]
 mac $s1, $imm1, $t2, $zero, 4, 0 #k*4
 add $s1, $s1, $imm1, $zero, 272, 0   # base + k*4
 add $s1, $s1, $t1, $zero, 0, 0         # + j

 lw $a0, $zero, $s0, $zero, 0, 0
 lw $a1, $zero, $s1, $zero, 0, 0
#Matrix3[i][j] = martix1[i][k] * matrix2[k][j] + matrix3[i][j]
 mac $v0, $a0, $a1, $v0, 0, 0

 add $t2, $t2, $imm1, $zero, 1, 0 # k++
 blt $zero, $t2, $imm2, $imm1, inner_loop, 4

 add $s0, $t0, $imm1, $zero, 288, 0   # result base + i
 add $s0, $s0, $t1, $zero, 0, 0         # + j
 sw $zero, $s0, $zero, $v0, 0, 0

 add $t1, $t1, $imm1, $zero, 1, 0 #j++
 blt $zero, $t1, $imm2, $imm1, middle_loop, 4

 add $t0, $t0, $imm1, $zero, 4, 0 #i+4
 blt $zero, $t0, $imm2, $imm1, outer_loop, 16

lw $s0, $sp, $imm2, $zero, 0, 3
lw $s1, $sp, $imm2, $zero, 0, 2
lw $a0, $sp, $imm2, $zero, 0, 1
lw $a1, $sp, $imm2, $zero, 0, 0
add $sp, $sp, $imm2, $zero, 0, 4

halt $zero, $zero, $zero, $zero, 0, 0