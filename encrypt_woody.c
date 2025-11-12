#include "include/woody_woodpacker.h"


int encrypt_elf(ElfFile *elf){

    elf->wmap = mmap(NULL, elf->st_out.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, elf->out, 0);
    elf->wehdr = (Elf64_Ehdr *)elf->wmap;
    //on stocke l'entrypoint de woody(qu'on voudra changer)
    elf->old_entry = elf->wehdr->e_entry;
    // printf("Ancien entrypoint: 0x%lx\n", old_entry);

    //on prend la programme header de woody
    elf->wphdr = (Elf64_Phdr *)((char *)elf->wmap + elf->wehdr->e_phoff);

    // mise ne place de la crytp pour rechercher la section .text

    //permet l acces au section header de woody
    elf->shwoody = (Elf64_Shdr *)((char *) elf->wmap + elf->wehdr->e_shoff);

    //permet l acces au nom des section present dans shwoody
    elf->shstrwoody = &elf->shwoody[elf->wehdr->e_shstrndx];
    const char *shstrtabwoody = (char *)elf->wmap + elf->shstrwoody->sh_offset;

    // p_text pointe vers la section .text dans woody
    elf->sh_offset = 0;
    elf->sh_addr = 0;
    elf->sh_size = 0;

    for (int i = 0; i < elf->wehdr->e_shnum; i++){
        
        const char *name = shstrtabwoody + elf->shwoody[i].sh_name;
        if(strcmp(name, ".text") == 0){
            printf(".text: offset=0x%lx addr=0x%lx size= 0x%lx\n", 
                (unsigned long)elf->shwoody[i].sh_offset,
                (unsigned long)elf->shwoody[i].sh_addr,
                (unsigned long)elf->shwoody[i].sh_size);
                elf->sh_offset = (unsigned long)elf->shwoody[i].sh_offset;
                elf->sh_addr = (unsigned long)elf->shwoody[i].sh_addr;
                elf->sh_size = (unsigned long)elf->shwoody[i].sh_size;
            }
    }
    
    // chiffrement en methode XOR
    if(elf->sh_offset != 0 && elf->sh_addr != 0 && elf->sh_size != 0){
        unsigned char *p_text = (unsigned char *)elf->wmap + elf->sh_offset;
        
        for (size_t i = 0; i < elf->sh_size; i++) {

			unsigned char v = p_text[i];

			v = (unsigned char)(v << 3) | (v >> 5); // rotation de 3 bits à gauche
            p_text[i] = v ^ elf->key[i % elf->key_len];  // on chiffre en place
        }
    }
    else{
        perror("encrypt error\n");
        return -1;
    }
    printf(".text chiffré avec la clé: ");
    for (size_t i = 0; i < elf->key_len; ++i) printf("%02X", elf->key[i]);
    printf("\n");

        
    printf("[PATCH] .text addr=0x%lx size=0x%lx old_entry=0x%lx key=",
        elf->sh_addr, elf->sh_size, elf->old_entry);
    for (size_t i = 0; i < elf->key_len; ++i) printf("%02X", elf->key[i]);
    printf("\n");

    //sauvegarder les changements sur le disque
    msync(elf->wmap, elf->st_out.st_size, MS_SYNC);
    return(0);
}