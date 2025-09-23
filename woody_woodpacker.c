#include "include/woody_woodpacker.h"
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

    ElfFile elf;

    if(open_and_map(av[1], &elf) < 0)
        return OPEN_AND_READ_ERROR;

    printf("entry = %ld\n", elf.ehdr->e_entry);

    elf.phdr = (Elf64_Phdr *)((char *)elf.map + elf.ehdr->e_phoff);

    for (int i = 0; i < elf.ehdr->e_phnum; i++) {
        if (elf.phdr[i].p_type == PT_LOAD) {
            printf("Segment %d: offset=0x%lx vaddr=0x%lx memsz=0x%lx filesz=0x%lx\n",
                   i,
                (unsigned long)elf.phdr[i].p_offset,
                (unsigned long)elf.phdr[i].p_vaddr,
                (unsigned long)elf.phdr[i].p_memsz,
                (unsigned long)elf.phdr[i].p_filesz);
        }
    }

    if(creat_copie_elf(&elf) < 0)
        return COPY_ERROR;
    //struct stat st_out;
    fstat(elf.out, &elf.st_out);

    if(encrypt_elf(&elf) < 0){
        return ENCRYPT_ERROR;
    }
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
    // on cherche l'endroit dans le bin asm pour changer le placeholder
    int found = 0;
    for (int i = 0; i < st_stub.st_size - 8; i++) {
        if (*(uint64_t *)(stub_bytes + i) == 0x1111111111111111ULL) {

            *(uint64_t *)(stub_bytes + i) = elf.old_entry;    
            found = 1;
            break;
        }
    }
    if (!found) {
        free(stub_bytes);
        munmap(elf.wmap, elf.st_out.st_size);
        close(elf.out);
        munmap(elf.map, elf.st.st_size);
        close(elf.fd);
        return 1;
    }

    // Taille du stub à injecter dans le binaire
    size_t stub_size = (size_t)st_stub.st_size;

    printf("stub_size = %ld\n", stub_size);
    // Index du segment PT_LOAD choisi pour l'injection (-1 = non trouvé)
    int target_idx = -1;

    // Offset dans le fichier où le stub sera injecté
    size_t inject_offset = 0;

    // Parcours de tous les headers de programme pour trouver un segment exécutable PT_LOAD
    for (int i = 0; i < elf.wehdr->e_phnum; i++) {

        // Ignorer les segments qui ne sont pas PT_LOAD
        if (elf.wphdr[i].p_type != PT_LOAD) continue;

        // Ignorer les segments qui ne sont pas exécutables (flags PF_X)
        if ((elf.wphdr[i].p_flags & PF_X) == 0) continue;

        // Offset du segment dans le fichier
        size_t seg_offset = (size_t)elf.wphdr[i].p_offset;

        // Taille actuelle des données présentes dans le fichier pour ce segment
        size_t seg_filesz = (size_t)elf.wphdr[i].p_filesz;

        // Adresse de fin du segment dans le fichier (où commencer l'injection possible)
        size_t seg_end_in_file = seg_offset + seg_filesz;

        // Chercher l'offset du segment PT_LOAD suivant pour déterminer la taille du "gap"
        size_t next_offset = (size_t)elf.st_out.st_size; // par défaut, fin du fichier
        for (int j = 0; j < elf.wehdr->e_phnum; j++) {
            if (elf.wphdr[j].p_type != PT_LOAD) continue; // uniquement PT_LOAD
            size_t other_off = (size_t)elf.wphdr[j].p_offset;
            // garder le plus petit offset supérieur à seg_offset
            if (other_off > seg_offset && other_off < next_offset) next_offset = other_off;
        }

        // Calcul de l'espace libre entre la fin du segment courant et le segment suivant
        ssize_t gap_signed = (ssize_t)next_offset - (ssize_t)seg_end_in_file;
        size_t gap = (gap_signed > 0) ? (size_t)gap_signed : 0; // ne garder que les gaps positifs

        // Vérification si le gap est assez grand pour contenir le stub et si on ne dépasse pas la taille du fichier
        if (gap >= stub_size && seg_end_in_file + stub_size <= (size_t)elf.st_out.st_size) {
            target_idx = i;            // on garde l'index du segment choisi
            inject_offset = seg_end_in_file; // offset exact où injecter le stub
            break;
        }
    }


    if (target_idx == -1) {
        free(stub_bytes);
        munmap(elf.wmap, elf.st_out.st_size);
        close(elf.out);
        munmap(elf.map, elf.st.st_size);
        close(elf.fd);
        return 1;
    }

    elf.wphdr[target_idx].p_flags |= PF_W;

    for (size_t i = 0; i < (size_t)st_stub.st_size - 8; i++) {
        uint64_t *p = (uint64_t *)(stub_bytes + i);

        if (*p == 0x2222222222222222ULL) *p = elf.sh_addr;    // adresse virtuelle .text
        if (*p == 0x3333333333333333ULL) *p = elf.sh_size;    // taille .text
        if (*p == 0x1111111111111111ULL) *p = elf.old_entry;  // ancien entrypoint
        if (*p == 0x4444444444444444ULL){
            uint64_t key64 = (uint64_t)elf.key;   // clé 8 bits étendue
            *p = key64;       // patch clé XOR
        }
    }

    printf("sh_size = %ld\n", elf.sh_size);

    // injection du stub dans le segment choisi
    memcpy((char *)elf.wmap + inject_offset, stub_bytes, stub_size);

    printf("[DEBUG] inject_offset=0x%zx stub[0..4]=%02x %02x %02x %02x\n",
       inject_offset, stub_bytes[0], stub_bytes[1], stub_bytes[2], stub_bytes[3]);

    //mise a jour de la taille des donnees pour ce segment
    //(pfilesz c'est la taille du segment qui apparait dans readelf)
    elf.wphdr[target_idx].p_filesz += (Elf64_Xword)stub_size;

    if (elf.wphdr[target_idx].p_memsz < elf.wphdr[target_idx].p_filesz)
        elf.wphdr[target_idx].p_memsz = elf.wphdr[target_idx].p_filesz;

    elf.wehdr->e_entry = elf.wphdr[target_idx].p_vaddr + (inject_offset - elf.wphdr[target_idx].p_offset);

    free(stub_bytes);

    printf("Injection OK: seg=%d inject_offset=0x%zx new_entry=0x%lx\n",
           target_idx, inject_offset, (unsigned long)elf.wehdr->e_entry);

    close(elf.fd);
    return 0;
}
