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

typedef struct {

    int fd;
    int out;
    struct stat st;
    struct stat st_out;
    void *map;
    void *wmap; 
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    Elf64_Ehdr *wehdr;
    Elf64_Phdr *wphdr;
    Elf64_Shdr *shwoody;
    Elf64_Shdr *shstrwoody;
    Elf64_Addr old_entry;
    unsigned long sh_offset;
    unsigned long sh_addr;
    unsigned long sh_size;
    unsigned char key;

} ElfFile;


int open_and_map(const char *path, ElfFile *elf);
int creat_copie_elf(ElfFile *elf);
int encrypt_elf(ElfFile *elf);

#endif
