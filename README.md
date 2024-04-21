# MiniMemcached

## ¿Que es?
Se trata de una versión simplificada de Memcached el cual es un sistema distribuido de propósito general para caché basado en memoria. Es empleado para el almacenamiento en caché de datos u objetos en la memoria RAM, reduciendo así las necesidades de acceso a un origen de datos externo (como una base de datos o una API).

## ¿Como funciona?
Su funcionamiento se puede dividir en dos partes. El servidor y la estructura.
Por parte del servidor, este es un servidor que usa la API de sockets y utiliza las funcionalidades de la API de epoll para su eficiencia.
Por parte de la estructura, esta consta de una tabla hash y una cola. En la tabla hash los clientes guardarán sus datos y la cola es usada para tener una noción de cantidad de "uso" de los datos ya que, cuando no se disponga de más memoria, se usa una política "LRU" (Least Recently Used - Menos Recientemente Usado) para la eliminación de datos en la tabla.

## Uso

Para compilar solo basta escribir `make` en la consola.
Para ejecutar `./server`

Consta de dos protocolos, texto y binario.

Las funcionalidades permitidas son:

- PUT key value: Agrega un valor v bajo la clave k.
- GET key: Retorna un valor asociado a la clave k.
- DEL key: Elimina el valor asociado a la clave k.
- STATS: Da diferentes estadisticas del programa.

Ante una respuesta afirmativa el programa responderá con **OK**. En el caso de un GET será **OK v** donde v es el valor asociado a la clave ingresada.

Diversos errores:

- EINVAL: El comando ingresado no es válido.
- EBIG: El comando a la respuesta son demasiado grandes.
- ENOTFOUND: La clave ingresada no fué encontrada.
- EBINARY: Se trató de ingresar datos binarios en el protocolo de texto.

### Texto
Para usar este protocolo se debe inicializar el sevidor y conectarnos mediante protocolo *netcat* al puerto **888**.

Cada mensaje obligatoriamente debe ser menor a los **2048 bytes** y debe terminar con **\n**.

### Binario
Para usar este protocolo se debe inicializar el sevidor y conectarnos mediante protocolo *netcat* al puerto **889**.

No hay longitud maxima de mensaje.

Los valores correspondientes a cada comando son los siguientes:

- PUT: 11
- DEL: 12
- GET 13
- STATS: 21
- OK: 101

La forma de mensaje debe cumplir las siguientes normas:

- 1er byte: Comando
- 2do a 5to byte: Longitud de la clave representada en un entero de 32 bits.
- 6to byte: Clave.

En el caso de ser un PUT (11):

- 7mo a 10mo byte: Longitud del valor representada en un entero de de 32 bits.
- 11vo: Valor.

## Interfaz Erlang
Se posee una interfaz programada en Erlang para el uso del modo binario.