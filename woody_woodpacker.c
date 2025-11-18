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


    ElfFile elf = {0};

    if (parse_args(ac, av, &elf) != 0)
        return 1;
    
    if(open_and_map(av[1], &elf) < 0)
        return OPEN_AND_READ_ERROR;

    elf.phdr = (Elf64_Phdr *)((char *)elf.map + elf.ehdr->e_phoff);

    if(creat_copie_elf(&elf) < 0)
        return COPY_ERROR;

    if(encrypt_elf(&elf) < 0)
        return ENCRYPT_ERROR;

    if(set_stub(&elf) < 0)
        return STUB_SETUP_ERROR;

    if(calcul_stub_position(&elf) < 0)
        return CALCUL_STUB_POSITION_FAIL;
    // injection du stub dans le segment choisi
    memcpy((char *)elf.wmap + elf.inject_offset, elf.stub_bytes, elf.stub_size);



    //mise a jour de la taille des donnees pour ce segment
    //(pfilesz c'est la taille du segment qui apparait dans readelf)
    elf.wphdr[elf.target_idx].p_filesz += (Elf64_Xword)elf.stub_size;

    if (elf.wphdr[elf.target_idx].p_memsz < elf.wphdr[elf.target_idx].p_filesz)
        elf.wphdr[elf.target_idx].p_memsz = elf.wphdr[elf.target_idx].p_filesz;

    elf.wehdr->e_entry = elf.wphdr[elf.target_idx].p_vaddr + (elf.inject_offset - elf.wphdr[elf.target_idx].p_offset);

    free(elf.stub_bytes);

    close(elf.fd);
    free(elf.key);
    return 0;
}
