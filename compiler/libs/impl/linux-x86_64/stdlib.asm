default rel

global p_print_u64
global p_print_char
global p_print_char_ptr
global p_malloc
global p_free


global d_print_u64_arg
global d_print_char_arg
global d_print_char_ptr_arg
global d_malloc_arg
global d_malloc_ret
global d_free_arg

section .data
    d_print_u64_arg dq 0
    d_print_char_arg db 0
    d_print_char_ptr_arg dq 0
    d_malloc_arg dq 0
    d_malloc_ret dq 0
    d_free_arg dq 0


section .bss
    buffer_22b resb 22

section .text
extern malloc
extern free

p_print_u64:
    mov rax, [d_print_u64_arg]

    test rax, rax
    jne .ppu64_convert

    mov byte [buffer_22b + 20], '0'
    mov byte [buffer_22b + 21], 0x0A 

    mov rax, 1    
    mov rdi, 1    
    lea rsi, [buffer_22b + 20]
    mov rdx, 2      
    syscall
    ret

.ppu64_convert:
    lea rsi, [buffer_22b + 21]
    mov rcx, 10

.ppu64_loop:
    xor rdx, rdx
    div rcx

    add dl, '0'

    dec rsi
    mov [rsi], dl

    test rax, rax
    jne .ppu64_loop

    mov rdx, buffer_22b + 21
    sub rdx, rsi                

    mov byte [rsi + rdx], 0x0A  
    inc rdx                     

    mov rax, 1        
    mov rdi, 1       
    syscall

    ret

p_print_char:
    mov al, [d_print_char_arg]
    mov [buffer_22b], al

    mov rax, 1
    mov rdi, 1
    lea rsi, [buffer_22b]
    mov rdx, 1
    syscall

    ret

p_print_char_ptr:
    mov rsi, [d_print_char_ptr_arg]

    mov rdx, 0

.pcp_len:
    cmp byte [rsi + rdx], 0
    je .pcp_write

    inc rdx
    jmp .pcp_len

.pcp_write:
    mov rax, 1
    mov rdi, 1
    syscall

    ret

;sorry, I'm NOT implementing malloc and free myself
p_malloc:
    mov rdi, [d_malloc_arg]
    call malloc

    mov [d_malloc_ret], rax
    ret

p_free:
    mov rdi, [d_free_arg]
    call free
    ret