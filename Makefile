# Nom des exécutables
PACKER = woody_woodpacker
SAMPLE = sample

# Répertoires
RES_DIR = resources

# Fichiers sources
PACKER_SRC = woody_woodpacker.c \
			read_elf.c \
			creat_elf.c \
			encrypt_woody.c \
			stub_setup.c \
			stub_position.c \
			key_check.c \
			parse_args.c

SAMPLE_SRC = $(RES_DIR)/sample.c

ASM_SRC = stub.s
ASM_BIN = stub.bin

# Compilateurs et flags
CC = gcc
NASM = nasm
CFLAGS = -Wall -Wextra -Werror -g3 -no-pie
NASMFLAGS = -f bin

# Règle par défaut
all: $(ASM_BIN) $(PACKER) $(SAMPLE)

# Compilation du stub en binaire
$(ASM_BIN): $(ASM_SRC)
	$(NASM) $(NASMFLAGS) $< -o $@

# Compilation du packer (ne linke pas le stub)
$(PACKER): $(PACKER_SRC)
	$(CC) $(CFLAGS) -o $(PACKER) $(PACKER_SRC)

# Compilation du sample
$(SAMPLE): $(SAMPLE_SRC)
	$(CC) $(CFLAGS) -o $(SAMPLE) $(SAMPLE_SRC)

# Exécution du sample pour test
run: $(SAMPLE)
	./$(SAMPLE)

# Nettoyage
clean:
	rm -f $(ASM_BIN)

# Nettoyage complet
fclean: clean
	rm -f $(PACKER) $(SAMPLE) woody 3woody

# Recompilation complète
re: fclean all

# Cibles "non fichiers"
.PHONY: all clean fclean re run
