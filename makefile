# Makefile básico

# Definición de variables
CC = gcc
CFLAGS = -Wall -Werror -g -pthread 
TARGET = server 
STRUCTURES = structures/memc.c structures/memc_node.c structures/memc_queue.c structures/memc_table.c 
MAIN = server.c text_parser.c int_to_binary.s 
UTILS = structures/utils.c
CONCURRENT = structures/lightswitch.c structures/memc_stat.c
TEST = structures/test.c

OBJECTS = $(STRUCTURES:.c=.o) $(UTILS:.c=.o) $(CONCURRENT:.c=.o) $(MAIN:.c=.o)

# Regla por defecto
all: $(TARGET)

# Regla para construir el programa
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) -lm

# Regla para limpiar los archivos generados
clean:
	rm -f $(TARGET)
