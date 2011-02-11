
.global _main

.section ".text"
.align 2
_main:
sts.l pr, @-r15
mov.l SysCall, r1
mov.w AllClr_DDVRAM, r0 
jsr @r1
nop
! two int on the stack
mov #0, r0
mov.l r0, @-r15
mov.l r0, @-r15

Loop1:
mov.l SysCall, r1
mov.w Getkey, r0
mov #0, r2
mov.l r2, @-r15
mov r15, r4
jsr @r1
nop
mov.l @r15+, r0
mov.w key_exe, r1
cmp/eq r1, r0
bt EndLoop
mov.w key_exe, r1
cmp/eq r1, r0
bt EndLoop
mov.w key_up, r1
cmp/eq r1, r0
bt UpCase
mov.w key_down, r1
cmp/eq r1, r0
bt DownCase
bra Loop1
nop

UpCase:
add #4, r15
mov.l @r15, r0
add #-1, r0
mov.l r0, @r15
add #-4, r15
bra EndCase
nop

DownCase:
add #4, r15
mov.l @r15, r0
add #1, r0
mov.l r0, @r15
add #-4, r15

EndCase:
mov.l SysCall, r1
mov.w SetPoint_DDVRAM, r0
mov #10, r4
mov #20, r5
mov #1, r6
jsr @r1
nop
bra Loop1
nop


EndLoop:
add #8, r15
lds.l @r15+, pr
rts
nop

.align 4
SysCall:
.long 0x80010070
AllClr_DDVRAM:
.word 0x144
SetPoint_DDVRAM:
.word 0x148
Getkey:
.word 0x90F


key_exe:
.word 30004
key_up:
.word 30018
key_down:
.word 30023
