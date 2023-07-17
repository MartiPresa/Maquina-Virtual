            xor BX, BX
            xor CX, CX
            mov EAX, %001
            mov ECX 2
            mov EDX, 1
            sys %1

            mov BX, [1]
            mov CX, [2]
            xor AX, AX
            cmp BX, CX
            jz divisible
            jn fin
            mov AX, 1

calcula:    sub BX, CX  ;le resto al que se esta dividiendo, lo q se divide . Si es mayor a 0 es pq sigue calculando
            add AX, 1
            cmp BX, CX
            jp calcula
            cmp BX, CX  ;innecesario
            jz divisible ; resto = 0
            mov DX, BX
            jmp fin

divisible:  add AX, 1

 fin:       mov [1], AX
            mov EAX, %001
            mov ECX 1
            mov EDX, 1
            sys %2
            stop
