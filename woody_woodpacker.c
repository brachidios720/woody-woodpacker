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

        
        
        // Affichage des segments PT_LOAD avec les macros definie dans le .h
    
    if(elf.is64)
    {
        printf("entry = %ld\n", EHDR64(&elf)->e_entry);
        for(int i = 0; i < EHDR64(&elf)->e_phnum; i++)
        {
            if(PHDR64(&elf, i)->p_type == PT_LOAD)
            {
                printf("Segment %d: offset=0x%zx vaddr=0x%lx filesz=0x%zx memsz=0x%zx\n",
                    i,
                    (unsigned long)PHDR64(&elf, i)->p_offset,
                    (unsigned long)PHDR64(&elf, i)->p_vaddr,
                    (unsigned long)PHDR64(&elf, i)->p_filesz,
                    (unsigned long)PHDR64(&elf, i)->p_memsz);
            }
        }
    }
    else if (!elf.is64)
    {
        printf("entry = %d\n", EHDR32(&elf)->e_entry);
        for(int i = 0; i < EHDR32(&elf)->e_phnum; i++)
        {
            if(PHDR32(&elf, i)->p_type == PT_LOAD)
            {
                printf("Segment %d: offset=0x%zx vaddr=0x%lx filesz=0x%zx memsz=0x%zx\n",
                    i,
                    (unsigned long)PHDR32(&elf, i)->p_offset,
                    (unsigned long)PHDR32(&elf, i)->p_vaddr,
                    (unsigned long)PHDR32(&elf, i)->p_filesz,
                    (unsigned long)PHDR32(&elf, i)->p_memsz);
            }
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

    if(calcul_stub_position(&elf) < 0)
        return CALCUL_STUB_POSITION_FAIL;
    // injection du stub dans le segment choisi
    /* injection du stub dans le segment choisi */
    memcpy((char *)elf.wmap + elf.inject_offset, elf.stub_bytes, elf.stub_size);

    /* forcer écriture sur disque de la zone modifiée */
    if (msync(elf.wmap, elf.st_out.st_size, MS_SYNC) == -1) {
        perror("msync after inject");
        /* on continue quand même pour mettre à jour les headers, mais on logue l'erreur */
    }

    printf("[DEBUG] inject_offset=0x%zx stub[0..4]=%02x %02x %02x %02x\n",
       elf.inject_offset, elf.stub_bytes[0], elf.stub_bytes[1], elf.stub_bytes[2], elf.stub_bytes[3]);

    /* mise a jour de la taille des donnees pour ce segment */
    if (elf.is64) {
        Elf64_Phdr *ph = &((Elf64_Phdr *)elf.wphdr)[elf.target_idx];
        ph->p_filesz += (Elf64_Xword)elf.stub_size;
        if (ph->p_memsz < ph->p_filesz)
            ph->p_memsz = ph->p_filesz;

        /* caster le header avant accès */
        Elf64_Ehdr *eh = (Elf64_Ehdr *)elf.wehdr;
        eh->e_entry = ph->p_vaddr + (Elf64_Off)(elf.inject_offset - ph->p_offset);
    } else {
        Elf32_Phdr *ph = &((Elf32_Phdr *)elf.wphdr)[elf.target_idx];
        ph->p_filesz += (Elf32_Word)elf.stub_size;
        if (ph->p_memsz < ph->p_filesz)
            ph->p_memsz = ph->p_filesz;

        Elf32_Ehdr *eh = (Elf32_Ehdr *)elf.wehdr;
        eh->e_entry = ph->p_vaddr + (Elf32_Off)(elf.inject_offset - ph->p_offset);
    }

    /* synchroniser les changements des headers sur disque */
    if (msync(elf.wmap, elf.st_out.st_size, MS_SYNC) == -1) {
        perror("msync after headers");
        /* on ne return pas d'erreur si on veut quand même finir et cleanup */
    }

    free(elf.stub_bytes);
    elf.stub_bytes = NULL;

    /* affichage du nouvel entrypoint via macros (correctement casté selon format) */
    if (elf.is64) {
        printf("Injection OK: seg=%d inject_offset=0x%zx new_entry=%#lx\n",
               elf.target_idx, elf.inject_offset, (unsigned long)EHDR64(&elf)->e_entry);
    } else {
        printf("Injection OK: seg=%d inject_offset=0x%zx new_entry=%#x\n",
               elf.target_idx, elf.inject_offset, (unsigned int)EHDR32(&elf)->e_entry);
    }

    /* cleanup */
    if (elf.wmap && elf.st_out.st_size)
        munmap(elf.wmap, elf.st_out.st_size);
    if (elf.map && elf.st.st_size)
        munmap(elf.map, elf.st.st_size);
    if (elf.out != -1)
        close(elf.out);
    if (elf.fd != -1)
        close(elf.fd);

    return 0;

}
