bits 64

section .text
global my_stub

my_stub:
    ; Sauvegarde des registres importants
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi

    ; -- Déchiffrement XOR --
    mov rbx, 0x22222222   ; placeholder SEG_ADDR (.text addr)
    mov rcx, 0x33333333   ; placeholder SEG_SIZE (.text size)
    mov r12, 0x44444444   ; placeholder KEY (8 bits suffisent)
    mov dl, r12b                  ; clé XOR dans dl

decrypt_loop:
    cmp rcx, 0
    je decrypt_done
    xor byte [rbx], dl
    inc rbx
    dec rcx
    jmp decrypt_loop

decrypt_done:
    ; -- Message "....WOODY...." --
    mov rax, 1          ; syscall: write
    mov rdi, 1          ; fd = stdout
    lea rsi, [rel message]
    mov rdx, message_len
    syscall

    ; -- Restauration des registres --
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; -- Jump vers l'ancien entrypoint --
    mov rax, 0x11111111   ; placeholder OLD_ENTRY
    jmp rax

section .data
    message db "....WOODY....", 10
    message_len equ $-message

