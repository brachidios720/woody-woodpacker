#include "include/woody_woodpacker.h"


static int parse_hex(const char *hex){


    size_t i = 0;
    while(hex[i]){

        if((hex[i] >= 'a' && hex[i] <= 'f') || (hex[i] >= 'A' && hex[i] <= 'F'))
            i++;
        else if((hex[i] >= '0' && hex[i] <= '9'))
            i++;
        else
            return -1;
    }
    return 0;

}

unsigned char *key_trans(const char *hex, ElfFile *elf){


    size_t len = strlen(hex);

    if(parse_hex(hex) == -1)
        return NULL;

    if (len % 2 != 0 || len < 2 || len > 8)
        return NULL;
    
    elf->key_len = len / 2;
    
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