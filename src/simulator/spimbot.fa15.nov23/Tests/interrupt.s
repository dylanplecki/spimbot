# tests whether an interrupt can be raised while another is pending
# desired behavior: handler should be entered only once

.data
TIMER_MASK = 0x8000
INTERRUPT_ENABLE = 1

TIMER = 0xffff001c
TIMER_ACK = 0xffff006c

PRINT_STRING = 4

request_timer:
	.word 1

entered_str:
	.asciiz "Entering interrupt handler\n"

timer_str:
	.asciiz "Entering timer interrupt\n"

.text
main:
	li	$t0, TIMER_MASK
	or	$t0, $t0, INTERRUPT_ENABLE
	mtc0	$t0, $12

	# enter interrupt asap
	lw	$t0, TIMER
	add	$t0, $t0, 1
	sw	$t0, TIMER

	jr	$ra

.kdata
chunkIH:
	.space 8

.ktext 0x80000180
interrupt_handler:
	.set noat
	move	$k1, $at
	.set at
	la	$k0, chunkIH
	sw	$v0, 0($k0)
	sw	$a0, 4($k0)

	li	$v0, PRINT_STRING
	la	$a0, entered_str
	syscall

interrupt_dispatch:
	mfc0	$k0, $13
	beq	$k0, 0, interrupt_end
	# only handling timer interrupts

timer_interrupt:
	li	$v0, PRINT_STRING
	la	$a0, timer_str
	syscall

	sw	$zero, TIMER_ACK
	lw	$k0, request_timer
	beq	$k0, 0, interrupt_dispatch
	sw	$zero, request_timer

	lw	$k0, TIMER
	add	$k0, $k0, 1
	sw	$k0, TIMER
	j	interrupt_dispatch

interrupt_end:
	la	$k0, chunkIH
	lw	$v0, 0($k0)
	lw	$a0, 4($k0)
	.set noat
	move	$at, $k1
	.set at
	eret
