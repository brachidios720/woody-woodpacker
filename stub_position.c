#include "include/woody_woodpacker.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>


//pour le calcul des changements de placeholders on regarde octet par octet alors qu'avant on regardait l'alignement ?
int calcul_stub_position(ElfFile *elf) {
    elf->target_idx = -1;
    elf->inject_offset = 0;

    if (!elf || !elf->wphdr) return -1;

    size_t phnum = elf->is64 ? (size_t)EHDR64(elf)->e_phnum : (size_t)EHDR32(elf)->e_phnum;
    if (phnum == 0) return -1;

    size_t file_end = (size_t)elf->st_out.st_size;

    if (elf->is64) {
        Elf64_Phdr *wph = (Elf64_Phdr *)elf->wphdr;

        // Chercher le segment exécutable PT_LOAD avec assez de place
        for (size_t i = 0; i < phnum; i++) {
            Elf64_Phdr *p = &wph[i];
            if (p->p_type != PT_LOAD) continue;
            if ((p->p_flags & PF_X) == 0) continue;

            size_t seg_end = p->p_offset + p->p_filesz;

            size_t next_off = file_end;
            for (size_t j = 0; j < phnum; j++) {
                size_t off2 = (size_t)((Elf64_Phdr *)elf->wphdr)[j].p_offset;
                if (off2 > p->p_offset && off2 < next_off) next_off = off2;
            }

            size_t gap = (next_off > seg_end) ? next_off - seg_end : 0;
            if (gap >= elf->stub_size && seg_end + elf->stub_size <= file_end) {
                elf->target_idx = (int)i;
                elf->inject_offset = seg_end;
                break;
            }
        }

        if (elf->target_idx == -1) {
            fprintf(stderr, "No suitable 64-bit segment found\n");
            return -1;
        }

        wph[elf->target_idx].p_flags |= PF_W;

        // Patch du stub octet par octet
        uint64_t placeholders[4] = {
            0x1111111111111111ULL, // OLD_ENTRY
            0x2222222222222222ULL, // SEG_ADDR
            0x3333333333333333ULL, // SEG_SIZE
            0x4444444444444444ULL  // KEY
        };
        uint64_t replacements[4] = {
            elf->old_entry,
            elf->sh_addr,
            elf->sh_size,
            elf->key
        };

        for (size_t ph = 0; ph < 4; ph++) {
            unsigned char pat[8], rep[8];
            for (int b = 0; b < 8; b++) {
                pat[b] = (placeholders[ph] >> (8*b)) & 0xFF;
                rep[b] = (replacements[ph] >> (8*b)) & 0xFF;
            }
            for (size_t off = 0; off + 8 <= elf->stub_size; off++) {
                if (memcmp(elf->stub_bytes + off, pat, 8) == 0)
                    memcpy(elf->stub_bytes + off, rep, 8);
            }
        }

    } else {
        // ELF32
        Elf32_Phdr *wph32 = (Elf32_Phdr *)elf->wphdr;

        for (size_t i = 0; i < phnum; i++) {
            Elf32_Phdr *p = &wph32[i];
            if (p->p_type != PT_LOAD) continue;
            if ((p->p_flags & PF_X) == 0) continue;

            size_t seg_end = p->p_offset + p->p_filesz;

            size_t next_off = file_end;
            for (size_t j = 0; j < phnum; j++) {
                size_t off2 = (size_t)((Elf32_Phdr *)elf->wphdr)[j].p_offset;
                if (off2 > p->p_offset && off2 < next_off) next_off = off2;
            }

            size_t gap = (next_off > seg_end) ? next_off - seg_end : 0;
            if (gap >= elf->stub_size && seg_end + elf->stub_size <= file_end) {
                elf->target_idx = (int)i;
                elf->inject_offset = seg_end;
                break;
            }
        }

        if (elf->target_idx == -1) {
            fprintf(stderr, "No suitable 32-bit segment found\n");
            return -1;
        }

        wph32[elf->target_idx].p_flags |= PF_W;

        // Patch du stub octet par octet
        uint32_t placeholders[4] = {
            0x11111111U, // OLD_ENTRY
            0x22222222U, // SEG_ADDR
            0x33333333U, // SEG_SIZE
            0x44444444U  // KEY
        };
        uint32_t replacements[4] = {
            (uint32_t)elf->old_entry,
            (uint32_t)elf->sh_addr,
            (uint32_t)elf->sh_size,
            (uint32_t)elf->key
        };

        for (size_t ph = 0; ph < 4; ph++) {
            unsigned char pat[4], rep[4];
            for (int b = 0; b < 4; b++) {
                pat[b] = (placeholders[ph] >> (8*b)) & 0xFF;
                rep[b] = (replacements[ph] >> (8*b)) & 0xFF;
            }
            for (size_t off = 0; off + 4 <= elf->stub_size; off++) {
                if (memcmp(elf->stub_bytes + off, pat, 4) == 0)
                    memcpy(elf->stub_bytes + off, rep, 4);
            }
        }
    }

    printf("chosen seg=%d inject_offset=0x%zx sh_addr=0x%lx sh_size=0x%zx old_entry=0x%zx\n",
           elf->target_idx, elf->inject_offset, (unsigned long)elf->sh_addr, elf->sh_size, elf->old_entry);

    return 0;
}
