# Makefile básico

# Definición de variables
CC = gcc
CFLAGS = -Wall -Werror -g
TARGET = mi_programa
SOURCE_FILES = test.c memc_node.c memc_queue.c memc_table.c utils.c 

# Regla por defecto
all: $(TARGET)

# Regla para construir el programa
$(TARGET): $(SOURCE_FILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE_FILES)

# Regla para limpiar los archivos generados
clean:
	rm -f $(TARGET)
