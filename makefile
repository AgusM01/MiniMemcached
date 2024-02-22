# Makefile básico

# Definición de variables
CC = gcc
CFLAGS = -Wall -Werror -g -pthread 
TARGET = server 
STRUCTURES = structures/memc.c structures/memc_node.c structures/memc_queue.c structures/memc_table.c 
SERVER = Server/text.c Server/manage_clients.c Server/comunicate.c Server/binary.c Server/epoll.c Server/sock.c 
MAIN = main.c
UTILS = structures/utils.c
CONCURRENT = structures/lightswitch.c structures/memc_stat.c
TEST = structures/test.c

OBJECTS = $(MAIN:.c=.o) $(SERVER:.c=.o) $(STRUCTURES:.c=.o) $(UTILS:.c=.o) $(CONCURRENT:.c=.o) 

# Regla por defecto
all: $(TARGET) 

# Regla para construir el programa
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) -lm

# Regla para limpiar los archivos generados
clean:
	rm -f $(TARGET) *.o Server/*.o structures/*.o 
