.data

NODE_SIZE = 12

# Syscall constants
PRINT_STRING	= 4

# Spimbot constants
VELOCITY		= 0xffff0010
ANGLE			= 0xffff0014
ANGLE_CONTROL	= 0xffff0018
BOT_X			= 0xffff0020
BOT_Y			= 0xffff0024

BONK_MASK		= 0x1000
BONK_ACK		= 0xffff0060

TIMER			= 0xffff001c
TIMER_MASK		= 0x8000
TIMER_ACK		= 0xffff006c

# Fruit constants
FRUIT_SCAN		= 0xffff005c
FRUIT_SMASH		= 0xffff0068

SMOOSHED_MASK	= 0x2000
SMOOSHED_ACK	= 0xffff0064

ENERGY_MASK   = 0x4000
ENERGY_ACK    = 0xffff00c4

REQ_PUZ_MASK  = 0x800
REQ_PUZ_ACK   = 0xffff00d8

REQUEST_WORD  = 0xffff00dc
REQUEST_PUZZLE = 0xffff00d0
SUBMIT_SOLUTION = 0xffff00d4

# Could be useful for debugging
PRINT_INT		= 0xffff0080

# -1: nothing, 0 = need target, 1 = chasing, 2 = waiting, 3 = smash_ready, 4 = smashing
.align 2
BOT_STATUS:		.word 0

.align 2
smooshed_fruit:	.word 0

.align 2
current_fruit:	.space 1 # Current fruit addr being chased

.align 2
min_frtdis:		.word 5 # Default min distance to fruit

.align 2
fruit_data:		.space 260

.align 2
puzzle_grid:		.space 8192
.align 2
puzzle_word:		.space 128
.align 2
node_memory:		.space 4096
.align 2
new_node_address: .word node_memory

.align 2
to_solve_puzzle: .byte 0

directions:
	.word -1  0
	.word  0  1
	.word  1  0
	.word  0 -1

.text # Main ASM code

main:
	# Initialize variables
	la	$t0, current_fruit
	sw	$0, 0($t0)

	# Enable interrupts
	li	$t4, TIMER_MASK		# Timer interrupt enable bit
	or	$t4, $t4, BONK_MASK	# Bonk interrupt bit
	or	$t4, $t4, SMOOSHED_MASK	# Smooshed interrupt bit
	or  $t4, $t4, ENERGY_MASK
	or  $t4, $t4, REQ_PUZ_MASK
	or	$t4, $t4, 1			# Global interrupt enable
	mtc0 $t4, $12			# Set interrupt mask (status register)

	# Get bot coord addresses
	la	$s0, BOT_STATUS
	la	$s1, BOT_X
	la	$s2, BOT_Y
	la	$s3, VELOCITY

	# Move to bottom
	la	$t1, ANGLE
	li	$t0, 90
	sw	$t0, 0($t1)
	la	$t1, ANGLE_CONTROL
	li	$t0, 1
	sw	$t0, 0($t1)
	#la	$t1, VELOCITY
	li	$t0, 10
	sw	$t0, 0($s3)

	# Wait until at bottom
	li	$t0, 275			# Minimum dist from bottom
m_bottom_loop:
	lw	$t1, 0($s2)
	bge $t1, $t0, m_end_bottom_loop
	j	m_bottom_loop

m_end_bottom_loop:
	sw	$0, 0($s3)

m_infinite:
   jal	check_solve_puzzle

	lw	$t0, 0($s0)
	li	$t1, 3
	beq	$t0, $t1, m_move_to_smash
	bne	$t0, $0, m_cont_inf_loop
	jal	target_fruit
	j	m_cont_inf_loop

m_move_to_smash:
	# Drive to bottom
	la	$t0, ANGLE
	li	$t1, 90
	sw	$t1, 0($t0)
	la	$t0, ANGLE_CONTROL
	li	$t1, 1
	sw	$t1, 0($t0)
	li	$t1, 10
	sw	$t1, 0($s3)

	# Update bot status
	li	$t0, 2
	sw	$t0, 0($s0)

m_cont_inf_loop:
	j	m_infinite			# Loop infinitely to process interrupts

## Functions ##

# void target_fruit() #
target_fruit:
	sub	$sp, $sp, 4
	sw	$ra, 0($sp)

	# Find next fruit
	la	$a0, min_frtdis
	lw	$a0, 0($a0)
	jal	find_next_fruit
	beq	$v0, $0, tf_req_timer
	la	$t0, current_fruit
	sw	$v0, 0($t0)

	# Get X coords
	lw	$t0, 8($v0)
	la	$t1, BOT_X
	lw	$t1, 0($t1)

	# Get params (if statement)
	beq	$t0, $t1, tf_ang_stay
	blt $t0, $t1, tf_ang_left
	j	tf_ang_right		# bgt $t0, $t1, tf_ang_right

	# Below: ($t0 = angle, $t1 = velocity)
tf_ang_stay:
	and	$t0, $t0, $0
	and	$t1, $t1, $0
	j tf_end_angle
tf_ang_left:
	li	$t0, 180
	li	$t1, 10
	j tf_end_angle
tf_ang_right:
	and	$t0, $t0, $0
	li	$t1, 10
	# j tf_end_angle
tf_end_angle:

	# Set angle and velocity
	li	$t2, 1
	la	$t3, ANGLE
	sw	$t0, 0($t3)
	la	$t3, ANGLE_CONTROL
	sw	$t2, 0($t3)
	la	$t3, VELOCITY
	sw	$t1, 0($t3)

	#li	$t0, 1
	#la	$t1, BOT_STATUS
	#sb	$t0, 0($t1)

	j tf_return

tf_req_timer: # Schedule timer if no fruit found
	lw	$t0, TIMER			# Current time
	add	$t0, $t0, 100
	sw	$t0, TIMER			# Request timer
	#li	$t0, -1
	#la	$t1, BOT_STATUS
	#sb	$t0, 0($t1)

tf_return:
	lw	$ra, 0($sp)
	add	$sp, $sp, 4
	jr	$ra

# fruit* find_next_fruit(int minDistY) #
find_next_fruit:
	sub	$sp, $sp, 36
	sw	$ra, 0($sp)
	sw	$s0, 4($sp)
	sw	$s1, 8($sp)
	sw	$s2, 12($sp)
	sw	$s3, 16($sp)
	sw	$s4, 20($sp)
	sw	$s5, 24($sp)
	sw	$s6, 28($sp)
	sw	$s7, 32($sp)

	# Get bot coordinates
	la	$s0, BOT_X
	la	$s1, BOT_Y
	lw	$s0, 0($s0)			# int botX
	lw	$s1, 0($s1)			# int botY

	# Establish variables
	and	$v0, $v0, $0		# fruit closest
	and	$s2, $s2, $0		# int cldis
	#and	$s3, $s3, $0	# int cldisy
	la	$s4, fruit_data		# fruit *curr
	sw	$s4, FRUIT_SCAN		# Populate array

fnf_loop:
	lw	$s5, 0($s4)			# fruit currF = *curr;
	beq	$s5, $0, fnf_return	# if (currF == NULL) break;
	lw	$s6, 8($s4)			# int currFX = currF.x;
	sub	$s6, $s0, $s6		# int disXR = botX - currFX;
	lw	$s7, 12($s4)		# int currFY = currF.y;
	sub	$s7, $s1, $s7		# int disYR = botY - currFY;
	ble	$s7, $0, fnf_cont_loop		# if (disYR > 0)
	abs	$s6, $s6			# int disX = abs(disXR);
	abs $s7, $s7			# int disY = abs(disYR);
	beq	$v0, $0, fnf_set_closest	# if (closest == NULL)

	# int $t0 = x^3 + y
	mul $t0, $s6, $s6
	mul $t0, $t0, $s6
	add $t0, $t0, $s7

	bge	$t0, $s2, fnf_cont_loop		# if (dis < cldis)
fnf_set_closest:
	move $v0, $s4			# closest = curr;
	move $s2, $t0			# cldis = dis;
fnf_cont_loop:
	add $s4, $s4, 16		# ++curr
	j	fnf_loop

fnf_return:
	lw	$ra, 0($sp)
	lw	$s0, 4($sp)
	lw	$s1, 8($sp)
	lw	$s2, 12($sp)
	lw	$s3, 16($sp)
	lw	$s4, 20($sp)
	lw	$s5, 24($sp)
	lw	$s6, 28($sp)
	lw	$s7, 32($sp)
	add	$sp, $sp, 36
	jr	$ra					# Return

check_solve_puzzle:
	sub $sp, $sp, 4
	sw	$ra, 0($sp)
	# Solve a puzzle if requested
m_if_solve_puzzle:
	and	$t1, $t1, $0
	lb	$t1, to_solve_puzzle
	beq	$t1, $0, m_end_if_spuzzle
	jal	solve_puzzle
	sb	$0, to_solve_puzzle
m_end_if_spuzzle:
	lw	$ra, 0($sp)
	add $sp, $sp, 4
	jr	$ra
   
#############################
# Node* allocate_new_node() #
#############################
allocate_new_node:
	lw	$v0, new_node_address
	add	$t0, $v0, NODE_SIZE
	sw	$t0, new_node_address
	jr	$ra


#################################################
# Node * set_node(int row, int col, Node *next) #
#################################################
set_node:
	sub	$sp, $sp, 16
	sw	$ra, 0($sp)
	sw	$a0, 4($sp)
	sw	$a1, 8($sp)
	sw	$a2, 12($sp)
	jal	allocate_new_node
	lw	$ra, 0($sp)
	lw	$a0, 4($sp)
	lw	$a1, 8($sp)
	lw	$a2, 12($sp)
	add	$sp, $sp, 16
	sw	$a0, 0($v0)
	sw	$a1, 4($v0)
	sw	$a2, 8($v0)
	jr	$ra


###################################################
# void remove_node(Node **head, int row, int col) #
###################################################
remove_node:
rn_for_loop:
	lw	$t0, 0($a0)					# Node *entry = *curr
	beq	$t0, $0, rn_end_for_loop	# for (entry != NULL)
	lw	$t1, 0($t0)					# 	[load] entry->row
	bne	$t1, $a1, rn_continue_loop	# 	if (entry->row == row)
	lw	$t2, 4($t0)					# 		[load] entry->col
	bne	$t2, $a2, rn_continue_loop	# 		if (entry->col == col)
	lw	$t1, 8($t0)					#			[load] entry->next
	sw	$t1, 0($a0)					#			*curr = entry->next
	jr	$ra							#			return
rn_continue_loop:
	add	$a0, $t0, 8					# 	curr = &entry->next
	j	rn_for_loop
rn_end_for_loop:
	jr	$ra							# return


#######################
# void start_puzzle() #
#######################
start_puzzle:
	la  $t0, puzzle_grid
	la  $t1, REQUEST_PUZZLE
	sw	$t0, 0($t1)
	jr	$ra


#######################
# void solve_puzzle() #
#######################
solve_puzzle:
	sub	$sp, $sp, 32
	sw	$ra, 0($sp)
	sw	$s0, 4($sp)
	sw	$s1, 8($sp)
	sw	$s2, 12($sp)
	sw	$s3, 16($sp)
	sw	$s4, 20($sp)
	sw	$s5, 24($sp)
	sw	$s6, 28($sp)
	# Get puzzle
	la  $s0, puzzle_grid
	# Get word to search for
	la  $s1, puzzle_word
	sw	$s1, REQUEST_WORD
	lw	$s2, 0($s0)			# num_rows
	lw	$s3, 4($s0)			# num_columns
	add	$s0, $s0, 8
	lb	$s6, 0($s1)			# word[0]
	beq	$s6, $0, sp_return	# Null word base-case
	# Loop initiation
	and	$s4, $s4, $0		# row
sp_r_loop:
	bge	$s4, $s2, sp_end_r_loop
	and	$s5, $s5, $0		# column
sp_c_loop:
	bge	$s5, $s3, sp_end_c_loop
	# Check for start of word
	mul		$t0, $s4, $s3				# next_row * num_cols
	add		$t0, $t0, $s5				# next_row * num_cols + next_col
	add		$t0, $t0, $s0				# &(puzzle[next_row * num_cols + next_col])
	lb		   $t0, 0($t0)
	#bne		$t0, $s6, sp_cont_loop		# Continue loop if puzzle[i][j] != word[0]
	# Call to search_neighbors
   sub   $a0, $s0, 8
	move	$a1, $s1
	move	$a2, $s4
	move	$a3, $s5
	jal	search_neighbors
	beq	$v0, $0, sp_cont_loop			# Continue loop if word not found
	sw	$v0, SUBMIT_SOLUTION			# Submit solution if nodes are available
	j	sp_return						# Return from function
sp_cont_loop:
	add	$s5, $s5, 1
	j	sp_c_loop
sp_end_c_loop:
	add	$s4, $s4, 1
	j	sp_r_loop
sp_end_r_loop:
sp_return:
	# Reset node pointer
	la  $t0, node_memory
	sw	$t0, new_node_address
	# Return
	lw	$ra, 0($sp)
	lw	$s0, 4($sp)
	lw	$s1, 8($sp)
	lw	$s2, 12($sp)
	lw	$s3, 16($sp)
	lw	$s4, 20($sp)
	lw	$s5, 24($sp)
	lw	$s6, 28($sp)
	add	$sp, $sp, 32
	jr	$ra


############################################################################
# Node* search_neighbors(char *puzzle, const char *word, int row, int col) #
############################################################################
search_neighbors: # TODO: Check for wrap-around scenarios
	# Run recursive base case
	bne	$a1, $0, sn_end_base_case	# if (word == NULL)
	move	$v0, $0						# return NULL
	jr		$ra
sn_end_base_case:
	# Store caller-saved registers
	sub		$sp, $sp, 28
	sw		$ra, 0($sp)
	sw		$s0, 4($sp)
	sw		$s1, 8($sp)
	sw		$s2, 12($sp)
	sw		$s3, 16($sp)
	sw		$s4, 20($sp)
	sw		$s5, 24($sp)
	# Get number of rows/columns
	lw		$s3, 0($a0)					# rows
	lw		$s4, 4($a0)					# columns
	add	$a0, $a0, 8					# move to puzzle address
	# Initialize for loop
	move	$s0, $0						# int i = 0
sn_for_loop:
	# Load temporary 'for' values
	li		$t0, 4
	# Check 'for' loop conditions
	bge		$s0, $t0, sn_end_for_loop	# for (i < 4)
	# Load temporary global var addresses
	la		$t0, directions
	# Get next_row, next_col, num_rows, num_cols
	mul		$s2, $s0, 8					# 2 col x 4 bytes
	add		$s2, $s2, $t0				# Get address offset
	lw		   $s1, 0($s2)					# directions[i][0]
	lw		   $s2, 4($s2)					# directions[i][1]
	add		$s1, $s1, $a2				# int next_row = row + directions[i][0]
	add		$s2, $s2, $a3				# int next_col = col + directions[i][1]
	# Check boundary (1L 'if' statement) with wrap-around
sn_recheck:
	bge		$s1, $0, sn_skip_1	# if (next_row < 0)
   add      $s1, $s3, $s1        #  next_row = num_rows + next_row
   j        sn_recheck
sn_skip_1:
	blt		$s1, $s3, sn_skip_2	# if (next_row >= num_rows)
   sub      $s1, $s1, $s3        #  next_row = next_row - num_rows
   j        sn_recheck
sn_skip_2:
	bge		$s2, $0, sn_skip_3	# if (next_col < 0)
   add      $s2, $s4, $s2        #  next_col = num_cols + next_col
   j        sn_recheck
sn_skip_3:
	blt		$s2, $s4, sn_skip_4	# if (next_col >= num_cols)
   sub      $s2, $s2, $s4        #  next_col = next_col - num_cols
   j        sn_recheck
sn_skip_4:
	# Get puzzle tile address
	mul		$s5, $s1, $s4				# next_row * num_cols
	add		$s5, $s5, $s2				# next_row * num_cols + next_col
	add		$s5, $s5, $a0				# &(puzzle[next_row * num_cols + next_col])
	# Load temporary 'if' values
	lb		$t0, 0($s5)					# puzzle[next_row * num_cols + next_col]
	lb		$t1, 0($a1)					# *word
	# Check equality (2L 'if' statement)
	bne		$t0, $t1, sn_end_bndeq_check	# if (puzzle[next_row * num_cols + next_col] == *word)
	# Load temporary 'if' values
	lb		$t0, 1($a1)					# *(word + 1)
	# Check null char (3L 'if' statement)
	bne		$t0, $0, sn_end_nullc_check	# if (*(word + 1) == '\0'
	# Return call to set_node
	#	_Don't need to save $aN regs_
	move	$a0, $s1
	move	$a1, $s2
	move	$a2, $0
	jal		set_node					# set_node(next_row, next_col, NULL)
	j		sn_return					# return
sn_end_nullc_check:
	# Mark visited puzzle position
	li		$t0, 42						# '*'
	sb		$t0, 0($s5)					# puzzle[next_row * num_cols + next_col] = '*'
	# Store arguments for function call
	sub	$sp, $sp, 16
	sw		$a0, 0($sp)
	sw		$a1, 4($sp)
	sw		$a2, 8($sp)
	sw		$a3, 12($sp)
	# Search for next char in word
	sub	$a0, $a0, 8
	add	$a1, $a1, 1					# word + 1
	move	$a2, $s1
	move	$a3, $s2
	jal	search_neighbors			# search_neighbors(puzzle, word + 1, next_row, next_col)
	# Load arguments after function call
	lw		$a0, 0($sp)
	lw		$a1, 4($sp)
	lw		$a2, 8($sp)
	lw		$a3, 12($sp)
	add		$sp, $sp, 16
	# Unmark visited puzzle position
	lb		$t0, 0($a1)					# *word
	sb		$t0, 0($s5)					# puzzle[next_row * num_cols + next_col] = *word
	# Check for valid neighbor
	beq		$v0, $0, sn_end_bndeq_check	# if (next_node)
	# Return call to set_node
	#	_Don't need to save $aN regs_
	move	$a0, $s1
	move	$a1, $s2
	move	$a2, $v0
	jal		set_node					# set_node(next_row, next_col, next_node)
	j		sn_return					# return
sn_end_bndeq_check:
	# Increment and loop
	add		$s0, $s0, 1					# i++
	j		sn_for_loop
sn_end_for_loop:
	# Return null on base
	move	$v0, $0						# return NULL
sn_return:
	# Restore caller-saved registers
	lw		$ra, 0($sp)
	lw		$s0, 4($sp)
	lw		$s1, 8($sp)
	lw		$s2, 12($sp)
	lw		$s3, 16($sp)
	lw		$s4, 20($sp)
	lw		$s5, 24($sp)
	add		$sp, $sp, 28
	# Jump back to caller function
	jr		$ra

   
.kdata # Kernel ASM data

chunkIH:		.space 8	# Space for two kernel registers
non_intrpt_str:	.asciiz "Non-interrupt exception\n"
unhandled_str:	.asciiz "Unhandled interrupt type\n"

.ktext 0x80000180 # Kernel ASM code

interrupt_handler:
.set noat
	move	$k1, $at		# Save $at.
.set at
	la	$k0, chunkIH
	sw	$a0, 0($k0)			# Get some free registers...
	sw	$a1, 4($k0)			# by storing them to a global variable

	mfc0	$k0, $13		# Get Cause register
	srl	$a0, $k0, 2
	and	$a0, $a0, 0xf		# ExcCode field
	bne	$a0, 0, non_intrpt

interrupt_dispatch:			# Interrupt:
	mfc0	$k0, $13		# 	Get Cause register, again
	beq	$k0, 0, done		# 	Handled all outstanding interrupts
	
	and $a0, $k0, ENERGY_MASK
	bne $a0, 0, energy_interrupt
	
	and $a0, $k0, REQ_PUZ_MASK
	bne $a0, 0, puzzle_interrupt

	and	$a0, $k0, BONK_MASK		# Is there a bonk interrupt?
	bne	$a0, 0, bonk_interrupt

	and	$a0, $k0, SMOOSHED_MASK	# Is there a smoosh interrupt?
	bne	$a0, 0, smooshed_interrupt

	and	$a0, $k0, TIMER_MASK	# Is there a timer interrupt?
	bne	$a0, 0, timer_interrupt

	# TODO: Add dispatch for other interrupt types here

	li	$v0, PRINT_STRING		# Unhandled interrupt types
	la	$a0, unhandled_str
	syscall 
	j	done
	
energy_interrupt:
	sw	$a1, ENERGY_ACK		# acknowledge interrupt
	
	# Request Puzzle Interrupt
	lb	$a0, to_solve_puzzle
	bne	$a0, $0, interrupt_dispatch
	la  $a0, puzzle_grid
	la  $a1, REQUEST_PUZZLE
	sw	$a0, 0($a1)
	
	j	interrupt_dispatch

puzzle_interrupt:
	sw	$a1, REQ_PUZ_ACK	# acknowledge interrupt
	
	la  $a0, to_solve_puzzle
	li  $a1, 1
	sb	$a1, 0($a0)
	
	j	interrupt_dispatch

bonk_interrupt:

	# Set bot status
	la	$a0, BOT_STATUS
	li	$a1, 4
	sw	$a1, 0($a0)
	
	# Smash fruits
	la	$k0, FRUIT_SMASH
	la	$k1, smooshed_fruit
	lw	$a0, 0($k1)
bi_smash_loop:
	ble	$a0, $0, bi_end_smash_loop
	sw	$a0, 0($k0)			# Smash a fruit
	sub	$a0, $a0, 1
	j	bi_smash_loop

bi_end_smash_loop:
	sw	$a0, 0($k1)
	sw	$a1, BONK_ACK		# Acknowledge interrupt

	# Drive back up
	la	$a0, ANGLE
	li	$a1, 270
	sw	$a1, 0($a0)
	la	$a0, ANGLE_CONTROL
	li	$a1, 1
	sw	$a1, 0($a0)
	la	$a0, VELOCITY
	li	$a1, 10
	sw	$a1, 0($a0)

	la	$k0, BOT_Y
	li	$k1, 275			# Minimum dist from bottom
bi_drive_loop:
	lw	$a1, 0($k0)
	ble $a1, $k1, bi_end_drive_loop
	j	bi_drive_loop

bi_end_drive_loop:
	sw	$0, 0($a0)

	# Set bot status
	la	$a0, BOT_STATUS
	li	$a1, 0
	sw	$a1, 0($a0)

	j	interrupt_dispatch	# See if other interrupts are waiting

smooshed_interrupt:	
	
	sw	$a1, SMOOSHED_ACK	# Acknowledge interrupt

	# TODO: Smash fruit (bonk)

	la	$a0, smooshed_fruit
	lw	$a1, 0($a0)
	add $a1, $a1, 1
	sw	$a1, 0($a0)

	# Check to smash fruit
	li	$a0, 5
	blt	$a1, $a0, si_continue

	# Set bot status
	la	$a0, BOT_STATUS
	li	$a1, 3
	sw	$a1, 0($a0)

si_continue:
	j	interrupt_dispatch	# See if other interrupts are waiting

timer_interrupt:
	sw	$a1, TIMER_ACK		# Acknowledge interrupt

	# TODO: Respond to timer

	j	interrupt_dispatch	# See if other interrupts are waiting

non_intrpt:					# Was some non-interrupt
	li	$v0, PRINT_STRING
	la	$a0, non_intrpt_str
	syscall					# Print out an error message
	# Fall through to done #

done:
	la	$k0, chunkIH
	lw	$a0, 0($k0)			# Restore saved registers
	lw	$a1, 4($k0)
.set noat
	move	$at, $k1		# Restore $at
.set at 
	eret
