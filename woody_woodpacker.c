#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>


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

    // mise ne place de la crytp pour rechercher la section .text

    //permet l acces au section header de woody
    Elf64_Shdr *shwoody = (Elf64_Shdr *)((char *) wmap + wehdr->e_shoff);

    //permet l acces au nom des section present dans shwoody
    Elf64_Shdr *shstrwoody = &shwoody[wehdr->e_shstrndx];
    const char *shstrtabwoody = (char *)wmap + shstrwoody->sh_offset;


    unsigned char key = 0x42; // clé XOR (exemple, fixe pour le moment)

    // p_text pointe vers la section .text dans woody
    
    
    unsigned long sh_offset = 0;
    unsigned long sh_addr = 0;
    unsigned long sh_size = 0;

    for (int i = 0; i < wehdr->e_shnum; i++){
        
        const char *name = shstrtabwoody + shwoody[i].sh_name;
        if(strcmp(name, ".text") == 0){
            printf(".text: offset=0x%lx addr=0x%lx size= 0x%lx\n", 
                (unsigned long)shwoody[i].sh_offset,
                (unsigned long)shwoody[i].sh_addr,
                (unsigned long)shwoody[i].sh_size);
                sh_offset = (unsigned long)shwoody[i].sh_offset;
                sh_addr = (unsigned long)shwoody[i].sh_addr;
                sh_size = (unsigned long)shwoody[i].sh_size;
            }
    }
    
    // chiffrement en methode XOR
    if(sh_offset != 0 && sh_addr != 0 && sh_size != 0){
        unsigned char *p_text = (unsigned char *)wmap + sh_offset;
        
        for (size_t i = 0; i < sh_size; i++) {
            p_text[i] ^= key;  // on chiffre en place
        }
        printf(".text chiffré avec la clé 0x%x\n", key);
    }
    else
        perror("encrypt error\n");
        
    //sauvegarder les changements sur le disque
    msync(wmap, st_out.st_size, MS_SYNC);

    //on ouvre le stub.bin de la compilation et on lit les octets dedans
    int stub_fd = open("stub.bin", O_RDONLY);
    if(stub_fd == -1){
        perror("Open stub");
        return 1;
    }

    struct stat st_stub;
    fstat(stub_fd, &st_stub);


    unsigned char *stub_bytes = malloc(st_stub.st_size);
    if (!stub_bytes) 
        return 1;
    read(stub_fd, stub_bytes, st_stub.st_size);
    close(stub_fd);

    // patcher le placeholder
    //on cherche l'endroit dans le bin asm pour changer le placeholder
    int found = 0;
    for (int i = 0; i < st_stub.st_size - 8; i++) {
        if (*(uint64_t *)(stub_bytes + i) == 0x1111111111111111ULL) {
            // calculer l'adresse virtuelle réelle de l'ancien entrypoint
            //car le loader mappe aleatoirement l'adresse ou le programme commence
            //Elf64_Phdr *text_seg = NULL;
            // for (int j = 0; j < wehdr->e_phnum; j++) {
            //     if (wphdr[j].p_type == PT_LOAD &&
            //         old_entry >= wphdr[j].p_vaddr &&
            //         old_entry < wphdr[j].p_vaddr + wphdr[j].p_memsz) {
            //         text_seg = &wphdr[j];
            //         break;
            //     }
            *(uint64_t *)(stub_bytes + i) = old_entry;    
            found = 1;
            break;
        }
            // if (!text_seg) {
            //     free(stub_bytes);
            //     munmap(wmap, st_out.st_size);
            //     close(out);
            //     munmap(map, st.st_size);
            //     close(fd);
            //     return 1;
            // }

            // uint64_t old_entry_virtual = old_entry + (text_seg->p_vaddr - text_seg->p_offset);
            // *(uint64_t *)(stub_bytes + i) = old_entry_virtual;
    }
    if (!found) {
        free(stub_bytes);
        munmap(wmap, st_out.st_size);
        close(out);
        munmap(map, st.st_size);
        close(fd);
        return 1;
    }

    // Taille du stub à injecter dans le binaire
    size_t stub_size = (size_t)st_stub.st_size;

    // Index du segment PT_LOAD choisi pour l'injection (-1 = non trouvé)
    int target_idx = -1;

    // Offset dans le fichier où le stub sera injecté
    size_t inject_offset = 0;

    // Parcours de tous les headers de programme pour trouver un segment exécutable PT_LOAD
    for (int i = 0; i < wehdr->e_phnum; i++) {

        // Ignorer les segments qui ne sont pas PT_LOAD
        if (wphdr[i].p_type != PT_LOAD) continue;

        // Ignorer les segments qui ne sont pas exécutables (flags PF_X)
        if ((wphdr[i].p_flags & PF_X) == 0) continue;

        // Offset du segment dans le fichier
        size_t seg_offset = (size_t)wphdr[i].p_offset;

        // Taille actuelle des données présentes dans le fichier pour ce segment
        size_t seg_filesz = (size_t)wphdr[i].p_filesz;

        // Adresse de fin du segment dans le fichier (où commencer l'injection possible)
        size_t seg_end_in_file = seg_offset + seg_filesz;

        // Chercher l'offset du segment PT_LOAD suivant pour déterminer la taille du "gap"
        size_t next_offset = (size_t)st_out.st_size; // par défaut, fin du fichier
        for (int j = 0; j < wehdr->e_phnum; j++) {
            if (wphdr[j].p_type != PT_LOAD) continue; // uniquement PT_LOAD
            size_t other_off = (size_t)wphdr[j].p_offset;
            // garder le plus petit offset supérieur à seg_offset
            if (other_off > seg_offset && other_off < next_offset) next_offset = other_off;
        }

        // Calcul de l'espace libre entre la fin du segment courant et le segment suivant
        ssize_t gap_signed = (ssize_t)next_offset - (ssize_t)seg_end_in_file;
        size_t gap = (gap_signed > 0) ? (size_t)gap_signed : 0; // ne garder que les gaps positifs

        // Vérification si le gap est assez grand pour contenir le stub et si on ne dépasse pas la taille du fichier
        if (gap >= stub_size && seg_end_in_file + stub_size <= (size_t)st_out.st_size) {
            target_idx = i;            // on garde l'index du segment choisi
            inject_offset = seg_end_in_file; // offset exact où injecter le stub
            break;
        }
    }


    if (target_idx == -1) {
        free(stub_bytes);
        munmap(wmap, st_out.st_size);
        close(out);
        munmap(map, st.st_size);
        close(fd);
        return 1;
    }

    // injection du stub dans le segment choisi
    memcpy((char *)wmap + inject_offset, stub_bytes, stub_size);

    //mise a jour de la taille des donnees pour ce segment
    //(pfilesz c'est la taille du segment qui apparait dans readelf)
    wphdr[target_idx].p_filesz += (Elf64_Xword)stub_size;

    if (wphdr[target_idx].p_memsz < wphdr[target_idx].p_filesz)
        wphdr[target_idx].p_memsz = wphdr[target_idx].p_filesz;

    wehdr->e_entry = wphdr[target_idx].p_vaddr + (inject_offset - wphdr[target_idx].p_offset);

    free(stub_bytes);


    printf("Injection OK: seg=%d inject_offset=0x%zx new_entry=0x%lx\n",
           target_idx, inject_offset, (unsigned long)wehdr->e_entry);



    close(fd);
    return 0;
    
}
