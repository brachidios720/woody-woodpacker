#include "include/woody_woodpacker.h"


int set_stub(ElfFile *elf){

    elf->stub_fd = open("stub.bin", O_RDONLY);
    if(elf->stub_fd == -1){
        perror("Open stub");
        return -1;
    }
    fstat(elf->stub_fd, &elf->st_stub);


    elf->stub_bytes = malloc(elf->st_stub.st_size);
    if (!elf->stub_bytes) 
        return -1;
    read(elf->stub_fd, elf->stub_bytes, elf->st_stub.st_size);
    close(elf->stub_fd);


    // Taille du stub à injecter dans le binaire
    elf->stub_size = (size_t)elf->st_stub.st_size;

    return 0;
}