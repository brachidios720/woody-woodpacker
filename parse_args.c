#include "include/woody_woodpacker.h"


int generate_random_key(size_t n, unsigned char **buff){

    unsigned char *buf = malloc(n);
    if(!buf)
    {
        perror("malloc key failed");
        return 1;
    }
    ssize_t got = getrandom(buf, n, 0);
    if(got < 0){
        
        int fd = open("/dev/urandom", O_RDONLY);
        if( fd < 0){
            perror("open /dev/urandom");
            free(buf);
            return -1;
        }
        ssize_t rd = read(fd, buf, n);
        close (fd);
        if(rd != (ssize_t)n){
            printf("urandom failed\n");
            free(buf);
            return -1;
        }
    }
    *buff = buf;
    return(0);
}

static void print_key_hex(const unsigned char *k, size_t n) {
    for (size_t i = 0; i < n; ++i) printf("%02X", k[i]);
}

int parse_args(int ac, char **av, ElfFile *elf){


    if(ac < 2){

        printf("error of usage: ./<woody> <sample> <option -k> <key with len of 8>\n");
        return -1;
    }

    if(ac == 2){

        elf->key_len = 4;
        if(generate_random_key(elf->key_len, &elf->key) != 0)
            return -1;
        printf("[KEY] generated random key (%zu bytes): ", elf->key_len);
        print_key_hex(elf->key, elf->key_len);
        printf("\n");
        return 0;
    }

    if(ac == 4 && strcmp(av[2], "-k") == 0){

        elf->key = key_trans(av[3], elf);
        if(!elf->key){
            printf("format key error. The key need 16 caractere in uppercase hexadecimal");
            return 1;
        }
    }
    return(0);
}