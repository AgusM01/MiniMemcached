# Makefile básico

# Definición de variables
CC = gcc
CFLAGS = -Wall -Werror -g
TARGET = mi_programa
STRUCTURES = memc.c memc_node.c memc_queue.c memc_table.c 
UTILS = utils.c
CONCURRENT = lightswitch.c memc_stat.c
TEST = test.c


# Regla por defecto
all: $(TARGET)

# Regla para construir el programa
$(TARGET): $(STRUCTURES) $(UTILS) $(CONCURRENT) $(TEST)
	$(CC) $(CFLAGS) -o $(TARGET) test.c $(STRUCTURES) $(UTILS) $(CONCURRENT)

# Regla para limpiar los archivos generados
clean:
	rm -f $(TARGET)
