.data

# movement memory-mapped I/O
VELOCITY            = 0xffff0010
ANGLE               = 0xffff0014
ANGLE_CONTROL       = 0xffff0018

# coordinates memory-mapped I/O
BOT_X               = 0xffff0020
BOT_Y               = 0xffff0024

# planet memory-mapped I/O
PLANETS_REQUEST     = 0xffff1014

# scanning memory-mapped I/O
SCAN_REQUEST        = 0xffff1010
SCAN_SECTOR         = 0xffff101c

# gravity memory-mapped I/O
FIELD_STRENGTH      = 0xffff1100

# bot info memory-mapped I/O
SCORES_REQUEST      = 0xffff1018
ENERGY              = 0xffff1104

# debugging memory-mapped I/O
PRINT_INT           = 0xffff0080

# interrupt constants
SCAN_MASK           = 0x2000
SCAN_ACKNOWLEDGE    = 0xffff1204
ENERGY_MASK         = 0x4000
ENERGY_ACKNOWLEDGE  = 0xffff1208
INTERFERENCE_MASK   = 0x8000
WHETHER_INTERFERENCE= 0xffff1300

# sector constants
SECTOR_HALF         = 18
SECTOR_SIZE         = 37
SECTORS_PER_ROW     = 8
NUM_SECTORS         = 64

# strategy constants
TEST_FIELD_STRENGTH = 10
CHASE_VELOCITY      = 4
DUST_WAIT_CYCLES    = 10000
RELEASE_CYCLES      = 1000

# global variables
.lcomm scan_done, 4
.lcomm interference_detect, 4
.lcomm interference_interrupt, 4
.lcomm dust, 256
.lcomm planets, 32
.lcomm dust_x, 4
.lcomm dust_y, 4

.text

# align ################################################################
#
# aligns a bot in a coordinate, busy waiting till complete
# argument $a0: function to call to get target coordinate
# argument $a1: 0 for aligning in X, 1 for aligning in Y

align:
	sub	$sp, $sp, 16
	sw	$ra, 0($sp)
	sw	$s0, 4($sp)
	sw	$s1, 8($sp)
	sw	$s2, 12($sp)

	move	$s0, $a0		# coordinate function
	mul	$s1, $a1, 4		# addressing int arrays
	mul	$s2, $a1, 90		# base angle (0 for X, 90 for Y)

align_loop:
	jalr	$s0			# $v0 = target coordinate
	lw	$t0, BOT_X($s1)		# bot coordinate
	beq	$v0, $t0, align_done

	slt	$t0, $v0, $t0		# target above or to the left
	mul	$t0, $t0, 180		# flip bot if needed
	add	$t0, $s2, $t0
	sw	$t0, ANGLE
	li	$t0, 1
	sw	$t0, ANGLE_CONTROL
	j	align_loop

align_done:
	lw	$ra, 0($sp)
	lw	$s0, 4($sp)
	lw	$s1, 8($sp)
	lw	$s2, 12($sp)
	add	$sp, $sp, 16
	jr	$ra


# coordinate getters for align #########################################
#

get_150:
    add $v0, $0, 150
    jr  $ra

get_200:
    add $v0, $0, 200
    jr  $ra

get_dust_x:
	lw	$v0, dust_x
	jr	$ra

get_dust_y:
	lw	$v0, dust_y
	jr	$ra

get_planet_x:
	la	$t0, planets
	sw	$t0, PLANETS_REQUEST
	lw	$v0, 0($t0)
	jr	$ra

get_planet_y:
	la	$t0, planets
	sw	$t0, PLANETS_REQUEST
	lw	$v0, 4($t0)
	jr	$ra


# find_best_sector #####################################################
#
# scans all sections for the one with the most dust
# return $v0: the sector ID

find_best_sector:
	li	$t0, 0			# current sector to scan
	la	$a0, dust		# array to store into
	move	$t1, $a0		# current array position
	li	$v0, -1			# current best sector
	li	$v1, -1			# current best dust amount

fbs_loop:
	sw	$zero, scan_done
	sw	$t0, SCAN_SECTOR
	sw	$a0, SCAN_REQUEST

fbs_wait:
	lw	$t2, scan_done
	beq	$t2, 0, fbs_wait

	lw	$t2, 0($t1)		# dust in scanned sector
	ble	$t2, $v1, fbs_next	# if not better than current best
	move	$v0, $t0		# new best sector
	move	$v1, $t2		# new best dust amount

fbs_next:
	add	$t0, $t0, 1
	add	$t1, $t1, 4
	blt	$t0, 64, fbs_loop

	jr	$ra


# get_sector_center ####################################################
#
# gets the center coordinates for a sector
# argument $a0: the sector number
# return $v0: sector center X
# return $v1: sector center Y

get_sector_center:
	rem	$v0, $a0, SECTORS_PER_ROW
	mul	$v0, $v0, SECTOR_SIZE
	add	$v0, $v0, SECTOR_HALF

	div	$v1, $a0, SECTORS_PER_ROW
	mul	$v1, $v1, SECTOR_SIZE
	add	$v1, $v1, SECTOR_HALF

	jr	$ra


# pick_up_dust #########################################################
#
# gets the bot to pick up dust by traveling to it and activating field
# assumes dust_x and dust_y have been set correctly

pick_up_dust:
	sub	$sp, $sp, 4
	sw	$ra, 0($sp)

	li	$t0, 10
	sw	$t0, VELOCITY
	la	$a0, get_dust_x
	li	$a1, 0
	jal	align
	la	$a0, get_dust_y
	li	$a1, 1
	jal	align

	sw	$zero, VELOCITY
	li	$t0, TEST_FIELD_STRENGTH
	sw	$t0, FIELD_STRENGTH
	li	$t0, 0

pud_wait:
	add	$t0, $t0, 1
	blt	$t0, DUST_WAIT_CYCLES, pud_wait

	lw	$ra, 0($sp)
	add	$sp, $sp, 4
	jr	$ra


# chase_planet #########################################################
#
# chases the bot's planet, returning when it's reached

chase_planet:
	sub	$sp, $sp, 8
	sw	$ra, 0($sp)
	sw	$s0, 4($sp)

	li	$t0, CHASE_VELOCITY
	sw	$t0, VELOCITY
	li	$s0, 0

cp_loop:
	la	$a0, get_planet_x
	li	$a1, 0
	jal	align
	la	$a0, get_planet_y
	li	$a1, 1
	jal	align

	jal	get_planet_x
	lw	$t0, BOT_X
	bne	$v0, $t0, cp_loop
	jal	get_planet_y
	lw	$t0, BOT_Y
	bne	$v0, $t0, cp_loop
	add	$s0, $s0, 1
	blt	$s0, RELEASE_CYCLES, cp_loop

	lw	$ra, 0($sp)
	lw	$s0, 4($sp)
	add	$sp, $sp, 8
	jr	$ra


# main #################################################################
#

main:
	sub	$sp, $sp, 8
	sw	$ra, 0($sp)
    sw  $s0, 4($sp)

	sw	$zero, VELOCITY
	li	$t0, INTERFERENCE_MASK
	or	$t0, $t0, 1
	mtc0	$t0, $12

    sw  $0,  scan_done

	li	$t0, TEST_FIELD_STRENGTH
	sw	$t0, FIELD_STRENGTH
    add $s0, $0, 0

loop:
    add $a1, $0, 1
    beq $s0, $0, go_loc1
    la  $a0, get_200
    jal align
    add $s0, $0, 0
    j   int_testing
go_loc1:
    la  $a0, get_150
    jal align
    add $s0, $0, 1
int_testing:
    la  $t0, interference_detect
    #sw  $t0, WHETHER_INTERFERENCE
    beq $t0, $0, loop1
    add $t0, $t0, 0                     # break here to detect when
                                        # interference is detected
loop1:
    lw  $t1, interference_interrupt
    bne $t1, 0xffff, loop
    sw  $0,  interference_interrupt     # break here to detect when
                                        # interference interrupt is given
    j	loop

	lw	$ra, 0($sp)
    lw  $s0, 4($sp)
	add	$sp, $sp, 8
	jr	$ra


# interrupt handler ####################################################
#

.ktext 0x80000180
interrupt_handler:
	lui	$k0, 0xffff
	sw	$zero, 0x1304($k0)
	# gp-addressed, so no $at use
	sw	$k0, interference_interrupt
	eret
