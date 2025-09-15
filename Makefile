# Nom des exécutables
PACKER = woody_woodpacker
SAMPLE = sample

# Répertoires
RES_DIR = resources

# Fichiers sources
PACKER_SRC = woody_woodpacker.c
SAMPLE_SRC = $(RES_DIR)/sample.c

# Compilateur et flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -g

# Règle par défaut
all: $(PACKER) $(SAMPLE)

# Compilation du packer
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
	rm -f $(PACKER) $(SAMPLE) woody
