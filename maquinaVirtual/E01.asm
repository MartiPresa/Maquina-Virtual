            xor EBX, EBX  ;inicializo en 0, aca voy a guardad la cantidad de numeros ingresados
            mov [2], 0  ;inicializo en 0, aca voy a guardad la suma de los numeros ingresados
ingresa:    mov EAX, %001
            mov ECX, 1
            mov EDX, 1
            sys %1
            cmp [1], 0
            jn calcula
            add EBX, 1
            add [2], [1]
            jmp ingresa
calcula:    cmp EBX, 0
            jz fin
            div [2], EBX
fin:        mov EDX, 2
            sys %2
            stop



