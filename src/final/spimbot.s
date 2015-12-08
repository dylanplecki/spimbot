.data

# syscall constants
PRINT_STRING  = 4

# spimbot constants
VELOCITY      = 0xffff0010
ANGLE         = 0xffff0014
ANGLE_CONTROL = 0xffff0018
BOT_X         = 0xffff0020
BOT_Y         = 0xffff0024
GET_ENERGY	  = 0xffff00c8
PRINT_INT     = 0xffff0080
OTHER_BOT_X   = 0xffff00a0
OTHER_BOT_Y   = 0xffff00a4
FRUIT_SMASH   = 0xffff0068

SCAN_X        = 0xffff0050
SCAN_Y        = 0xffff0054
SCAN_RADIUS   = 0xffff0058
SCAN_ADDRESS  = 0xffff005c

BONK_MASK     = 0x1000
BONK_ACK      = 0xffff0060
SMOOSH_MASK   = 0x2000
SMOOSH_ACK    = 0xffff0064
ENERGY_MASK   = 0x4000
ENERGY_ACK    = 0xffff00c4
REQ_PUZ_MASK  = 0x800
REQ_PUZ_ACK   = 0xffff00d8

REQUEST_WORD  = 0xffff00dc
REQUEST_PUZZLE = 0xffff00d0
SUBMIT_SOLUTION = 0xffff00d4

TIMER         = 0xffff001c
TIMER_MASK    = 0x8000
TIMER_ACK     = 0xffff006c

three:	.float	3.0
five:	.float	5.0
PI:		.float	3.141592
F180:	.float  180.0

################
# Program Data #
################

.align 2
fruit_data:		.space 260

.align 2
puzzle_grid:	.space 8192
.align 2
puzzle_word:	.space 128
.align 2
node_memory:	.space 4096

new_node_address: .word node_memory

directions:
	.word -1  0
	.word  0  1
	.word  1  0
	.word  0 -1

SMOOSHED: .word 0

FRUIT_X: .word 0
FRUIT_Y: .word 0
FRUIT_ID: .word 0 

TAN_FRUIT: .word 0 
FRUIT_PTR: .word 0

solve_puzzle: .byte 0

.text
main:
	
	# TODO
	li	$t4, BONK_MASK				# bonk interrupt bit
	or $t4, $t4, SMOOSH_MASK
	or $t4, $t4, TIMER_MASK
	or	$t4, $t4, 1				# global interrupt enable
	mtc0	$t4, $12

	la $t0, fruit_data 
	sw $t0, SCAN_ADDRESS
	sw $t0, FRUIT_PTR
	
main_loop:
	la $t0, fruit_data 
	sw $t0, SCAN_ADDRESS
	
	# Solve a puzzle if requested
m_if_solve_puzzle:
	la  $t0, solve_puzzle
	and	$t1, $t1, $0
	lb	$t1, 0($t0)
	beq	$t1, $0, m_end_if_spuzzle
	sb	$0, 0($t0)
	jal	solve_puzzle
m_end_if_spuzzle:
	
	li $t6, 420
	sw $t6, TIMER
	
	lw $t3, FRUIT_PTR
	lw $t3, 0($t3)
	
	bne $t3, $0, not_null
	sw $t0, FRUIT_PTR
	
not_null:
	lw $t0, FRUIT_PTR
	lw $t5, 0($t0)
	lw $t1, FRUIT_ID
	
	# if fruit ID is the same we already know angle
	# and are moving towards it 
	beq $t1, $t5, main_skip
	
	li $t0, 420			# set timer 
	sw $t0, TIMER
	sw $t5, FRUIT_ID
	
	lw $t1, 8($t0)			# Fruit in x position
	sw $t1, FRUIT_X
	lw $t2, 12($t0) 		# Fruit in y position
	sw $t2, FRUIT_Y
	
	# We should write if statement here because
	# this will solve every new Fruit on screen
	jal start_puzzle
	
	jal predicted_fallout
	
	move $a0, $v0			# Move to this x value
	move $a1, $v1			# Move to this y value
	
	jal chase
	
main_skip:
	# Only want to solve puzzle while waiting for 
	# the ratio between x and y
	j main_loop
	

###########################################
# int x int y predictedFallout() #
###########################################

predicted_fallout:
#TODO:
	lw $t0, TAN_FRUIT
	lw $t4, FRUIT_X
	lw $t5, FRUIT_Y
	
	beq $t0, 0, zero
	bge $t0, 1, postive
	
negative:
	li $t1, 300
	div $t2, $t4, $t0	# y = x / tan(theta) 
	bge $t2, $t1, y_300
	
x_0:
	li $v0, 0
	move $v1, $t2
	jr $ra
	
	
positive: 
	li $t1, 300	
	sub $t1, $t1, $t4
	div $t2, $t1, $t0			# y = (300 - x) / Tan(theta) 
	bge $t2, $t1, y_300			# y >= 300
	
x_300: 
	li $v0, 300
	move $v1, $t2
	jr $ra 
	
y_300:
	li $v1, 300				# y = 300
	mul	 $v0, $t0, 300
	jr $ra
	
zero:
	lw $v0, FRUIT_X
	li $v1, 300
	jr $ra



jr $ra #wat

################
# void chase() #
################
chase:
	sub $sp, $sp, 12
	sw 	$a0, 0($sp)
	sw	$a1, 4($sp)
	sw	$ra, 8($sp)
	
	jal catchable
	
	lw 	$a0, 0($sp)
	lw	$a1, 4($sp)
	lw	$ra, 8($sp)
	
	beq $v0, $0, dont_chase
	
	#TO DO: Change velocity towards Predicted fruit
	jal go_to_location	
	
dont_chase:
	la $t0, fruit_data 
	sw $t0, SCAN_ADDRESS
	lw $t1, FRUIT_PTR
	add $t1, $t1, 16
	sw $t1, FRUIT_PTR
	
	lw $t1, 0($t1)
	sw $t1, FRUIT_ID		# Changing ID To Catch
	
	lw $a0, 0($sp)
	lw	$a1, 4($sp)
	lw	$ra, 8($sp)
	add $sp, $sp, 12
	jr		$ra


#######################
#  bool catchable(int x, int y, fruit* frt)       
#######################

catchable:
	lw $t0, BOT_X
	lw $t1, BOT_Y
	
	# Distance Formula
	sub $a0, $a0, $t0	  # x value difference
	abs $a0, $a0
	sub $a1, $a1, $t1     # Y value difference
	abs $a1, $a1
	
	jal euclidean_dis
	
	div $t0, $v0, 100	# time for fruit
	div $t1, $v0, 1000	# time for robot
	
	bge $t1, $t0, end
	li $v0, 1
	jr $ra

end:
	li $v0, 0
	jr $ra


#####################################
# void go_to_location(int x, int y) #
# goes to a give location(REWORKING TO GO WITH PAUL'S NEW CODE) #
#####################################	
go_to_location:
	sub $sp, $s0, 12
	sw	 $ra, 0($sp)
	sw  $s0, 4($sp)
	sw  $s1, 8($sp)

	move $s0, $a0
	move $s1, $a1

	lw $t9, BOT_X # bots x coord
	lw $t8, BOT_Y # bots y coord
	beq	$t9, $a0, at_x # check if we are at the x_coord
	beq	$t8, $a1, at_y # check if we are at the y_coord
	
gtl_loop:
	sub	$a0, $a0, $t9 # puts the fruits x - bot x into the first arg
	sub $a1, $a1, $t8 # puts the fruits y - bot y into the second arg
	jal sb_arctan     # v0 should have the angle we need now (Going to need to test this)
	sw	$v0, ANGLE
	li	$t6, 1
	sw	$t6, ANGLE_CONTROL
	li	$t7, 10
	sw	$t7, VELOCITY
	j	go_to_location

at_x:
	lw $t8, BOT_Y # bots y coord
	beq	$t8, $s1, at_location
	j	gtl_loop

at_y:
	lw $t8, BOT_Y # bots y coord
	beq	$t9, $s0, at_location
	j	gtl_loop
	
at_location:
	lw	 $ra, 0($sp)
	lw  $s0, 4($sp)
	lw  $s1, 8($sp)
	add $sp, $s0, 12
	jr	$ra #just returns to whatever called it
###########################################
# fruit* find_closest_fruit(int minDistY) #
###########################################
find_closest_fruit:
        sub     $sp, $sp, 36
        sw      $ra, 0($sp)
        sw      $s0, 4($sp)
        sw      $s1, 8($sp)
        sw      $s2, 12($sp)
        sw      $s3, 16($sp)
        sw      $s4, 20($sp)
        sw      $s5, 24($sp)
        sw      $s6, 28($sp)
        sw      $s7, 32($sp)
        # Get bot coordinates
        la      $s0, BOT_X
        la      $s1, BOT_Y
        lw      $s0, 0($s0)                     # int botX
        lw      $s1, 0($s1)                     # int botY
        # Establish variables
        and     $v0, $v0, $0            # fruit closest
        and     $s2, $s2, $0            # int cldisx
        and     $s3, $s3, $0            # int cldisy
        la      $s4, fruit_data         # fruit *curr
        sw      $s4, SCAN_ADDRESS         # Populate array
fnf_loop:
        lw      $s5, 0($s4)                 # fruit currF = *curr;
        beq     $s5, $0, fnf_return     	# if (currF == NULL) break;
        lw      $s6, 8($s4)                 # int currFX = currF.x;
        sub     $s6, $s0, $s6           	# int disXR = botX - currFX;
        lw      $s7, 12($s4)            	# int currFY = currF.y;
        sub     $s7, $s1, $s7           	# int disYR = botY - currFY;
        abs     $s6, $s6                    # int disX = abs(disXR);
        ab		$s7, $s7                    # int disY = abs(disYR);
        beq     $v0, $0, fnf_set_closest    # if (closest == NULL)
        bge     $s6, $s2, fnf_cont_loop     # if (disX < cldisX)
        bge     $s7, $s3, fnf_cont_loop     # if (disY < cldisY)
fnf_set_closest:
        move $v0, $s4                   # closest = curr;
        move $s2, $s6                   # cldisX = disX;
        move $s3, $s7                   # cldisY = disY;
fnf_cont_loop:
        add $s4, $s4, 16                # ++curr
        j       fnf_loop
fnf_return:
        lw      $ra, 0($sp)
        lw      $s0, 4($sp)
        lw      $s1, 8($sp)
        lw      $s2, 12($sp)
        lw      $s3, 16($sp)
        lw      $s4, 20($sp)
        lw      $s5, 24($sp)
        lw      $s6, 28($sp)
        lw      $s7, 32($sp)
        add     $sp, $sp, 36
        jr      $ra # Return


##########################################
# float euclidean_dist(float x, float y) #
##########################################
euclidean_dist:
	mul		$a0, $a0, $a0	# x^2
	mul		$a1, $a1, $a1	# y^2
	add		$v0, $a0, $a1	# x^2 + y^2
	mtc1	$v0, $f0
	cvt.s.w	$f0, $f0	# float(x^2 + y^2)
	sqrt.s	$f0, $f0	# sqrt(x^2 + y^2)
	cvt.w.s	$f0, $f0	# int(sqrt(...))
	mfc1	$v0, $f0
	jr		$ra


###############################
# int sb_arctan(int x, int y) #
###############################
sb_arctan:
	li		$v0, 0		# angle = 0;

	abs		$t0, $a0	# get absolute values
	abs		$t1, $a1
	ble		$t1, $t0, no_TURN_90	  

	## if (abs(y) > abs(x)) { rotate 90 degrees }
	move	$t0, $a1	# int temp = y;
	neg		$a1, $a0	# y = -x;      
	move	$a0, $t0	# x = temp;    
	li		$v0, 90		# angle = 90;  

no_TURN_90:
	bgez	$a0, pos_x 	# skip if (x >= 0)

	## if (x < 0)
	add		$v0, $v0, 180	# angle += 180;

pos_x:
	mtc1	$a0, $f0
	mtc1	$a1, $f1
	cvt.s.w $f0, $f0	# convert from ints to floats
	cvt.s.w $f1, $f1
	
	div.s	$f0, $f1, $f0	# float v = (float) y / (float) x;

	mul.s	$f1, $f0, $f0	# v^^2
	mul.s	$f2, $f1, $f0	# v^^3
	l.s		$f3, three	# load 5.0
	div.s 	$f3, $f2, $f3	# v^^3/3
	sub.s	$f6, $f0, $f3	# v - v^^3/3

	mul.s	$f4, $f1, $f2	# v^^5
	l.s		$f5, five	# load 3.0
	div.s 	$f5, $f4, $f5	# v^^5/5
	add.s	$f6, $f6, $f5	# value = v - v^^3/3 + v^^5/5

	l.s		$f8, PI		# load PI
	div.s	$f6, $f6, $f8	# value / PI
	l.s		$f7, F180	# load 180.0
	mul.s	$f6, $f6, $f7	# 180.0 * value / PI

	cvt.w.s $f6, $f6	# convert "delta" back to integer
	mfc1	$t0, $f6
	add		$v0, $v0, $t0	# angle += delta

	jr 	$ra


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
	lb	$s6, 0($s1)
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
	lb		$t0, 0($t0)
	bne		$t0, $s6, sp_cont_loop		# Continue loop if puzzle[i][j] != word[0]
	# Call to search_neighbors
	move	$a0, $s0
	move	$a1, $s1
	li	$a2, $s4
	li	$a3, $s5
	jal	search_neighbors
	sw	$v0, SUBMIT_SOLUTION
sp_cont_loop:
	add	$s5, $s5, 1
	j	sp_c_loop
sp_end_c_loop:
	add	$s4, $s4, 1
	j	sp_r_loop
sp_end_r_loop:
sp_return:
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
	bne		$a1, $0, sn_end_base_case	# if (word == NULL)
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
	add		$a0, $a0, 8					# move to puzzle address
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
	lw		$s1, 0($s2)					# directions[i][0]
	lw		$s2, 4($s2)					# directions[i][1]
	add		$s1, $s1, $a2				# int next_row = row + directions[i][0]
	add		$s2, $s2, $a3				# int next_col = col + directions[i][1]
	# Load temporary 'if' values
	li		$t0, -1
	# Check boundary (1L 'if' statement)
	ble		$s1, $t0, sn_end_bndeq_check	# if (next_row > -1)
	bge		$s1, $s3, sn_end_bndeq_check	# if (next_row < num_rows)
	ble		$s2, $t0, sn_end_bndeq_check	# if (next_col > -1)
	bge		$s2, $s4, sn_end_bndeq_check	# if (next_col < num_cols)
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
	sub		$sp, $sp, 16
	sw		$a0, 0($sp)
	sw		$a1, 4($sp)
	sw		$a2, 8($sp)
	sw		$a3, 12($sp)
	# Search for next char in word
	add		$a1, $a1, 1					# word + 1
	move	$a2, $s1
	move	$a3, $s2
	sub		$a0, $a0, 8
	jal		search_neighbors			# search_neighbors(puzzle, word + 1, next_row, next_col)
	add		$a0, $a0, 8
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
	
	
.kdata					# Interrupt handler data (separated just for readability)
chunkIH:	.space 16	# Space for several registers
non_intrpt_str:	.asciiz "Non-interrupt exception\n"
unhandled_str:	.asciiz "Unhandled interrupt type\n"

.ktext 0x80000180
interrupt_handler:
.set noat
	move	$k1, $at	# Save $at                               
.set at
	la	$k0, chunkIH
	sw	$a0, 0($k0)		# Get some free registers                  
	sw	$a1, 4($k0)		# by storing them to a global variable
	sw	$a2, 8($k0)
	sw	$v0, 12($k0) 
	sw  $t0, 16($k0)
	sw  $t1, 20($k0)
	sw  $ra, 24($k0)

	mfc0	$k0, $13	# Get Cause register                       
	srl	$a0, $k0, 2                
	and	$a0, $a0, 0xf	# ExcCode field                            
	bne	$a0, 0, non_intrpt         

interrupt_dispatch:		# Interrupt:                             
	mfc0	$k0, $13	# Get Cause register, again                 
	beq	$k0, 0, done	# handled all outstanding interrupts     

	# TODO: Add interrupts here
	and $a0, $k0, ENERGY_MASK
	bne $a0, 0, energy_interrupt
	
	and $a0, $k0, REQ_PUZ_MASK
	bne $a0, 0, puzzle_interrupt
	
	and	$a0, $k0, BONK_MASK	          
	bne	$a0, 0, bonk_interrupt  

	and $a0, $k0, SMOOSH_MASK
	bne $a0, 0, smooshed_interrupt
	
	and $a0, $k0, TIMER_MASK
	bne $a0, 0, timer_interrupt
	
	li	$v0, PRINT_STRING	# Unhandled interrupt types
	la	$a0, unhandled_str
	syscall 
	j	done

energy_interrupt:
	sw	$a1, ENERGY_ACK		# acknowledge interrupt
	
	# Request Puzzle Interrupt
	la  $t0, puzzle_grid
	la  $t1, REQUEST_PUZZLE
	sw	$t0, 0($t1)
	
	j	interrupt_dispatch

puzzle_interrupt:
	sw	$a1, REQ_PUZ_ACK	# acknowledge interrupt
	
	la  $a0, solve_puzzle
	li  $a1, 1
	sb	$a1, 0($a0)
	
	j	interrupt_dispatch

timer_interrupt:
	sw	$a1, TIMER_ACK		# acknowledge interrupt
	
	la $a0, fruit_data
	sw $a0, SCAN_ADDRESS
	
	lw $a1, 8($a0)			# Fruit in x position
	lw $a2, 12($a0)			# Fruit in y position
	
	lw $v0, FRUIT_Y			# Fruit in last y pos
	lw $a0, FRUIT_X			# Fruit in last x pos
	sub $a0, $a1, $a0 
	sub $a1, $a2, $v0
	
	mtc1	$a0, $f0
	mtc1	$a1, $f1
	vt.s.w $f0, $f0		# convert from ints to floats
	cvt.s.w $f1, $f1
	div.s	$f0, $f1, $f0	# Tan(Theta) = (float) y / (float) x;
	
	cvt.w.s $f0, $f0	# convert "delta" back to integer
	mfc1	$t0, $f6	# Tan(Theta) is now in #t0
	
	sw $t0, TAN_FRUIT
	
	
	li $a0, 0
	sw $a0, TIMER
	
	j	interrupt_dispatch	# see if other interrupts are waiting\
	
smooshed_interrupt:
	sw $a0, FRUIT_SMOOSHED_ACK

	lw $a1, SMOOSHED
	add $a1, $a1, 1			# add one to global variable every interruupt
	sw $a1, SMOOSHED

	j interrupt_dispatch
	
bonk_interrupt:
	lw $a1, SMOOSHED

	# TODO: When should we smash? Now
	
	# Check if there are any caught fruit and if so smash
	li $a2, 5
	bge $a1, $a2, ack
	sw  $a1, FRUIT_SMASH
	sw  $a1, FRUIT_SMASH
	sw  $a1, FRUIT_SMASH
	sw  $a1, FRUIT_SMASH
	add $a1, $a1, -4				# Smash one then decrease smoosh count
	sw  $a1, SMOOSHED
	

ack:
	sw	$a1, BONK_ACK		# acknowledge interrupt

	li	$a0, 0
	sw	$a0, VELOCITY		# drive
	
	j	interrupt_dispatch	# see if other interrupts are waiting

non_intrpt:				# was some non-interrupt
	li	$v0, PRINT_STRING
	la	$a0, non_intrpt_str
	syscall				# print out an error message
	# fall through to done
	j done

done:
	la	$k0, chunkIH
	lw	$a0, 0($k0)		# Restore saved registers
	lw	$a1, 4($k0)
	lw	$a2, 8($k0)
	lw	$v0, 12($k0)
	lw  $t0, 16($k0)
	lw  $t1, 20($k0)
	lw  $ra, 24($k0)
.set noat
	move	$at, $k1	# Restore $at
.set at 
	eret
	