/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.
   Execute MIPS syscalls in bare mode, when running on MIPS systems.
   Copyright (c) 1990-2010, James R. Larus.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the James R. Larus nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include <QPainter>

#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef _WIN32
#include <io.h>
#endif

#include "spim.h"
#include "string-stream.h"
#include "inst.h"
#include "reg.h"
#include "mem.h"
#include "sym-tbl.h"
#include "syscall.h"
#include "../SpimBOT/robot.h"

/* The address of the last exception. Different from EPC
 * if one exception occurs inside another or an interrupt. */
mem_addr last_exception_addr;

/* Decides which syscall to execute or simulate.  Returns zero upon
   exit syscall and non-zero to continue execution. */

int
do_syscall (int context)
{
  /* Syscalls for the source-language version of SPIM.  These are easier to
     use than the real syscall and are portable to non-MIPS operating
     systems. */

  if (spimbot_tournament)
     return 1;

  reg_image_t &reg_image = reg_images[context];
  mem_image_t &mem_image = mem_images[context];

  switch (reg_image.R[REG_V0])
    {
    case PRINT_INT_SYSCALL:
      write_output (console_out, "%d", reg_image.R[REG_A0]);
      break;

    case PRINT_FLOAT_SYSCALL:
      {
	float val = FPR_S (reg_image, REG_FA0);

	write_output (console_out, "%.8f", val);
	break;
      }

    case PRINT_DOUBLE_SYSCALL:
      write_output (console_out, "%.18g", reg_image.FPR[REG_FA0 / 2]);
      break;

    case PRINT_STRING_SYSCALL:
      write_output (console_out, "%s", mem_reference (context, reg_image.R[REG_A0]));
      break;

    case READ_INT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	reg_image.R[REG_RES] = atol (str);
	break;
      }

    case READ_FLOAT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	FPR_S (reg_image, REG_FRES) = (float) atof (str);
	break;
      }

    case READ_DOUBLE_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	reg_image.FPR [REG_FRES] = atof (str);
	break;
      }

    case READ_STRING_SYSCALL:
      {
	read_input ( (char *) mem_reference (context, reg_image.R[REG_A0]), reg_image.R[REG_A1]);
	mem_image.data_modified = true;
	break;
      }

    case SBRK_SYSCALL:
      {
	mem_addr x = mem_image.data_top;
	expand_data (context, reg_image.R[REG_A0]);
	reg_image.R[REG_RES] = x;
	mem_image.data_modified = true;
	break;
      }

    case PRINT_CHARACTER_SYSCALL:
      write_output (console_out, "%c", reg_image.R[REG_A0]);
      break;

    case READ_CHARACTER_SYSCALL:
      {
	static char str [2];

	read_input (str, 2);
	if (*str == '\0') *str = '\n';      /* makes xspim = spim */
	reg_image.R[REG_RES] = (long) str[0];
	break;
      }

    case EXIT_SYSCALL:
      robots[context].done = 1;
      spim_return_value = 0;
      return !robots[!context].done;

    case EXIT2_SYSCALL:
      robots[context].done = 1;
      spim_return_value = reg_image.R[REG_A0];	/* value passed to spim's exit() call */
      return !robots[!context].done;

    case OPEN_SYSCALL:
      {
#ifdef _WIN32
        reg_image.R[REG_RES] = _open((char*)mem_reference (context, reg_image.R[REG_A0]), reg_image.R[REG_A1], reg_image.R[REG_A2]);
#else
    reg_image.R[REG_RES] = open((char*)mem_reference (context, reg_image.R[REG_A0]), reg_image.R[REG_A1], reg_image.R[REG_A2]);
#endif
	break;
      }

    case READ_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (context, reg_image.R[REG_A1] + reg_image.R[REG_A2] - 1);
#ifdef _WIN32
	reg_image.R[REG_RES] = _read(reg_image.R[REG_A0], mem_reference (context, reg_image.R[REG_A1]), reg_image.R[REG_A2]);
#else
	reg_image.R[REG_RES] = read(reg_image.R[REG_A0], mem_reference (context, reg_image.R[REG_A1]), reg_image.R[REG_A2]);
#endif
	mem_image.data_modified = true;
	break;
      }

    case WRITE_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (context, reg_image.R[REG_A1] + reg_image.R[REG_A2] - 1);
#ifdef _WIN32
	reg_image.R[REG_RES] = _write(reg_image.R[REG_A0], mem_reference (context, reg_image.R[REG_A1]), reg_image.R[REG_A2]);
#else
	reg_image.R[REG_RES] = write(reg_image.R[REG_A0], mem_reference (context, reg_image.R[REG_A1]), reg_image.R[REG_A2]);
#endif
	break;
      }

    case CLOSE_SYSCALL:
      {
#ifdef _WIN32
	reg_image.R[REG_RES] = _close(reg_image.R[REG_A0]);
#else
	reg_image.R[REG_RES] = close(reg_image.R[REG_A0]);
#endif
	break;
      }

  case PRINT_HEX_SYSCALL:
    write_output (console_out, "%x", reg_image.R[REG_A0]);
    break;

    default:
      run_error ("Unknown system call: %d\n", reg_image.R[REG_V0]);
      break;
    }

  return (1);
}


void
handle_exception (reg_image_t &reg_image)
{
  if (spimbot_tournament && CP0_ExCode(reg_image) != ExcCode_Int) {
    robots[reg_image.context].done = 1;
    return;
  }

  if (!quiet && CP0_ExCode(reg_image) != ExcCode_Int)
    error ("Exception occurred at PC=0x%08x\n", last_exception_addr);

  reg_image.exception_occurred = 0;
  reg_image.PC = EXCEPTION_ADDR;

  switch (CP0_ExCode(reg_image))
    {
    case ExcCode_Int:
      break;

    case ExcCode_AdEL:
      if (!quiet)
	error ("  Unaligned address in inst/data fetch: 0x%08x\n", CP0_BadVAddr(reg_image));
      break;

    case ExcCode_AdES:
      if (!quiet)
	error ("  Unaligned address in store: 0x%08x\n", CP0_BadVAddr(reg_image));
      break;

    case ExcCode_IBE:
      if (!quiet)
	error ("  Bad address in text read: 0x%08x\n", CP0_BadVAddr(reg_image));
      break;

    case ExcCode_DBE:
      if (!quiet)
	error ("  Bad address in data/stack read: 0x%08x\n", CP0_BadVAddr(reg_image));
      break;

    case ExcCode_Sys:
      if (!quiet)
	error ("  Error in syscall\n");
      break;

    case ExcCode_Bp:
      reg_image.exception_occurred = 0;
      return;

    case ExcCode_RI:
      if (!quiet)
	error ("  Reserved instruction execution\n");
      break;

    case ExcCode_CpU:
      if (!quiet)
	error ("  Coprocessor unuable\n");
      break;

    case ExcCode_Ov:
      if (!quiet)
	error ("  Arithmetic overflow\n");
      break;

    case ExcCode_Tr:
      if (!quiet)
	error ("  Trap\n");
      break;

    case ExcCode_FPE:
      if (!quiet)
	error ("  Floating point\n");
      break;

    default:
      if (!quiet)
	error ("Unknown exception: %d\n", CP0_ExCode(reg_image));
      break;
    }
}
