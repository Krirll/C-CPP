    format ELF64
    public f
    ;число в xmm0, результат нужно тоже вернуть в xmm0
    ;   f = 0
    ;   f1 = 1;
	;	f2 = n = 1;
	;	step = 3;
	;	fac = 1;
	;	addend = (f*step)/fac;
	;	while (fabs(addend) > e) {
	;		sum += addend;
	;		f = f1 + f2;
	;		f1 = f2;
    ;       f2 = f;
	;		step *= 3;
	;		n++;
	;		fac *= n;
	;		addend = (f*step)/fac;
	;	}
    ;dword - 4
	;qword - 8
f:
    ;st0 = step
    ;st1 = f 
    ;st2 = fac
    ;st3 = n
    ;st4 = f1 
    ;st5 = f2 
    ;st6 = sum
    ;st7 = add
    sub RSP, 8+4+4+4 ;выделение памяти
    movsd qword[RSP],xmm0 ;переносим полученное число в стек
    mov dword[RSP+8], 3
    mov dword[RSP+12], 3 ;future step
    mov dword[RSP+16], 1
    fld qword[RSP] ; e
    fldz ;sum = 0
    fld1 ;f2 = 1
    fld1 ;f1 = 1
    fld1 ;fac = 1
    fld1 ;f = 1 
    ;st0(f) = 1, st1(fac) = 1, st2(f1) = 1, st3(f2) = 1, st4(sum) = 0, st5(e) = e
    fild dword[RSP+8] 
    ;st0(step) = 3, st1(f) = 1, st2(fac) = 1, st3(f1) = 1, st4(f2) = 1, st5(sum) = 0, st6(e) = e
    fmul st0, st1
    ;st0(step*f) = 3, st1(f) = 1, st2(fac) = 1, st3(f1) = 1, st4(f2) = 1, st5(sum) = 0, st6(e) = e
    fdiv st0, st2
    ;st0(step*f/fac) = new add, st1(f) = 1, st2(fac) = 1, st3(f1) = 1, st4(f2) = 1, st5(sum) = 0, st6(e) = e
l:
    fcomi st0, st6 ;comparison new add with e
    jb loopend ;if < then jump end, else go
    ;st0(step*f/fac) = new add, st1(f) = 1, st2(fac) = 1, st3(f1) = 1, st4(f2) = 1, st5(sum) = 0, st6(e) = e
    faddp st5, st0
    ;st0(f) = 1, st1(fac) = 1, st2(f1) = 1, st3(f2) = 1, st4(sum) = new add, st5(e) = e
    fsub st0, st0
    ;st0(f) = 0, st1(fac) = 1, st2(f1) = 1, st3(f2) = 1, st4(sum) = new add, st5(e) = e
    fadd st0, st2
    ;st0(f) = 1, st1(fac) = 1, st2(f1) = 1, st3(f2) = 1, st4(sum) = new add, st5(e) = e
    fadd st0, st3
    ;st0(f) = 2, st1(fac) = 1, st2(f1) = 1, st3(f2) = 1, st4(sum) = new add, st5(e) = e 
    fadd st3, st0
    ;st0(f) = 2, st1(fac) = 1, st2(f1) = 1, st3(f2) = 3, st4(sum) = new add, st5(e) = e
    fxch st2
    ;st0(f) = 1, st1(fac) = 1, st2(f1) = 2, st3(f2) = 3, st4(sum) = new add, st5(e) = e
    fsub st0, st0
    ;st0(f) = 0, st1(fac) = 1, st2(f1) = 2, st3(f2) = 3, st4(sum) = new add, st5(e) = e
    fadd st0, st3
    ;st0(f) = 3, st1(fac) = 1, st2(f1) = 2, st3(f2) = 3, st4(sum) = new add, st5(e) = e
    fsub st0, st2
    ;st0(f) = 1, st1(fac) = 1, st2(f1) = 2, st3(f2) = 3, st4(sum) = new add, st5(e) = e
    fxch st2
    ;st0(f) = 2, st1(fac) = 1, st2(f1) = 1, st3(f2) = 3, st4(sum) = new add, st5(e) = e
    fxch st3
    ;st0(f) = 3, st1(fac) = 1, st2(f1) = 1, st3(f2) = 2, st4(sum) = new add, st5(e) = e
    fsub st0, st2
    ;st0(f) = 2, st1(fac) = 1, st2(f1) = 1, st3(f2) = 2, st4(sum) = new add, st5(e) = e
    fxch st3
    ;st0(f) = 2, st1(fac) = 1, st2(f1) = 1, st3(f2) = 2, st4(sum) = new add, st5(e) = e
    ;подготовка для следующего числа фибоначи
    
    fild dword[RSP+16]
    ;st0(fac-inc) = 1, st1(f) = 2, st2(fac) = 1, st3(f1) = 1, st4(f2) = 2, st5(sum) = new add, st6(e) = e
    fld1
    ;st0(int) = 1, st1(fac-inc) = 1, st2(f) = 2, st3(fac) = 1, st4(f1) = 1, st5(f2) = 2, st6(sum) = new add, st7(e) = e
    faddp st1, st0
    ;st0(fac-inc) = 2, st1(f) = 2, st2(fac) = 1, st3(f1) = 1, st4(f2) = 2, st5(sum) = new add, st6(e) = e
    fist dword[RSP+16]
    fmulp st2, st0
    ;st0(f) = 2, st1(fac) = 2, st2(f1) = 1 st3(f2) = 2, st4(sum) = new add, st5(e) = e
    ;подготовка факториала
    
    fild dword[RSP+8] 
    fild dword[RSP+12]
    ;st0(step) = 3, st1(three) = 3, st2(f) = 2, st3(fac) = 2, st4(f1) = 1, st5(f2) = 2, st6(sum) = new add, st7(e) = e
    fmulp st1, st0
    ;st0(step) = 9, st1(f) = 2, st2(fac) = 2, st3(f1) = 1, st4(f2) = 2, st5(sum) = new add, st6(e) = e
    fist dword[RSP+12] ;save step
    ;вычисление степени

    fmul st0, st1
    ;st0(step*f) = 18, st1(f) = 2, st2(fac) = 2, st3(f1) = 1, st4(f2) = 2, st5(sum) = 3, st6(e) = e
    fdiv st0, st2
    ;st0(step*f/fac) = 9, st1(f) = 2, st2(fac) = 2, st3(f1) = 1, st4(f2) = 2, st5(sum) = 3, st6(e) = e
    ;нахождение нового слагаемого
    jmp l ;прыжок в начало цикла

loopend:
    fxch st5
	fstp qword[RSP]
	movsd xmm0, qword[RSP]
	add RSP, 8+4+4+4
    ret