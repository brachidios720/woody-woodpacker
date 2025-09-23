#include "include/woody_woodpacker.h"


int calcul_stub_position(ElfFile *elf){


    elf->target_idx = -1;

    // Offset dans le fichier où le stub sera injecté
    elf->inject_offset = 0;

    // Parcours de tous les headers de programme pour trouver un segment exécutable PT_LOAD
    for (int i = 0; i < elf->wehdr->e_phnum; i++) {

        // Ignorer les segments qui ne sont pas PT_LOAD
        if (elf->wphdr[i].p_type != PT_LOAD) continue;

        // Ignorer les segments qui ne sont pas exécutables (flags PF_X)
        if ((elf->wphdr[i].p_flags & PF_X) == 0) continue;

        // Offset du segment dans le fichier
        elf->seg_offset = (size_t)elf->wphdr[i].p_offset;

        // Taille actuelle des données présentes dans le fichier pour ce segment
        elf->seg_filesz = (size_t)elf->wphdr[i].p_filesz;

        // Adresse de fin du segment dans le fichier (où commencer l'injection possible)
        elf->seg_end_in_file = elf->seg_offset + elf->seg_filesz;

        // Chercher l'offset du segment PT_LOAD suivant pour déterminer la taille du "gap"
        size_t next_offset = (size_t)elf->st_out.st_size; // par défaut, fin du fichier
        for (int j = 0; j < elf->wehdr->e_phnum; j++) {
            if (elf->wphdr[j].p_type != PT_LOAD) continue; // uniquement PT_LOAD
            size_t other_off = (size_t)elf->wphdr[j].p_offset;
            // garder le plus petit offset supérieur à seg_offset
            if (other_off > elf->seg_offset && other_off < next_offset) next_offset = other_off;
        }

        // Calcul de l'espace libre entre la fin du segment courant et le segment suivant
        ssize_t gap_signed = (ssize_t)next_offset - (ssize_t)elf->seg_end_in_file;
        size_t gap = (gap_signed > 0) ? (size_t)gap_signed : 0; // ne garder que les gaps positifs

        // Vérification si le gap est assez grand pour contenir le stub et si on ne dépasse pas la taille du fichier
        if (gap >= elf->stub_size && elf->seg_end_in_file + elf->stub_size <= (size_t)elf->st_out.st_size) {
            elf->target_idx = i;            // on garde l'index du segment choisi
            elf->inject_offset = elf->seg_end_in_file; // offset exact où injecter le stub
            break;
        }
    }


    if (elf->target_idx == -1) {
        free(elf->stub_bytes);
        munmap(elf->wmap, elf->st_out.st_size);
        close(elf->out);
        munmap(elf->map, elf->st.st_size);
        close(elf->fd);
        return 1;
    }

    elf->wphdr[elf->target_idx].p_flags |= PF_W;

    for (size_t i = 0; i < (size_t)elf->st_stub.st_size - 8; i++) {
        uint64_t *p = (uint64_t *)(elf->stub_bytes + i);

        if (*p == 0x2222222222222222ULL) *p = elf->sh_addr;    // adresse virtuelle .text
        if (*p == 0x3333333333333333ULL) *p = elf->sh_size;    // taille .text
        if (*p == 0x1111111111111111ULL) *p = elf->old_entry;  // ancien entrypoint
        if (*p == 0x4444444444444444ULL){
            uint64_t key64 = (uint64_t)elf->key;   // clé 8 bits étendue
            *p = key64;       // patch clé XOR
        }
    }

    printf("sh_size = %ld\n", elf->sh_size);
    return 0;
}