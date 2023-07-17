AUX		EQU 	-1
mov eax,10
mov [eax+aux],1

mov ax,%001
mov edx, 15
mov ecx, 1
sys %2

