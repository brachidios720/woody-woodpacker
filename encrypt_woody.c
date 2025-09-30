#include "include/woody_woodpacker.h"


int encrypt_elf(ElfFile *elf){

    elf->wmap = mmap(NULL, elf->st_out.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, elf->out, 0);
	if (elf->wmap == MAP_FAILED) {
		perror("mmap output");
		return -1;
	}
    elf->wehdr = EHDR(elf);

	//on doit initialiser les pointeurs wehdr, wphdr et shdr
	//car ils pointent vers des structures ELF32 ou ELF64
	//et on ne peut pas faire de cast direct depuis void*
	//on utilise donc la macro is64 pour savoir si on est en 32 ou 64 bits
	//et on initialise les pointeurs en conséquence

	if(elf->is64)
	{
		elf->wehdr = (Elf64_Ehdr *)elf->wmap;
		elf->wphdr = (Elf64_Phdr *)((char *)elf->wmap + elf->wehdr->e_phoff);
		elf->shdr = (Elf64_Shdr *)((char *)elf->wmap + elf->wehdr->e_shoff);
	} else{
		elf->wehdr = (Elf32_Ehdr *)elf->wmap;
		elf->wphdr = (Elf32_Phdr *)((char *)elf->wmap + elf->wehdr->e_phoff);
		elf->shdr = (Elf32_Shdr *)((char *)elf->wmap + elf->wehdr->e_shoff);
	}

	//initalisation du old_entry
	elf->old_entry = (size_t)elf->wehdr->e_entry;
	// printf("Ancien entrypoint: 0x%lx\n", old_entry);

	//recherche de la section .text
	//permet l acces au section header de woody
	if(elf->is64)
	{
		Elf64_Ehdr *eh = (Elf64_Ehdr *)elf->wehdr;
		Elf64_Shdr *sh = (Elf64_Shdr *)elf->shdr;
		Elf64_Shdr *shstr = &sh[eh->e_shstrndx];
		const char *shstrtab = (char *)elf->wmap + shstr->sh_offset;

		for(int i = 0; i < eh->e_shnum; i++)
		{
			const char *name = shstrtab + sh[i].sh_name;
			if(strcmp(name, ".text") == 0)
			{
				elf->sh_offset = (unsigned long)sh[i].sh_offset;
				elf->sh_addr = (unsigned long)sh[i].sh_addr;
				elf->sh_size = (unsigned long)sh[i].sh_size;
				break ;
			}
		}
	}
	else 
	{
		Elf32_Ehdr *eh = (Elf32_Ehdr *)elf->wehdr;
		Elf32_Shdr *sh = (Elf32_Shdr *)elf->shdr;
		Elf32_Shdr *shstr = &sh[eh->e_shstrndx];
		const char *shstrtab = (char *)elf->wmap + shstr->sh_offset;

		for(int i = 0; i < eh->e_shnum; i++)
		{
			const char *name = shstrtab + sh[i].sh_name;
			if(strcmp(name, ".text") == 0)
			{
				elf->sh_offset = (unsigned long)sh[i].sh_offset;
				elf->sh_addr = (unsigned long)sh[i].sh_addr;
				elf->sh_size = (unsigned long)sh[i].sh_size;
				break ;
			}
		}
	}

	if(elf->sh_offset == 0 || elf->sh_addr == 0 || elf->sh_size == 0)
	{
		printf("Section .text not found\n");
		munmap(elf->wmap, elf->st_out.st_size);
		return -1;
	}

	//chiffrement XOR sur la section .text
	elf->key = 0x42; // clé XOR (exemple, fixe pour le moment)
	unsigned char *p_text = (unsigned char *)elf->wmap + elf->sh_offset;

	for(size_t i = 0; i < elf->sh_size; i++)
	{
		p_text[i] ^= elf->key;  // on chiffre en place
	}

	//sauvegarde sur le disque
	if(msync(elf->wmap, elf->st_out.st_size, MS_SYNC) == -1)
	{
		perror("msync");
		munmap(elf->wmap, elf->st_out.st_size);
		return -1;
	}

	return 0;
}