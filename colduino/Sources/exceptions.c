/*
 * File:    exceptions.c
 * Purpose: Generic exception handling for ColdFire processors
 *
 */

#include "derivative.h"
#include "exceptions.h"
#include "startcf.h"
#include "BRTOS.h"

#define REGISTER_ABI __REGABI__

/***********************************************************************/
/*
 *  Set NO_PRINTF to 0 in order the exceptions.c interrupt handler
 *  to output messages to the standard io. 
 * 
 */
#define NO_PRINTF    1

#if NO_PRINTF
#define VECTORDISPLAY(MESSAGE)                    asm { nop; };
#define VECTORDISPLAY2(MESSAGE,MESSAGE2)          asm { nop; };
#define VECTORDISPLAY3(MESSAGE,MESSAGE2,MESSAGE3) asm { nop; };
#else
#include <stdio.h>
#define VECTORDISPLAY(MESSAGE1)                    printf(MESSAGE1);
#define VECTORDISPLAY2(MESSAGE1,MESSAGE2)          printf(MESSAGE1,MESSAGE2);
#define VECTORDISPLAY3(MESSAGE1,MESSAGE2,MESSAGE3) printf(MESSAGE1,MESSAGE2,MESSAGE3);
#endif

#ifdef __cplusplus
extern "C"
{
#endif

extern unsigned long far _SP_INIT[];

/***********************************************************************/
/*
 * Handling of the TRK ColdFire libs (printf support in Debugger Terminal) 
 * 
 * To enable this support:  
 * - Set CONSOLE_IO_SUPPORT  1 in this file; this will enable 
 *   TrapHandler_printf for the trap #14 exception.
 *
 * - Make sure the file console_io_cf.c is in your project. 
 *
 * - In the debugger make sure that in the Connection "Setup" dialog, 
 *   "Debug Options" property page, the check box 
 *   "Enable Terminal printf support" is set.
 *   
 *
 * 
 */
#define CONSOLE_IO_SUPPORT  0 

#if CONSOLE_IO_SUPPORT
asm __declspec(register_abi) void TrapHandler_printf(void)
{
	HALT
	RTE
}
#endif                                                   

#if (defined _MCF51MM256_H) || (defined _MCF51JE256_H) || (defined _MCF51JE128_H)

volatile unsigned char usb_int_disable = 0;
volatile unsigned char usb_int_en_copy = 0;

void usb_int_dis(void)
{
	usb_int_disable++;
}

void usb_int_en(void)
{
	volatile unsigned char dummy;
	if (usb_int_disable)
	{
		usb_int_disable--;
	}

	if (INT_ENB == 0)
	{
		if (usb_int_disable == 0)
		{
			dummy = usb_int_en_copy;
			usb_int_en_copy = 0;
			INT_ENB = dummy;
		}
	}
}

#endif /* (defined _MCF51MM256_H) || (defined _MCF51JE256_H) */

/***********************************************************************/
/*
 * This is the handler for all exceptions which are not common to all 
 * ColdFire Chips.  
 *
 * Called by mcf_exception_handler
 * 
 */
void derivative_interrupt(unsigned long vector)
{
	if (vector < 64 || vector > 192)
	{
		VECTORDISPLAY2("User Defined Vector #%d\n", vector);
	}
}

/***********************************************************************
 *
 * This is the exception handler for all  exceptions common to all 
 * chips ColdFire.  Most exceptions do nothing, but some of the more 
 * important ones are handled to some extent.
 *
 * Called by asm_exception_handler 
 *
 * The ColdFire family of processors has a simplified exception stack
 * frame that looks like the following:
 *
 *              3322222222221111 111111
 *              1098765432109876 5432109876543210
 *           8 +----------------+----------------+
 *             |         Program Counter         |
 *           4 +----------------+----------------+
 *             |FS/Fmt/Vector/FS|      SR        |
 *   SP -->  0 +----------------+----------------+
 *
 * The stack self-aligns to a 4-byte boundary at an exception, with
 * the FS/Fmt/Vector/FS field indicating the size of the adjustment
 * (SP += 0,1,2,3 bytes).
 *             31     28 27      26 25    18 17      16 15                                  0
 *           4 +---------------------------------------+------------------------------------+
 *             | Format | FS[3..2] | Vector | FS[1..0] |                 SR                 |
 *   SP -->  0 +---------------------------------------+------------------------------------+
 */
#define MCF5XXX_RD_SF_FORMAT(PTR)   \
   ((*((unsigned short *)(PTR)) >> 12) & 0x00FF)

#define MCF5XXX_RD_SF_VECTOR(PTR)   \
   ((*((unsigned short *)(PTR)) >>  2) & 0x00FF)

#define MCF5XXX_RD_SF_FS(PTR)    \
   ( ((*((unsigned short *)(PTR)) & 0x0C00) >> 8) | (*((unsigned short *)(PTR)) & 0x0003) )

#define MCF5XXX_SF_SR(PTR)    *(((unsigned short *)(PTR))+1)

#define MCF5XXX_SF_PC(PTR)    *((unsigned long *)(PTR)+1)

#define MCF5XXX_EXCEPTFMT     "%s -- PC = %#08X\n"

void mcf_exception_handler(void *framepointer)
{
	volatile unsigned long exceptionStackFrame =
			(*(unsigned long *) (framepointer));
	volatile unsigned short stackFrameSR = MCF5XXX_SF_SR(framepointer);
	volatile unsigned short stackFrameWord =
			(*(unsigned short *) (framepointer));
	volatile unsigned long stackFrameFormat =
			(unsigned long) MCF5XXX_RD_SF_FORMAT(&stackFrameWord);
	volatile unsigned long stackFrameFS =
			(unsigned long) MCF5XXX_RD_SF_FS(&stackFrameWord);
	volatile unsigned long stackFrameVector =
			(unsigned long) MCF5XXX_RD_SF_VECTOR(&stackFrameWord);
	volatile unsigned long stackFramePC = MCF5XXX_SF_PC(framepointer);

	switch (stackFrameFormat)
	{
	case 4:
	case 5:
	case 6:
	case 7:
		break;
	default:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Illegal stack type", stackFramePC);
		break;
	}

	switch (stackFrameVector)
	{
	case 2:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Access Error", stackFramePC);
		switch (stackFrameFS)
		{
		case 4:
			VECTORDISPLAY("Error on instruction fetch\n");
			break;
		case 8:
			VECTORDISPLAY("Error on operand write\n");
			break;
		case 9:
			VECTORDISPLAY("Attempted write to write-protected space\n");
			break;
		case 12:
			VECTORDISPLAY("Error on operand read\n");
			break;
		default:
			VECTORDISPLAY("Reserved Fault Status Encoding\n");
			break;
		}
		break;
	case 3:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Address Error", stackFramePC);
		switch (stackFrameFS)
		{
		case 4:
			VECTORDISPLAY("Error on instruction fetch\n");
			break;
		case 8:
			VECTORDISPLAY("Error on operand write\n");
			break;
		case 9:
			VECTORDISPLAY("Attempted write to write-protected space\n");
			break;
		case 12:
			VECTORDISPLAY("Error on operand read\n");
			break;
		default:
			VECTORDISPLAY("Reserved Fault Status Encoding\n");
			break;
		}
		break;
	case 4:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Illegal instruction", stackFramePC);
		break;
	case 8:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Privilege violation", stackFramePC);
		break;
	case 9:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Trace Exception", stackFramePC);
		break;
	case 10:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Unimplemented A-Line Instruction",
				stackFramePC);
		break;
	case 11:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Unimplemented F-Line Instruction",
				stackFramePC);
		break;
	case 12:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Debug Interrupt", stackFramePC);
		break;
	case 14:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Format Error", stackFramePC);
		break;
	case 15:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Unitialized Interrupt",
				stackFramePC);
		break;
	case 24:
		VECTORDISPLAY3(MCF5XXX_EXCEPTFMT, "Spurious Interrupt", stackFramePC);
		break;
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
	case 31:
		VECTORDISPLAY2("Autovector interrupt level %d\n",
				stackFrameVector - 24);
		break;
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
		VECTORDISPLAY2("TRAP #%d\n", stackFrameVector - 32);
		break;
	case 5:
	case 6:
	case 7:
	case 13:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 48:
	case 49:
	case 50:
	case 51:
	case 52:
	case 53:
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
	case 59:
	case 60:
	case 61:
	case 62:
	case 63:
		VECTORDISPLAY2("Reserved: #%d\n", stackFrameVector);
		break;
	default:
		derivative_interrupt(stackFrameVector);
		break;
	}
}
#if 0
asm  __declspec(register_abi) void asm_exception_handler(void)
{
   link     a6,#0 
   lea     -20(sp), sp
   movem.l d0-d2/a0-a1, (sp)
   lea     24(sp),a0   /* A0 point to exception stack frame on the stack */
   jsr     mcf_exception_handler
   movem.l (sp), d0-d2/a0-a1
   lea     20(sp), sp
   unlk a6
   rte
}
#endif

#if 1
#pragma TRAP_PROC
void asm_exception_handler(void)
{
	// ************************
	// Entrada de interrupção
	// ************************
	OS_INT_ENTER();

	// Interrupt Handling
	PTBD_PTBD0 = 0;
	PTBDD_PTBDD0 = 1;
	
	DisableInterrupts;
	
	while (1)
	{
		__RESET_WATCHDOG();
	};

	// ************************
	// Interrupt Exit
	// ************************
	OS_INT_EXIT();
	// ************************
}
#endif

typedef void (*vectorTableEntryType)(void);

#if CONSOLE_IO_SUPPORT
vectorTableEntryType vector_printf @Vtrap14 = TrapHandler_printf;
#endif

/*
 *  MCF51JE128 vector table
 *  CF V1 has 114 vector + SP_INIT in the vector table (115 entries)
 */

extern void TickTimer(void);
extern void SwitchContext(void);
extern void USB_ISR(void);
extern void Analog_comparator(void);

#include "uart.h"
#if (ENABLE_UART1 == TRUE)
extern void uart1_tx(void);
extern void uart1_rx(void);
extern void uart1_error(void);
#endif

#if (ENABLE_UART2 == TRUE)
extern void uart2_tx(void);
extern void uart2_rx(void);
extern void uart2_error(void);
#endif 

extern void Timer2Overflow(void);

#if (Vusb != 0x00000120)
#warning "Install the latest MM256 Compiler Patch !!!. Vusb!=0x120 !!!"
#endif 

#ifndef VReserved67
/* MM256 Compiler patch not applied */
#define VReserved67   0x0000010C
#undef  Vusb
#define Vusb          0x00000120

#endif /* VReserved67 */

__declspec(weak)   vectorTableEntryType vector_0 @INITSP
= (vectorTableEntryType) &_SP_INIT;
__declspec(weak)   vectorTableEntryType vector_1 @INITPC
= (vectorTableEntryType) &_startup;
__declspec(weak)   vectorTableEntryType vector_2 @Vaccerr = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_3 @Vadderr = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_4 @Viinstr = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_5 @VReserved5 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_6 @VReserved6 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_7 @VReserved7 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_8 @Vprviol = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_9 @Vtrace = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_10 @Vunilaop = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_11 @Vunilfop = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_12 @Vdbgi = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_13 @VReserved13
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_14 @Vferror = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_15 @VReserved15
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_16 @VReserved16
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_17 @VReserved17
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_18 @VReserved18
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_19 @VReserved19
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_20 @VReserved20
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_21 @VReserved21
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_22 @VReserved22
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_23 @VReserved23
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_24 @Vspuri = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_25 @VReserved25
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_26 @VReserved26
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_27 @VReserved27
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_28 @VReserved28
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_29 @VReserved29
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_30 @VReserved30
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_31 @VReserved31
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_32 @Vtrap0 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_33 @Vtrap1 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_34 @Vtrap2 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_35 @Vtrap3 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_36 @Vtrap4 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_37 @Vtrap5 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_38 @Vtrap6 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_39 @Vtrap7 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_40 @Vtrap8 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_41 @Vtrap9 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_42 @Vtrap10 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_43 @Vtrap11 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_44 @Vtrap12 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_45 @Vtrap13 = asm_exception_handler;
#if CONSOLE_IO_SUPPORT == 0
__declspec(weak)   vectorTableEntryType vector_46 @Vtrap14 = SwitchContext;
#endif
__declspec(weak)   vectorTableEntryType vector_47 @Vtrap15 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_48 @VReserved48
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_49 @VReserved49
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_50 @VReserved50
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_51 @VReserved51
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_52 @VReserved52
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_53 @VReserved53
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_54 @VReserved54
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_55 @VReserved55
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_56 @VReserved56
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_57 @VReserved57
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_58 @VReserved58
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_59 @VReserved59
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_60 @VReserved60
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_61 @Vunsinstr = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_62 @VReserved62
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_63 @VReserved63
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_64 @Virq = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_65 @Vlvd = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_66 @Vlol = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_67 @VReserved67
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_68 @Vpdb = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_69 @Vdac = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_70 @Vspi1 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_71 @Vadc = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_72 @Vusb = asm_exception_handler; //USB_ISR;
__declspec(weak)   vectorTableEntryType vector_73 @Vtpm1ch0 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_74 @Vtpm1ch1 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_75 @Vtpm1ch2 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_76 @Vtpm1ch3 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_77 @Vtpm1ovf = TickTimer;
__declspec(weak)   vectorTableEntryType vector_78 @Vspi2 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_79 @Vcmt = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_80 @Vtpm2ch0 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_81 @Vtpm2ch1 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_82 @Vtpm2ch2 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_83 @Vtpm2ch3 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_84 @Vtpm2ovf = Timer2Overflow;
__declspec(weak)   vectorTableEntryType vector_85 @Viic = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_86 @Vpracmp = Analog_comparator;
#if (ENABLE_UART1 == TRUE)
__declspec(weak)   vectorTableEntryType vector_87 @Vsci1err = uart1_error;
__declspec(weak)   vectorTableEntryType vector_88 @Vsci1rx = uart1_rx;
__declspec(weak)   vectorTableEntryType vector_89 @Vsci1tx = uart1_tx;
#else
__declspec(weak)   vectorTableEntryType vector_87 @Vsci1err = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_88 @Vsci1rx = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_89 @Vsci1tx = asm_exception_handler;
#endif
#if (ENABLE_UART2 == TRUE)
__declspec(weak)   vectorTableEntryType vector_90 @Vsci2err = uart2_error;
__declspec(weak)   vectorTableEntryType vector_91 @Vsci2rx = uart2_rx;
__declspec(weak)   vectorTableEntryType vector_92 @Vsci2tx = uart2_tx;
#else
__declspec(weak)   vectorTableEntryType vector_90 @Vsci2err = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_91 @Vsci2rx = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_92 @Vsci2tx = asm_exception_handler;
#endif
__declspec(weak)   vectorTableEntryType vector_93 @VReserved93
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_94 @VReserved94
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_95 @Vkbi1 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_96 @Vkbi2 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_97 @Vtod = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_98 @VReserved98
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_99 @VReserved99
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_100 @VReserved100
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_101 @VReserved101
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_102 @VReserved102
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_103 @VL7swi = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_104 @VL6swi = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_105 @VL5swi = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_106 @VL4swi = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_107 @VL3swi = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_108 @VL2swi = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_109 @VL1swi = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_110 @VReserved110
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_111 @VReserved111
= asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_112 @Vftsr1 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_113 @Vftsr2 = asm_exception_handler;
__declspec(weak)   vectorTableEntryType vector_114 @VReserved114
= asm_exception_handler;

#ifdef __cplusplus
}
#endif

