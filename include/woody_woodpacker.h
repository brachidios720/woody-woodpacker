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


// Macros pour accéder aux champs des structures ELF32 ou ELF64
// en fonction de la valeur de is64 dans ElfFile
#define EHDR(elf)  ((elf)->is64 ? (Elf64_Ehdr *)(elf)->ehdr : (Elf32_Ehdr *)(elf)->ehdr)
#define PHDR(elf)  ((elf)->is64 ? (Elf64_Phdr *)(elf)->phdr : (Elf32_Phdr *)(elf)->phdr)
#define SHDR(elf)  ((elf)->is64 ? (Elf64_Shdr *)(elf)->shdr : (Elf32_Shdr *)(elf)->shdr)
#define WEHDR(elf) ((elf)->is64 ? (Elf64_Ehdr *)(elf)->wehdr : (Elf32_Ehdr *)(elf)->wehdr)
#define WPHDR(elf) ((elf)->is64 ? (Elf64_Phdr *)(elf)->wphdr : (Elf32_Phdr *)(elf)->wphdr)

// Macro pour accéder à un programme header spécifique
#define PHDR_IDX(elf, idx) ( (elf)->is64 ? &((Elf64_Phdr *)((elf)->phdr))[idx] : &((Elf32_Phdr *)((elf)->phdr))[idx] )


typedef struct s_elffile {

    int fd;
    int out;
    int stub_fd;

    struct stat st;
    struct stat st_out;
    struct stat st_stub;

    void *map;   // mmap du fichier original
    void *wmap;  // mmap du fichier copié

    void *ehdr;  // pointeur générique vers Elf32_Ehdr ou Elf64_Ehdr
    void *phdr;  // pointeur générique vers Elf32_Phdr ou Elf64_Phdr
    void *shdr;  // pointeur générique vers Elf32_Shdr ou Elf64_Shdr
    void *wehdr; // header du binaire copié
    void *wphdr; // phdr du binaire copié

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

    int is64;   // <-- nouveau champ
} ElfFile;



int open_and_map(const char *path, ElfFile *elf);
int creat_copie_elf(ElfFile *elf);
int encrypt_elf(ElfFile *elf);
int set_stub(ElfFile *elf);
int calcul_stub_position(ElfFile *elf);

#endif
