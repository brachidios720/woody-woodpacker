#include "./include/woody_woodpacker.h"

int encrypt_elf(ElfFile *elf)
{
    elf->wmap = mmap(NULL, elf->st_out.st_size,
                     PROT_READ | PROT_WRITE, MAP_SHARED,
                     elf->out, 0);
    if (elf->wmap == MAP_FAILED) {
        perror("mmap output");
        return -1;
    }

    // initialise le header du binaire copié
    elf->wehdr = elf->wmap;
    elf->wphdr = (char *)elf->wmap + 
                 (elf->is64 ? EHDR64(elf)->e_phoff : EHDR32(elf)->e_phoff);
    elf->shdr  = (char *)elf->wmap +
                 (elf->is64 ? EHDR64(elf)->e_shoff : EHDR32(elf)->e_shoff);

    // old_entry dépend du format
    elf->old_entry = elf->is64 ? (size_t)EHDR64(elf)->e_entry
                               : (size_t)EHDR32(elf)->e_entry;

    // recherche de la section .text
    if (elf->is64) {
        Elf64_Ehdr *eh = EHDR64(elf);
        Elf64_Shdr *sh = (Elf64_Shdr *)elf->shdr;
        Elf64_Shdr *shstr = &sh[eh->e_shstrndx];
        const char *shstrtab = (char *)elf->wmap + shstr->sh_offset;

        for (int i = 0; i < eh->e_shnum; i++) {
            const char *name = shstrtab + sh[i].sh_name;
            if (strcmp(name, ".text") == 0) {
                elf->sh_offset = sh[i].sh_offset;
                elf->sh_addr   = sh[i].sh_addr;
                elf->sh_size   = sh[i].sh_size;
                break;
            }
        }
    } else {
        Elf32_Ehdr *eh = EHDR32(elf);
        Elf32_Shdr *sh = (Elf32_Shdr *)elf->shdr;
        Elf32_Shdr *shstr = &sh[eh->e_shstrndx];
        const char *shstrtab = (char *)elf->wmap + shstr->sh_offset;

        for (int i = 0; i < eh->e_shnum; i++) {
            const char *name = shstrtab + sh[i].sh_name;
            if (strcmp(name, ".text") == 0) {
                elf->sh_offset = sh[i].sh_offset;
                elf->sh_addr   = sh[i].sh_addr;
                elf->sh_size   = sh[i].sh_size;
                break;
            }
        }
    }

    if (elf->sh_offset == 0 || elf->sh_addr == 0 || elf->sh_size == 0) {
        printf("Section .text not found\n");
        munmap(elf->wmap, elf->st_out.st_size);
        return -1;
    }

    // chiffrement XOR
    elf->key = 0x42;
    unsigned char *p_text = (unsigned char *)elf->wmap + elf->sh_offset;

    for (size_t i = 0; i < elf->sh_size; i++) {
        p_text[i] ^= elf->key;
    }

    if (msync(elf->wmap, elf->st_out.st_size, MS_SYNC) == -1) {
        perror("msync");
        munmap(elf->wmap, elf->st_out.st_size);
        return -1;
    }

    return 0;
}
