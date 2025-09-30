#include "include/woody_woodpacker.h"


int calcul_stub_position(ElfFile *elf){
	elf->target_idx = -1;
	elf->inject_offset = 0;

	size_t phnum = EHDR(elf)->e_phnum;
	size_t file_end  = (size_t)elf->st_out.st_size;


if (elf->is64) {
        Elf64_Phdr *wph = (Elf64_Phdr *)elf->wphdr;
        for (size_t i = 0; i < phnum; i++) {
            Elf64_Phdr *p = &wph[i];
            if (p->p_type != PT_LOAD) continue;
            if ((p->p_flags & PF_X) == 0) continue; // pas exécutable

            size_t p_offset = (size_t)p->p_offset;
            size_t p_filesz = (size_t)p->p_filesz;
            size_t seg_end = p_offset + p_filesz;

            // trouver le prochain offset PT_LOAD > p_offset
            size_t next_off = file_end;
            for (size_t j = 0; j < phnum; j++) {
                Elf64_Phdr *p2 = &wph[j];
                size_t off2 = (size_t)p2->p_offset;
                if (off2 > p_offset && off2 < next_off) next_off = off2;
            }

            ssize_t gap_signed = (ssize_t)next_off - (ssize_t)seg_end;
            size_t gap = gap_signed > 0 ? (size_t)gap_signed : 0;

            if (gap >= elf->stub_size && seg_end + elf->stub_size <= file_end) {
                elf->target_idx = (int)i;
                elf->inject_offset = seg_end;
                break;
            }
        }

        if (elf->target_idx == -1) {
            fprintf(stderr, "No suitable 64-bit segment found for stub injection\n");
            return -1;
        }

        // rendre le segment writable dans la copie
        wph[elf->target_idx].p_flags |= PF_W;

        // patcher le stub (placeholders 64-bit)
        for (size_t off = 0; off + sizeof(uint64_t) <= elf->stub_size; off++) {
            uint64_t *p64 = (uint64_t *)(elf->stub_bytes + off);
            if (*p64 == 0x2222222222222222ULL) *p64 = (uint64_t)elf->sh_addr;
            else if (*p64 == 0x3333333333333333ULL) *p64 = (uint64_t)elf->sh_size;
            else if (*p64 == 0x1111111111111111ULL) *p64 = (uint64_t)elf->old_entry;
            else if (*p64 == 0x4444444444444444ULL) *p64 = (uint64_t)elf->key;
        }
    } else {
        /* ELF32 branch */
        Elf32_Phdr *wph32 = (Elf32_Phdr *)elf->wphdr;
        for (size_t i = 0; i < phnum; i++) {
            Elf32_Phdr *p = &wph32[i];
            if (p->p_type != PT_LOAD) continue;
            if ((p->p_flags & PF_X) == 0) continue;

            size_t p_offset = (size_t)p->p_offset;
            size_t p_filesz = (size_t)p->p_filesz;
            size_t seg_end = p_offset + p_filesz;

            size_t next_off = file_end;
            for (size_t j = 0; j < phnum; j++) {
                Elf32_Phdr *p2 = &wph32[j];
                size_t off2 = (size_t)p2->p_offset;
                if (off2 > p_offset && off2 < next_off) next_off = off2;
            }

            ssize_t gap_signed = (ssize_t)next_off - (ssize_t)seg_end;
            size_t gap = gap_signed > 0 ? (size_t)gap_signed : 0;

            if (gap >= elf->stub_size && seg_end + elf->stub_size <= file_end) {
                elf->target_idx = (int)i;
                elf->inject_offset = seg_end;
                break;
            }
        }

        if (elf->target_idx == -1) {
            fprintf(stderr, "No suitable 32-bit segment found for stub injection\n");
            return -1;
        }

        // rendre le segment writable dans la copie
        wph32[elf->target_idx].p_flags |= PF_W;

        // patcher le stub (placeholders 32-bit)
        for (size_t off = 0; off + sizeof(uint32_t) <= elf->stub_size; off++) {
            uint32_t *p32 = (uint32_t *)(elf->stub_bytes + off);
            if (*p32 == 0x22222222U) *p32 = (uint32_t)elf->sh_addr;
            else if (*p32 == 0x33333333U) *p32 = (uint32_t)elf->sh_size;
            else if (*p32 == 0x11111111U) *p32 = (uint32_t)elf->old_entry;
            else if (*p32 == 0x44444444U) *p32 = (uint32_t)elf->key;
        }
    }

    printf("chosen seg=%d inject_offset=0x%zx sh_addr=0x%lx sh_size=0x%zx old_entry=0x%zx\n",
           elf->target_idx, elf->inject_offset, (unsigned long)elf->sh_addr, elf->sh_size, elf->old_entry);

    return 0;
}