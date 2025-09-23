#include "include/woody_woodpacker.h"



int open_and_map(const char *path, ElfFile *elf){


    elf->fd = open(path, O_RDONLY);
    if(elf->fd == -1)
    {
        perror("open");
        return -1;
    }

    if(fstat(elf->fd, &elf->st) < 0)
    {
        perror("fstat");
        close(elf->fd);
        return -1;
    }

    elf->map = mmap(NULL, elf->st.st_size, PROT_READ, MAP_PRIVATE, elf->fd, 0);
    if(elf->map == MAP_FAILED){
        perror(elf->map);
        close(elf->fd);
        return -1;
    }

     elf->ehdr = (Elf64_Ehdr *)elf->map;

    if(elf->ehdr->e_ident[EI_MAG0] != ELFMAG0 || 
        elf->ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        elf->ehdr->e_ident[EI_MAG2] != ELFMAG2 || 
        elf->ehdr->e_ident[EI_MAG3] != ELFMAG3){
    
        fprintf(stderr, "not on elf");
        munmap(elf->map, elf->st.st_size);
        close(elf->fd);
        return -1;
    }

    if(elf->ehdr->e_ident[EI_CLASS] != ELFCLASS64){
        fprintf(stderr, "wromg elf format");
        munmap(elf->map, elf->st.st_size);
        close(elf->fd);
        return -1;
    }
    return 0;
}
