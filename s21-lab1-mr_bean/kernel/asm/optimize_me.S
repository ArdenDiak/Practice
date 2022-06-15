@		 Tips
@ 1. The code in this state is almost impossible to understand, trying to do
@ 	 is not a good use of your time.
@ 2. Try to do small optimizations that reduce the number of instructions.
@ 3. You have 14 GP registers. You don't need so many stack operations.
@ 4. Once you have reduced the size of the code to a manageable level, you
@    will have to understand it and restructure the code.
@ 5. The programmers manual is you friend, if you are unsure about what an
@    insturction does, consult the manual!
@ 5. HINT: The source code had a lot of things declared volatile, things that
@		 did not have to be volatile. Consider removing unnecessary loads.
@ 6. If you are attempting the bonus, talk to a TA/post privately on piazza 
@    before you do.

	.syntax unified
	.cpu cortex-m4
	.fpu softvfp
	.thumb

	.data
	.align	1
	.type	three, %object
	.size	three, 2
three:
	.short	3
	.align	1
	.type	thirty_three, %object
	.size	thirty_three, 2
thirty_three:
	.short	33
	.align	1
	.type	eleven, %object
	.size	eleven, 2
eleven:
	.short	11
	.align	1
	.type	five, %object
	.size	five, 2
five:
	.short	5
	.align	1
	.type	one, %object
	.size	one, 2
one:
	.short	1
	.align	2
	.type	deadbeef, %object
	.size	deadbeef, 4
deadbeef:
	.word	-559038737
	.text
	.align	2

	.thumb_func
	.type	func, %function
func:
	@ condense the logic of this function, and remember, branches are expensive
	push	{r7}
	sub	sp, sp, #12
	add	r7, sp, #0
	mov	r3, r0
	mov	r2, r1
	strh	r3, [r7, #6]	@ movhi
	mov	r3, r2	@ movhi
	strh	r3, [r7, #4]	@ movhi
	ldrh	r2, [r7, #6]
	
	@ can you maybe do this in just one instruction?
	movw	r3, #:lower16:eleven
	movt	r3, #:upper16:eleven
	ldrh	r3, [r3]	@ movhi
	uxth	r3, r3

	mul	r2, r3, r2
	ldrh	r1, [r7, #4]

	@ same here
	movw	r3, #:lower16:thirty_three
	movt	r3, #:upper16:thirty_three
	ldrh	r3, [r3]	@ movhi
	uxth	r3, r3

	mul	r3, r3, r1
	subs	r3, r2, r3
	mov	r0, r3
	adds	r7, r7, #12
	mov	sp, r7
	@ sp needed
	ldr	r7, [sp], #4
	bx	lr

	.global	optimize_me
	.thumb_func
optimize_me:
	push	{r4, r7, lr}
	sub	sp, sp, #28
	add	r7, sp, #0
	str	r0, [r7, #4]
	ldr	r3, [r7, #4]
	str	r3, [r7, #12]
	movs	r3, #0
	str	r3, [r7, #20]
	b	.L4
.L7:
	@ same here
	movw	r3, #:lower16:one
	movt	r3, #:upper16:one
	ldrh	r3, [r3]	@ movhi
	uxth	r3, r3
	mov	r2, r3

	ldr	r3, [r7, #20]
	ands	r3, r3, r2
	cmp	r3, #0
	bne	.L5

	movw	r3, #:lower16:three
	movt	r3, #:upper16:three
	ldrh	r3, [r3]	@ movhi
	uxth	r2, r3

	ldr	r3, [r7, #20]
	lsls	r3, r3, #1
	ldr	r1, [r7, #4]
	add	r3, r3, r1
	ldr	r1, [r7, #20]
	lsls	r1, r1, #1
	ldr	r0, [r7, #4]
	add	r1, r1, r0
	ldrh	r1, [r1]
	smulbb	r2, r1, r2
	uxth	r2, r2
	strh	r2, [r3]	@ movhi
	b	.L6
.L5:

	movw	r3, #:lower16:five
	movt	r3, #:upper16:five
	ldrh	r3, [r3]	@ movhi
	uxth	r2, r3

	ldr	r3, [r7, #20]
	lsls	r3, r3, #1
	ldr	r1, [r7, #4]
	add	r3, r3, r1
	ldr	r1, [r7, #20]
	lsls	r1, r1, #1
	ldr	r0, [r7, #4]
	add	r1, r1, r0
	ldrh	r1, [r1]
	smulbb	r2, r1, r2
	uxth	r2, r2
	strh	r2, [r3]	@ movhi
.L6:
	ldr	r3, [r7, #20]
	adds	r3, r3, #1
	str	r3, [r7, #20]
.L4:
	ldr	r3, [r7, #20]
	cmp	r3, #500
	blt	.L7
	movs	r3, #0
	str	r3, [r7, #16]
	b	.L8
.L9:
	ldr	r3, [r7, #16]
	lsls	r3, r3, #2
	mov	r2, r3
	ldr	r3, [r7, #4]
	add	r3, r3, r2
	ldrh	r3, [r3]	@ movhi
	strh	r3, [r7, #10]	@ movhi
	ldr	r3, [r7, #16]
	lsls	r3, r3, #2
	adds	r3, r3, #2
	ldr	r2, [r7, #4]
	add	r3, r3, r2
	ldrh	r3, [r3]	@ movhi
	strh	r3, [r7, #8]	@ movhi
	ldr	r3, [r7, #16]
	lsls	r3, r3, #2
	ldr	r2, [r7, #12]
	adds	r4, r2, r3
	ldrh	r2, [r7, #10]
	ldrh	r3, [r7, #8]
	mov	r0, r2
	mov	r1, r3
	bl	func
	mov	r2, r0

	@ loading a 32 bit constant cannot be done in one instruction.
	@ You will require two moves, one for the upper bits and one for the lower
	@ Consider doing this just once and leaving the value in the register.
	movw	r3, #:lower16:deadbeef
	movt	r3, #:upper16:deadbeef
	ldr	r3, [r3]

	add	r3, r3, r2
	str	r3, [r4]
	ldr	r3, [r7, #16]
	adds	r3, r3, #1
	str	r3, [r7, #16]
.L8:
	ldr	r3, [r7, #16]
	cmp	r3, #249
	ble	.L9
	adds	r7, r7, #28
	mov	sp, r7
	pop	{r4, r7, pc}
