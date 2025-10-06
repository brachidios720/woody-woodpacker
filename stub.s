; ============================================================================
;  woody_woodpacker - stub de déchiffrement XOR (x86_64, Linux)
;  Patcher dans le packer :
;    0x2222222222222222 -> SEG_ADDR  (vaddr début .text chiffrée)
;    0x3333333333333333 -> SEG_SIZE  (taille .text chiffrée)
;    0x4444444444444444 -> KEY_ADDR  (vaddr du tableau clé copié dans stub)
;    0x5555555555555555 -> KEY_LEN   (taille clé, en octets)
;    0x1111111111111111 -> OLD_ENTRY (ancien entrypoint)
;  Hypothèse: le packer a mis le segment PT_LOAD cible en PF_W (écriture autorisée).
; ============================================================================

bits 64
default rel

section .text
global my_stub

my_stub:
    ; ===== Prologue : sauver l’état CPU =====
    pushfq
    push rax
    push rcx
    push rdx
    push rbx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push rbp

    ; ===== Paramètres patchés par le packer =====
    mov rbx, 0x2222222222222222    ; SEG_ADDR : début de la zone chiffrée (.text)
    mov rcx, 0x3333333333333333    ; SEG_SIZE : taille à déchiffrer (octets)
    mov rsi, 0x4444444444444444    ; KEY_ADDR : adresse (vaddr) du tableau clé
    mov r8,  0x5555555555555555    ; KEY_LEN  : longueur de la clé (octets)

    ; ===== Sécurité =====
    test r8, r8
    jz  .decrypt_done

    ; ===== Déchiffrement XOR (clé multi-octets, wrap) =====
    xor r9, r9                     ; r9 = index clé (0)

.decrypt_loop:
    test rcx, rcx
    jz   .decrypt_done

    mov dl, [rsi + r9]             ; dl = key[r9]
    xor byte [rbx], dl             ; *rbx ^= dl

    inc rbx                        ; avancer dans la zone
    dec rcx                        ; bytes restants --

    inc r9                         ; index clé ++
    cmp r9, r8
    jb  .decrypt_loop              ; si r9 < KEY_LEN, continuer
    xor r9, r9                     ; sinon wrap (r9 = 0)
    jmp .decrypt_loop

.decrypt_done:
    ; ===== Message "....WOODY...." =====
    mov rax, 1                     ; write
    mov rdi, 1                     ; stdout
    lea rsi, [rel message]         ; buffer
    mov rdx, message_len           ; longueur
    syscall

    ; ===== Restauration de l’état CPU =====
    pop rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbx
    pop rdx
    pop rcx
    pop rax
    popfq

    ; ===== Reprendre l’exécution à l’ancien entrypoint =====
    mov rax, 0x1111111111111111    ; OLD_ENTRY (patché par le packer)
    jmp rax

section .rodata
message:     db "....WOODY....", 10
message_len: equ $-message

; ===== Marqueur + espace pour la clé (copiée par le packer) =====
key_marker:  dq 0xAAAAAAAAAAAAAAAA
key_space:   times 16 db 0x00      ; augmente si tu veux accepter des clés plus longues
