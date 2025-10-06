#include "include/woody_woodpacker.h"


unsigned char *key_trans(const char *hex, ElfFile *elf){


    size_t len = strlen(hex);

    if(len % 2 != 0) // cle de taille pair obligatoire 
        return NULL;
    if(len != 8) // maximum 16 bytes
        return NULL;
    
    elf->key_len = len / 2; // division part 2 
    
    unsigned char *bytes = malloc(elf->key_len);
    if(!bytes){

        perror("malloc");
        return NULL; 
    }

    for(size_t i = 0; i < elf->key_len; i++){
        if(sscanf(hex + 2*i, "%2hhx", &bytes[i]) != 1){
            free(bytes);
            return NULL;
        }
    }

    return(bytes);
}