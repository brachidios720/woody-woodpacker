bits 64

section .text

global my_stub ;rendre visible pour le c

my_stub:
    ;le stub ici sert a injecter du code depuis
    ;une nouvelle entrypoint

    ;sauvegardes des registres car sinon impossible de jmp
    push rdi
    push rsi
    push rdx
    push rax


    ;gestion du dechiffrement
    mov rbx, 0x2222222222222222   ; placeholder SEG_ADDR
    mov rcx, 0x3333333333333333   ; placeholder SEG_SIZE
    mov dl,  0x44                 ; placeholder KEY

decrypt_loop:
    cmp rcx, 0
    je decrypt_done
    xor byte [rbx], dl
    inc rbx
    dec rcx
    jmp decrypt_loop

decrypt_done:
    ;ecriture du message woody
    mov rax, 1 ;syscall du write
    mov rdi, 1 ;fd (donc le stdout)

    lea rsi, [rel message]
    mov rdx, message_len
    syscall

    ;restauration des registrers
    pop rax
    pop rdx
    pop rsi
    pop rdi

    mov rax,  0x1111111111111111 ; placeholder pour ancien entrypoint
    jmp rax            ; jump a l'ancien entrypoint apres code effectue


section .data
    message db "....WOODY....", 10
    message_len equ $-message