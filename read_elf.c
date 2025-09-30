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
        perror("mmap");
        close(elf->fd);
        return -1;
    }


	unsigned char *e_ident = (unsigned char *)elf->map;
    if(e_ident[EI_MAG0] != ELFMAG0 || 
		e_ident[EI_MAG1] != ELFMAG1 ||
        e_ident[EI_MAG2] != ELFMAG2 || 
        e_ident[EI_MAG3] != ELFMAG3){
    
        fprintf(stderr, "not on elf");
        munmap(elf->map, elf->st.st_size);
        close(elf->fd);
        return -1;
    }

	//detection de l'architecture 32 ou 64 bits
	if(e_ident[EI_CLASS] == ELFCLASS64)
	{
		elf->is64 = 1;
		elf->ehdr = (Elf64_Ehdr *)elf->map;
		elf->phdr = (Elf64_Phdr *)((char *)elf->map + ((Elf64_Ehdr *)elf->map)->e_phoff);
		elf->shdr = (Elf64_Shdr *)((char *)elf->map + ((Elf64_Ehdr *)elf->map)->e_shoff);
	} else if (e_ident[EI_CLASS] == ELFCLASS32)
	{
		elf->is64 = 0;
		elf->ehdr = (Elf32_Ehdr *)elf->map;
		elf->phdr = (Elf32_Phdr *)((char *)elf->map + ((Elf32_Ehdr *)elf->map)->e_phoff);
		elf->shdr = (Elf32_Shdr *)((char *)elf->map + ((Elf32_Ehdr *)elf->map)->e_shoff);
	} else {
		fprintf(stderr, "Unknown ELF class\n");
		munmap(elf->map, elf->st.st_size);
		close(elf->fd);
		return -1;
	}
    return 0;
}
