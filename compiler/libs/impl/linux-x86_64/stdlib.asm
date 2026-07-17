global p_print_u64
global d_print_u64_arg

section .data
    d_print_u64_arg dq 0

section .bss
    buffer resb 22

section .text

p_print_u64:
    mov rax, [d_print_u64_arg]

    test rax, rax
    jne .convert

    mov byte [buffer + 20], '0'
    mov byte [buffer + 21], 0x0A 

    mov rax, 1    
    mov rdi, 1    
    lea rsi, [buffer + 20]
    mov rdx, 2      
    syscall
    ret

.convert:
    lea rsi, [buffer + 21]
    mov rcx, 10

.loop:
    xor rdx, rdx
    div rcx

    add dl, '0'

    dec rsi
    mov [rsi], dl

    test rax, rax
    jne .loop

    mov rdx, buffer + 21
    sub rdx, rsi                

    mov byte [rsi + rdx], 0x0A  
    inc rdx                     

    mov rax, 1        
    mov rdi, 1       
    syscall

    ret