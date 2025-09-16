#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>


extern void my_stub();


// readelf -h sample     # affiche le header ELF
// readelf -S sample     # affiche les sections (dont .text)
// readelf -l sample     # affiche les segments
// objdump -D mon_programme | less # Désassembler le binaire entier
// objdump -d -j .text mon_programme | less # Désassembler seulement .text (le code) :
// hexdump -C mon_programme | less # Voir le fichier brut en hexadécimal (octet par octet).Pratique pour vérifier si ton stub a bien été écrit à la fin du fichier. Hexdump classique :
// xxd mon_programme | less # Avec xxd (souvent plus lisible) :


int main(int ac, char **av){


    if(ac != 2 )
    {
        fprintf(stderr, "Wrong number of arguments %s\n", av[0]);
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

    void *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(map == MAP_FAILED){
        perror(map);
        close(fd);
        return 1;
    }

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)map;

    if(ehdr->e_ident[EI_MAG0] != ELFMAG0 || 
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || 
        ehdr->e_ident[EI_MAG3] != ELFMAG3){
    
        fprintf(stderr, "not on elf");
        munmap(map, st.st_size);
        close(fd);
        return 1;
    }

    if(ehdr->e_ident[EI_CLASS] != ELFCLASS64){
        fprintf(stderr, "wromg elf format");
        munmap(map, st.st_size);
        close(fd);
        return 1;
    }


    //Elf64_Addr old_entry = ehdr->e_entry;

    Elf64_Phdr *phdr = (Elf64_Phdr *)((char *)map + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            printf("Segment %d: offset=0x%lx vaddr=0x%lx memsz=0x%lx filesz=0x%lx\n",
                   i,
                (unsigned long)phdr[i].p_offset,
                (unsigned long)phdr[i].p_vaddr,
                (unsigned long)phdr[i].p_memsz,
                (unsigned long)phdr[i].p_filesz);
        }
    }

    int out = open("3woody",O_RDWR | O_CREAT | O_TRUNC , 0755);
    if(out == -1){
        perror("open woody");
        return 1;
    }

    if(write(out, map, st.st_size) != st.st_size){
        perror("write woody");
        close(out);
        return 1;
    }

    struct stat st_out;
    fstat(out, &st_out);


    //wmap c'est le pointeur qui pointe sur le programme header de woody mappe en memoire
    void *wmap = mmap(NULL, st_out.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, out, 0);
    Elf64_Ehdr *wehdr = (Elf64_Ehdr *)wmap;
    //on stocke l'entrypoint de woody(qu'on voudra changer)
    Elf64_Addr old_entry = wehdr->e_entry;
    printf("Ancien entrypoint: 0x%lx\n", old_entry);

    //on prend la programme header de woody
    Elf64_Phdr *wphdr = (Elf64_Phdr *)((char *)wmap + wehdr->e_phoff);

    //on calcule le nouvel entrypoint
    Elf64_Addr new_entry = 0;

    for(int i = 0; i < wehdr->e_phnum; i++){
        if(wphdr[i].p_type == PT_LOAD){
            //chopper la fin du segment .text ou on veut rajouter des octets
            new_entry = wphdr[i].p_vaddr + wphdr[i].p_filesz;
            //new entry pointe vers l'endroit ou on veut mettre le stub
        }
    }

    //on defini mnt ou on veut l'entrypoint
    //avant c'etait sur le .text
    //Maintenant c'est sur adresse virtuelle de la ou on va mettre le stub
    wehdr->e_entry = new_entry;
    printf("Nouveau entrypoint: 0x%lx\n", new_entry);

    //sauvegarder les changements sur le disque
    msync(wmap, st_out.st_size, MS_SYNC);
    //vider la memoire de wmap
    munmap(wmap, st_out.st_size);

    my_stub();

    

    close(fd);
    return 0;
    
}
