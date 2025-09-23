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
    return 0;
}

