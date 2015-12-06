# syscall constants
PRINT_STRING  = 4

# spimbot constants
VELOCITY      = 0xffff0010
ANGLE         = 0xffff0014
ANGLE_CONTROL = 0xffff0018
BOT_X         = 0xffff0020
BOT_Y         = 0xffff0024
PRINT_INT     = 0xffff0080
OTHER_BOT_X   = 0xffff00a0
OTHER_BOT_Y   = 0xffff00a4

SCAN_X        = 0xffff0050
SCAN_Y        = 0xffff0054
SCAN_RADIUS   = 0xffff0058
SCAN_ADDRESS  = 0xffff005c
SCAN_MASK     = 0x2000
SCAN_ACK      = 0xffff0064

SMASH_ADDRESS = 0xffff0068

BONK_MASK     = 0x1000
BONK_ACK      = 0xffff0060

SMOOSH_MASK   = 0x2000
SMOOSH_ACK    = 0xffff0064

ENERGY_MASK   = 0x4000
ENERGY_ACK    = 0xffff00c4

REQ_PUZ_MASK  = 0x800
REQ_PUZ_ACK   = 0xffff00d8

TIMER         = 0xffff001c
TIMER_MASK    = 0x8000
TIMER_ACK     = 0xffff006c

.text
main:
	# go wild
	# the world is your oyster
	jr	$ra
