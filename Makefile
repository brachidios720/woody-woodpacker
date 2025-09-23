# Nom des exécutables
PACKER = woody_woodpacker
SAMPLE = sample

# Répertoires
RES_DIR = resources

# Fichiers sources
PACKER_SRC = woody_woodpacker.c \
			read_elf.c \
			creat_elf.c \
			encrypt_woody.c 

SAMPLE_SRC = $(RES_DIR)/sample.c

ASM_SRC = stub.s
ASM_OBJ = stub.o

# Compilateurs et flags
CC = gcc
NASM = nasm
CFLAGS = -Wall -Wextra -Werror -g3 -no-pie
NASMFLAGS = -f elf64

# Règle par défaut
all: $(PACKER) $(SAMPLE)

# Compilation du packer
$(PACKER): $(PACKER_SRC) $(ASM_OBJ)
	$(CC) $(CFLAGS) -o $(PACKER) $(PACKER_SRC) $(ASM_OBJ)

# Compilation du sample
$(SAMPLE): $(SAMPLE_SRC)
	$(CC) $(CFLAGS) -o $(SAMPLE) $(SAMPLE_SRC)

# Compilation du stub en NASM
$(ASM_OBJ): $(ASM_SRC)
	$(NASM) $(NASMFLAGS) $< -o $@

# Exécution du sample pour test
run: $(SAMPLE)
	./$(SAMPLE)

# Nettoyage
clean:
	rm -f $(PACKER) $(SAMPLE) woody
	rm -f 3woody
	rm -f $(ASM_OBJ)

