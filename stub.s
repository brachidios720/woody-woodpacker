bits 64

section .text

global my_stub ;rendre visible pour le c

my_stub:
    ;le stub ici sert a injecter du code depuis
    ;une nouvelle entrypoint

    mov rax, 0x1111111111111111 ;ancien entrypoint
    jmp rax            ; jump a l'ancien entrypoint apres code effectue
