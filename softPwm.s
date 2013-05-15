
/*
 * softPwm.s
 *
 * Created: 2012/10/03 17:06:22
 *  Author: strv
 */ 

 #include <avr/io.h>
 
 .global endPwm
 .func endPwm


 endPwm:
	push r16
 ep0:
	lds	r16,TCNT2
	cpi	r16,126
	brlo ep0

	ldi r16,0
tu0:	
	subi r24, 1		;1         31 cycles total
	brcc ow1		;2  1
	cbi _SFR_IO_ADDR(PORTB),PB2	;   2
	rjmp tu1		;   2
ow1:	
	nop			;1
	nop			;1
	nop			;1
tu1:		

	subi r22, 1		;1
	brcc ow2		;2  1
	cbi _SFR_IO_ADDR(PORTB),PB1	;   2
	rjmp tu2		;   2
ow2:	
	nop			;1
	nop			;1
	nop			;1
tu2:		

	subi r20, 1		;1
	brcc ow3		;2  1
	cbi _SFR_IO_ADDR(PORTB),PB0	;   2
	rjmp tu3		;   2
ow3:	
	nop			;1
	nop			;1
	nop			;1
tu3:		

	subi r18, 1		;1
	brcc ow4		;2  1
	cbi _SFR_IO_ADDR(PORTD),PD7	;   2
	rjmp tu4		;   2
ow4:	
	nop			;1
	nop			;1
	nop			;1
tu4:		

	nop			;cycles for rent
	nop
	nop
	nop

	dec r16		;1
	brne tu0		;2

	pop r16
	ret
 .endfunc