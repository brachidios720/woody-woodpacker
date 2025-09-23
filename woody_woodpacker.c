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

    if(encrypt_elf(&elf) < 0)
        return ENCRYPT_ERROR;

    if(set_stub(&elf) < 0)
        return STUB_SETUP_ERROR;

    if(calcul_stub_position(&elf) < 0);
        return CALCUL_STUB_POSITION_FAIL;
    // injection du stub dans le segment choisi
    memcpy((char *)elf.wmap + elf.inject_offset, elf.stub_bytes, elf.stub_size);

    printf("[DEBUG] inject_offset=0x%zx stub[0..4]=%02x %02x %02x %02x\n",
       elf.inject_offset, elf.stub_bytes[0], elf.stub_bytes[1], elf.stub_bytes[2], elf.stub_bytes[3]);

    //mise a jour de la taille des donnees pour ce segment
    //(pfilesz c'est la taille du segment qui apparait dans readelf)
    elf.wphdr[elf.target_idx].p_filesz += (Elf64_Xword)elf.stub_size;

    if (elf.wphdr[elf.target_idx].p_memsz < elf.wphdr[elf.target_idx].p_filesz)
        elf.wphdr[elf.target_idx].p_memsz = elf.wphdr[elf.target_idx].p_filesz;

    elf.wehdr->e_entry = elf.wphdr[elf.target_idx].p_vaddr + (elf.inject_offset - elf.wphdr[elf.target_idx].p_offset);

    free(elf.stub_bytes);

    printf("Injection OK: seg=%d inject_offset=0x%zx new_entry=0x%lx\n",
           elf.target_idx, elf.inject_offset, (unsigned long)elf.wehdr->e_entry);

    close(elf.fd);
    return 0;
}
