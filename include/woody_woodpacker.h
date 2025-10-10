#ifndef WOODY_WOODPACKER_H
#define WOODY_WOODPACKER_H

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/random.h>



# define OPEN_AND_READ_ERROR 10
# define COPY_ERROR 11
# define ENCRYPT_ERROR 12
# define STUB_SETUP_ERROR 13
# define CALCUL_STUB_POSITION_FAIL 14

typedef struct {

    int fd;
    int out;
    int stub_fd;

    struct stat st;
    struct stat st_out;
    struct stat st_stub;

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
    unsigned char *key;

    size_t seg_offset;
    size_t seg_filesz;
    size_t seg_end_in_file;

    unsigned char *stub_bytes;
    size_t stub_size;

    int target_idx;
    size_t inject_offset;
    size_t key_len;

    size_t key_marker;

} ElfFile;


int open_and_map(const char *path, ElfFile *elf);
int creat_copie_elf(ElfFile *elf);
int encrypt_elf(ElfFile *elf);
int set_stub(ElfFile *elf);
int calcul_stub_position(ElfFile *elf);
unsigned char *key_trans(const char *hex, ElfFile *elf);
int parse_args(int ac, char **av, ElfFile *elf);
int generate_random_key(size_t n, unsigned char **buff);

#endif
