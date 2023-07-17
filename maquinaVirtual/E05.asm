            push 1001
            call separa
            add SP, 1
            mov EAX, %001
            mov ECX, 1
            mov EDX, 1
            sys %2

separa:     push BP
            mov BP, SP
            xor AX, AX ;guardo el primer operando
            xor BX, BX ;guardo el segundo operando
            xor FX, FX //reg auxiliar
            push CX //direccion actual
            mov CX, [BP+2] ;en BP+1 esta ret y en +2 esta dir

1erop:      cmp [CX], ' ' ;veo si se termino de cargar el operando
            jz operacion
            mul AX, 10
            add AX, [CX] ;sumo el valor q hay en la direccion CX
            add CX, 1
            jmp 1erop

operacion:  add CX, 1
            mov DX, [CX]
            jmp 2doop
            ;aca va si no es ninguna de las 4 op un error

2doop:      cmp [CX], '/0'
            jz fin
            mul BX, 10
            add BX, [CX] ;sumo el valor q hay en la direccion CX
            add CX, 1
            jmp 2doop

fin:        cmp DX, '+'
            add AX, BX
            cmp DX, '-'
            sub AX, BX
            cmp DX, '*'
            mul AX, BX
            cmp DX, '/'
            div AX, BX
            mov AX, [1]
            pop CX
            mov SP, BP
            pop BP
            ret
