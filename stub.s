bits 64

section .text

global my_stub ;rendre visible pour le c

my_stub:
    ;le stub ici sert a injecter du code depuis
  
    ;une nouvelle entrypoint
    push rdi
    push rsi
    push rdx
    push rax

    mov rax, 1 ;syscall du write
    mov rdi, 1 ;fd (donc le stdout)

    lea rsi, [rel message]
    mov rdx, message_len
    syscall

    
    pop rax
    pop rdx
    pop rsi
    pop rdi


    mov rax,  0x1111111111111111 ; placeholder pour ancien entrypoint
    jmp rax            ; jump a l'ancien entrypoint apres code effectue


section .data
    message db "....WOODY....", 10
    message_len equ $-message
