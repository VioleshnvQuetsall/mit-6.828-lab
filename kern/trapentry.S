/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */


#define T(n) \
.data;                          \
	.long handler##n;          \
.text;                          \
	TRAPHANDLER(handler##n, n)

#define T_NOEC(n) \
.data;                          \
	.long handler##n;          \
.text;                          \
	TRAPHANDLER_NOEC(handler##n, n)

#define T_OC(n) \
.data;           \
	.long 0

.data
	.global handler_entry

handler_entry:
.text
 	T_NOEC(0)
	T_NOEC(1)
	T_OC(2)
	T_NOEC(3)
	T_NOEC(4)
	T_NOEC(5)
	T_NOEC(6)
	T_NOEC(7)
	T(8)
	T_NOEC(9)
	T(10)
	T(11)
	T(12)
	T(13)
	T(14)
	T_OC(15)
	T_NOEC(16)

#define FILL_LONG(begin, end) \
	.data;                    \
		.fill ((end) - (begin) - 1), 4, 0

.data
	FILL_LONG(16, 32)
	T_NOEC(32)
	T_NOEC(33)
	FILL_LONG(33, 36)
	T_NOEC(36)
	FILL_LONG(36, 39)
	T_NOEC(39)
	FILL_LONG(39, 46)
	T_NOEC(46)
	FILL_LONG(46, 51)
	T_NOEC(51)

	TRAPHANDLER_NOEC(handler_syscall, T_SYSCALL)
	TRAPHANDLER_NOEC(handler_default, T_DEFAULT)

/*
 * Lab 3: Your code here for _alltraps
 */

 .text

_alltraps:
	pushl %ds;
	pushl %es;
	pushal;

	movw $GD_KD, %ax;
	movw %ax, %ds;
	movw %ax, %es;

	pushl %esp;
	call trap;