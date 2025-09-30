#include "include/woody_woodpacker.h"


int set_stub(ElfFile *elf){

	const char *stub_file = elf->is64 ? "stub64.bin" : "stub32.bin";
	printf("Using stub file: %s\n", stub_file);



    elf->stub_fd = open(stub_file, O_RDONLY);
    if(elf->stub_fd == -1){
        perror("Open stub");
        return -1;
    }
	if (fstat(elf->stub_fd, &elf->st_stub) < 0) {
		perror("fstat stub");
		close(elf->stub_fd);
		return -1;
	}


	elf->stub_size = (size_t)elf->st_stub.st_size;
    elf->stub_bytes = malloc(elf->stub_size);
    if (!elf->stub_bytes)
	{
		close(elf->stub_fd)
        return -1;
	} 

	if(read(elf->stub_fd, elf->stub_bytes, elf->st_stub.st_size) != elf->st_stub.st_size) {
		perror("Read stub");
		free(elf->stub_bytes);
		close(elf->stub_fd);
		return -1;
	}
    close(elf->stub_fd);


    // Taille du stub à injecter dans le binaire
}