;// file : mmx_zoom.s
;// author : JC Hoelt <jeko@free.fr>
;//
;// history
;// 07/01/2001 : Changing FEMMS to EMMS : slower... but run on intel machines
;//	03/01/2001 : WIDTH and HEIGHT are now variable
;//	28/12/2000 : adding comments to the code, suppress some useless lines
;//	27/12/2000 : reducing memory access... improving performance by 20%
;//		coefficients are now on 1 byte
;//	22/12/2000 : Changing data structure
;//	16/12/2000 : AT&T version
;//	14/12/2000 : unrolling loop
;//	12/12/2000 : 64 bits memory access


.data

thezero:
	.long 0x00000000
	.long 0x00000000


.text

.globl mmx_zoom		;// name of the function to call by C program
.extern coeffs		;// the transformation buffer
.extern expix1,expix2 ;// the source and destination buffer
.extern mmx_zoom_size, zoom_width ;// size of the buffers

.align 16
mmx_zoom:

push %ebp
push %esp

;// initialisation du mm7 à zero
movq (thezero), %mm7

movl zoom_width, %eax
movl $4, %ebx
mull %ebx
movl %eax, %ebp

movl (coeffs), %eax
movl (expix1), %edx
movl (expix2), %ebx
movl $10, %edi
movl mmx_zoom_size, %ecx

.while:
	;// esi <- nouvelle position
	movl (%eax), %esi
	leal (%edx, %esi), %esi

	;// recuperation des deux premiers pixels dans mm0 et mm1
	movq (%esi), %mm0		/* b1-v1-r1-a1-b2-v2-r2-a2 */
	movq %mm0, %mm1			/* b1-v1-r1-a1-b2-v2-r2-a2 */

	;// recuperation des 4 coefficients
	movd 4(%eax), %mm6		/* ??-??-??-??-c4-c3-c2-c1 */
	;// depackage du premier pixel
	punpcklbw %mm7, %mm0	/* 00-b2-00-v2-00-r2-00-a2 */

	movq %mm6, %mm5			/* ??-??-??-??-c4-c3-c2-c1 */
	;// depackage du 2ieme pixel
	punpckhbw %mm7, %mm1	/* 00-b1-00-v1-00-r1-00-a1 */

	;// extraction des coefficients...
	punpcklbw %mm5, %mm6	/* c4-c4-c3-c3-c2-c2-c1-c1 */
	movq %mm6, %mm4			/* c4-c4-c3-c3-c2-c2-c1-c1 */
	movq %mm6, %mm5			/* c4-c4-c3-c3-c2-c2-c1-c1 */

	punpcklbw %mm5, %mm6	/* c2-c2-c2-c2-c1-c1-c1-c1 */
	punpckhbw %mm5, %mm4	/* c4-c4-c4-c4-c3-c3-c3-c3 */

	movq %mm6, %mm3			/* c2-c2-c2-c2-c1-c1-c1-c1 */
	punpcklbw %mm7, %mm6	/* 00-c1-00-c1-00-c1-00-c1 */
	punpckhbw %mm7, %mm3	/* 00-c2-00-c2-00-c2-00-c2 */
	
	;// multiplication des pixels par les coefficients
	pmullw %mm6, %mm0		/* c1*b2-c1*v2-c1*r2-c1*a2 */
	pmullw %mm3, %mm1		/* c2*b1-c2*v1-c2*r1-c2*a1 */
	paddw %mm1, %mm0
	
	;// ...extraction des 2 derniers coefficients
	movq %mm4, %mm5			/* c4-c4-c4-c4-c3-c3-c3-c3 */
	punpcklbw %mm7, %mm4	/* 00-c3-00-c3-00-c3-00-c3 */
	punpckhbw %mm7, %mm5	/* 00-c4-00-c4-00-c4-00-c4 */

	;// recuperation des 2 derniers pixels
	movq (%esi,%ebp), %mm1
	movq %mm1, %mm2
	
	;// depackage des pixels
	punpcklbw %mm7, %mm1
	punpckhbw %mm7, %mm2
	
	;// multiplication pas les coeffs
	pmullw %mm4, %mm1
	pmullw %mm5, %mm2
	
	;// ajout des valeurs obtenues à la valeur finale
	paddw %mm1, %mm0
	paddw %mm2, %mm0

	;// division par 256 = 16+16+16+16, puis repackage du pixel final
	psrlw $8, %mm0
	packuswb %mm7, %mm0
	
	;// passage au suivant
	leal 8(%eax), %eax

	decl %ecx
	;// enregistrement du resultat
	movd %mm0, (%ebx)
	leal 4(%ebx), %ebx

	;// test de fin du tantque
	cmpl $0, %ecx				;// 400x300

jz .fin_while
jmp .while

.fin_while:
emms

pop %esp
pop %ebp

ret                  ;//The End
