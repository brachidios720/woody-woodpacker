#include "include/woody_woodpacker.h"


int creat_copie_elf(ElfFile *elf){

    elf->out = open("3woody",O_RDWR | O_CREAT | O_TRUNC , 0755);
    if(elf->out == -1){
        perror("open woody");
        return -1;
    }

    if(write(elf->out, elf->map, elf->st.st_size) != elf->st.st_size){
        perror("write woody");
        close(elf->out);
        return -1;
    }
    if (fstat(elf->out, &elf->st_out) == -1) {
        perror("fstat out");
        close(elf->out);
        return -1;
    }

    printf("[DEBUG22222] Copied %ld bytes into 3woody\n", elf->st_out.st_size);
    return 0;
}

