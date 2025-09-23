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

    // patcher le placeholder
    // on cherche l'endroit dans le bin asm pour changer le placeholder
    int found = 0;
    for (int i = 0; i < elf->st_stub.st_size - 8; i++) {
        if (*(uint64_t *)(elf->stub_bytes + i) == 0x1111111111111111ULL) {

            *(uint64_t *)(elf->stub_bytes + i) = elf->old_entry;    
            found = 1;
            break;
        }
    }
    if (!found) {
        free(elf->stub_bytes);
        munmap(elf->wmap, elf->st_out.st_size);
        close(elf->out);
        munmap(elf->map, elf->st.st_size);
        close(elf->fd);
        return -1;
    }

    // Taille du stub à injecter dans le binaire
    elf->stub_size = (size_t)elf->st_stub.st_size;

    printf("stub_size = %ld\n", elf->stub_size);
    return 1;
}