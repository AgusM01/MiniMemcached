# Makefile básico

# Definición de variables
CC = gcc
CFLAGS = -Wall -Werror -g -pthread 
TARGET = server 
STRUCTURES = structures/memc.c structures/memc_node.c structures/memc_queue.c structures/memc_table.c 
MAIN = server/server.c server/text.c server/manage_clients.c server/comunicate.c server/binary.c server/epoll.c server/sock.c
UTILS = structures/utils.c
CONCURRENT = structures/lightswitch.c structures/memc_stat.c
TEST = structures/test.c


# Regla por defecto
all: $(TARGET)

# Regla para construir el programa
$(TARGET): $(STRUCTURES) $(UTILS) $(CONCURRENT) $(TEST) $(MAIN) 
	$(CC) $(CFLAGS) -o $(TARGET) $(STRUCTURES) $(UTILS) $(CONCURRENT) $(MAIN) 

# Regla para limpiar los archivos generados
clean:
	rm -f $(TARGET)
