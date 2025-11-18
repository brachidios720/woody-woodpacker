#include "include/woody_woodpacker.h"


int set_stub(ElfFile *elf){

    elf->stub_fd = open("stub.bin", O_RDONLY);
    if(elf->stub_fd == -1){
        perror("Open stub");
        return -1;
    }


    if (fstat(elf->stub_fd, &elf->st_stub) < 0)
    {
        perror("fstat");
        close(elf->stub_fd);
        free(elf->key);
        return -1;	
    }

    elf->stub_bytes = malloc(elf->st_stub.st_size);
    if (!elf->stub_bytes)
    {
        close(elf->stub_fd);
        free(elf->key);
        return -1;
    } 
    if (read(elf->stub_fd, elf->stub_bytes, elf->st_stub.st_size) < 0)
    {
        perror("read");
        free(elf->key);
        close(elf->stub_fd);
        return -1;
    }
    close(elf->stub_fd);

    // Taille du stub à injecter dans le binaire
    elf->stub_size = (size_t)elf->st_stub.st_size;

    return 0;
}