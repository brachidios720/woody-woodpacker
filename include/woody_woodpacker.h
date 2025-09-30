#ifndef WOODY_WOODPACKER_H
#define WOODY_WOODPACKER_H

#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>

# define OPEN_AND_READ_ERROR 10
# define COPY_ERROR 11
# define ENCRYPT_ERROR 12
# define STUB_SETUP_ERROR 13
# define CALCUL_STUB_POSITION_FAIL 14

/*
  NOTE:
  - On évite d'utiliser une expression ternaire qui retourne
    des Elf64_* * et Elf32_* * (car gcc alerterait pointer type mismatch).
  - Fournir des helpers EHDR64/EHDR32, PHDR64/PHDR32 et PHDR_IDX.
  - Utiliser PHDR_IDX(elf, i) dans les boucles pour obtenir un pointeur
    vers le i-ème program header (typed correctement).
*/

/* Accesseurs typés (utiliser en conditionnant sur elf->is64) */
#define EHDR64(elf) ((Elf64_Ehdr *)((elf)->ehdr))
#define EHDR32(elf) ((Elf32_Ehdr *)((elf)->ehdr))

#define PHDR64(elf, i) (&((Elf64_Phdr *)((elf)->phdr))[i])
#define PHDR32(elf, i) (&((Elf32_Phdr *)((elf)->phdr))[i])

#define SHDR64(elf, i) (&((Elf64_Shdr *)((elf)->shdr))[i])
#define SHDR32(elf, i) (&((Elf32_Shdr *)((elf)->shdr))[i])

/* Macro pratique : renvoie l'adresse du program header idx (déjà typée) */
#define PHDR_IDX(elf, idx) ( (elf)->is64 ? PHDR64(elf, idx) : PHDR32(elf, idx) )

/* Macro pratique pour l'ehdr si tu veux un accès rapide pour la taille / phnum:
   mais attention: EHDR_RAW renvoie (void*) — il ne faut pas déréférencer ce type
   sans cast explicite ; préférez EHDR64/EHDR32 dans le code. */
#define EHDR_RAW(elf) ( (void *)((elf)->ehdr) )

typedef struct s_elffile {
    int fd;
    int out;
    int stub_fd;

    struct stat st;
    struct stat st_out;
    struct stat st_stub;

    void *map;   /* mmap du fichier original */
    void *wmap;  /* mmap du fichier copié */

    void *ehdr;  /* pointeur générique vers Elf32_Ehdr ou Elf64_Ehdr */
    void *phdr;  /* pointeur générique vers Elf32_Phdr ou Elf64_Phdr */
    void *shdr;  /* pointeur générique vers Elf32_Shdr ou Elf64_Shdr */
    void *wehdr; /* header du binaire copié (même principe) */
    void *wphdr; /* phdr du binaire copié */

    size_t old_entry;

    unsigned long sh_offset;
    unsigned long sh_addr;
    unsigned long sh_size;
    unsigned char key;

    size_t seg_offset;
    size_t seg_filesz;
    size_t seg_end_in_file;

    unsigned char *stub_bytes;
    size_t stub_size;

    int target_idx;
    size_t inject_offset;

    int is64;
} ElfFile;

int open_and_map(const char *path, ElfFile *elf);
int creat_copie_elf(ElfFile *elf);
int encrypt_elf(ElfFile *elf);
int set_stub(ElfFile *elf);
int calcul_stub_position(ElfFile *elf);

#endif
