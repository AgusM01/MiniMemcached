# Makefile básico

# Definición de variables
CC = gcc
CFLAGS = -Wall -Werror -g -pthread 
TARGET = server 
STRUCTURES = Structures/memc.c Structures/memc_node.c Structures/memc_queue.c Structures/memc_table.c 
SERVER = Server/text.c Server/manage_clients.c Server/comunicate.c Server/binary.c Server/epoll.c Server/sock.c 
MAIN = main.c
UTILS = Structures/utils.c
CONCURRENT = Structures/lightswitch.c Structures/memc_stat.c
TEST = Structures/test.c

OBJECTS = $(MAIN:.c=.o) $(SERVER:.c=.o) $(STRUCTURES:.c=.o) $(UTILS:.c=.o) $(CONCURRENT:.c=.o) 

# Regla por defecto
all: $(TARGET) 

# Regla para construir el programa
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) -lm

# Regla para limpiar los archivos generados
clean:
	rm -f $(TARGET) *.o Server/*.o Structures/*.o 
