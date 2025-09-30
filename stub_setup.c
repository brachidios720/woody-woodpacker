#include "include/woody_woodpacker.h"

int set_stub(ElfFile *elf) {
    const char *stub_file = elf->is64 ? "stub64.bin" : "stub32.bin";
    printf("Using stub file: %s\n", stub_file);

    elf->stub_fd = open(stub_file, O_RDONLY);
    if (elf->stub_fd == -1) {
        perror("open stub");
        return -1;
    }

    if (fstat(elf->stub_fd, &elf->st_stub) < 0) {
        perror("fstat stub");
        close(elf->stub_fd);
        return -1;
    }

    elf->stub_size = (size_t)elf->st_stub.st_size;
    if (elf->stub_size == 0) {
        fprintf(stderr, "stub file is empty\n");
        close(elf->stub_fd);
        return -1;
    }

    elf->stub_bytes = malloc(elf->stub_size);
    if (!elf->stub_bytes) {
        perror("malloc stub_bytes");
        close(elf->stub_fd);
        return -1;
    }

    /* read in a loop to handle short reads */
    size_t to_read = elf->stub_size;
    unsigned char *bufp = elf->stub_bytes;
    while (to_read > 0) {
        ssize_t r = read(elf->stub_fd, bufp, to_read);
        if (r < 0) {
            perror("read stub");
            free(elf->stub_bytes);
            elf->stub_bytes = NULL;
            close(elf->stub_fd);
            return -1;
        }
        if (r == 0) {
            /* EOF reached before expected size */
            fprintf(stderr, "unexpected EOF while reading stub\n");
            free(elf->stub_bytes);
            elf->stub_bytes = NULL;
            close(elf->stub_fd);
            return -1;
        }
        bufp += r;
        to_read -= (size_t)r;
    }

    close(elf->stub_fd);
    elf->stub_fd = -1; /* mark closed */

    /* TODO: vérifier que stub_size rentre bien dans le segment cible,
       et calculer la position d'injection (set elf->stub_size, elf->stub_bytes déjà prêts) */

    return 0;
}
