#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>



// readelf -h sample     # affiche le header ELF
// readelf -S sample     # affiche les sections (dont .text)
// readelf -l sample     # affiche les segments

int main(int ac, char **av){


    if(ac != 2 )
    {
        fprintf(stderr, "Wrong number of arguments", av[0]);
        return 1;
    }

    int fd = open(av[1], O_RDONLY);
    if(fd == -1)
    {
        perror("open");
        return 1;
    }

    struct stat st;
    if(fstat(fd, &st) < 0)
    {
        perror("fstat");
        close(fd);
        return 1;
    }


 

}