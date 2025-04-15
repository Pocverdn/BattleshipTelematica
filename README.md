# BattleshipTelematica
## Como correr el programa.

### client.out
El programa esta diseñado para funcionar con el sistema operativo linux.
Se necesita un archivo address.config para decirle al cliente donde se va a conectar.
La estructura de este archivo debe ser:

**serverip= IP a conectarse**

**port=el puerto.**

El cliente requiere requiere un log, este puede crear un log si no existe uno, pero aun necesita que se le espesifique donde crearlo.

**./client.out path/log**

### server.out
Al igual que el cliente requiere que se le espesifique donde queda el archivo de log, si no encunetra alguno, el programa crea uno en esa ubicación.
Ademas de esto el servidor necesita que se le da la IP y el puerto donde se va a hostiar por consola.
La estructura es, IP, puerto y luego el log:

**./server.out IP puerto path/log**

# Introducción

El trabajo trata de un juego de batalla naval, donde un servidor desarrollado en C, y 2 o más clientes desarrollados en C++, pueden jugar partidas en las cuales se permite realizar las acciones típicas del juego de mesa tales como:

    Ubicar barcos
    Disparar a las casillas
    Recibir notificaciones de acierto, agua o hundimiento

Este juego ha sido diseñado para poder correr en máquinas con un sistema operativo LINUX y ha sido probado por medio del servicio de nube de AWS (Amazon Web Service), utilizando además sockets, threads y un sistema de codificación desarrollado por el equipo para crear el protocolo que se utilizara para enviar los datos de las naves, ataques y banderas, todo con el objetivo de hacer el transporte de datos lo más preciso posible en cuanto a cantidad de bytes enviados se refiere.

Adicionalmente, tanto cliente como servidor mantienen un registro de todas las acciones hechas durante sus ejecuciones, es decir, los disparos, ingresos de sección, quién gano o perdió, etc.

# Desarrollo

El desarrollo del proyecto se divide en tres secciones, la primera siendo el código de servidor hecho en C, en el cual se manejan las conexiones entre los diferentes clientes, que piden y envían datos a este para ser procesadas y enviadas tanto al mismo cliente como al del otro jugador, la segunda parte es el desarrollo del código cliente en C++, en el cual se maneja tanto el envío recibo de datos por parte del servidor, como la lógica interna del juego, por ultimo se maneja en un archivo .h todas las funciones del protocolo, es decir los codificadores y decodificadores necesarios para enviar los datos. a continuación explicaremos que contienen estos tres códigos y con que otros archivos van ligados.

## Server.c

Como se explicó previamente, el servidor es el encargado de recibir las peticiones de los clientes (Jugadores), procesar los datos enviados y enviar una respuesta, muchas veces esta respuesta implica mandar un mensaje de notificación tanto al jugador 1 como el jugador 2, este código además está diseñado para ser concurrente, es decir que puede mantener varias secciones de juego a la vez con diferentes jugadores, para esto el código emplea las siguientes herramientas:

Threads: Utilizada para mantener varias conexiones de diferentes clientes a la vez, con el objetivo de poder emparejarlos en una partida

Sockets: Encargado de establecer la conexión entre cliente y servidor, permite tanto enviar como recibir datos, los cuales son posteriormente procesados. Es importante aclarar que utilizamos sockets tipo TCP IPv4

Funciones del juego: Con el objetivo de procesar los datos enviados por los clientes, el servidor contiene la lógica de cada una de las acciones del juego de mesa, tales como disparar, colocar barcos, mostrar tablero, etc.

Codificación y decodificación: Con el objetivo de desarrollar un protocolo personal, el equipo creó varios métodos de codificación para el envío y recibo de ataques y barcos, de esta manera aumentando la eficiencia del transporte de datos.

Log: Un sistema que guarda las acciones hechas por los clientes, tales como, ataques o inicios de sesión.

Structs: El código emplea diferentes tipos de Structs, cada uno con un objetivo específico, desde controlar la lógica del juego, hasta mantener las secciones de juego activas. Algunos ejemplos son:


![image](https://github.com/user-attachments/assets/feba0a4f-44e6-46be-b20f-adb5d1f6be10)
![image](https://github.com/user-attachments/assets/9788f31c-0b60-491c-b08f-55e240582a67)
![image](https://github.com/user-attachments/assets/e946fee6-73de-4158-84c9-8ee880df9142)


Las tecnologias empleadas son:

    Lenguajes: C
    Sistema operativo: Linux
    Modelo de red: TCP/IP
    Bibliotecas:
![image](https://github.com/user-attachments/assets/eb579cb3-4227-4f71-9ffd-580c37aabfe8)



## Cliente.cpp

El codigo de cliente proporciona al jugador con una interfaz visual en la cual puede realizar todas las acciones relacionadas con el juego de batalla naval, desde ubicar los barcos, realizar disparos, recibir notificaciones, etc. El codigo tambien cuenta con un sistema de log donde se registran las acciones hechas por el jugador.

Cliente de igual forma que server, contiene las funciones relacionadas con la logica del juego y codificación/decodificación, de esta manera se puede procesar, codificar y enviar datos concretos al servidor, para que este responda de manera correcta. La explicación de las funciones son las siguientes:

    unsigned char* encode(ship[]): Codifica los datos de 9 barcos en un arreglo de 14 bytes, optimizando el uso de bits.
    
    ship* decode(const unsigned char[]): Reconstruye los barcos desde la representación binaria compacta.
    
    unsigned char encodeAttack(attack): Codifica un ataque en 1 byte.
    
    attack decodeAttack(unsigned char): Decodifica un ataque de 1 byte a estructura.

    initializeBoard: Inicializa el tablero con agua sin revelar (~).

    placeShipSize: Verifica si un barco cabe en el tablero y si el espacio está disponible.
    
    setShips: Permite al usuario colocar manual o aleatoriamente los barcos.
    
    showBoard: Visualiza los tableros del jugador y el oponente lado a lado.

    shoot: Evalúa un disparo e indica si fue un acierto, agua o repetido.

    countShoot: Cuenta los impactos en el tablero.
    
    countShips: Cuenta el total de casillas ocupadas por barcos.

    safe_log: Registra mensajes en un archivo de log de manera segura (con bloqueo de archivo).

    trim: Elimina espacios y saltos de línea de los extremos de una cadena.
    
    parse_config: Lee IP y puerto del servidor desde un archivo de configuración (.conf).



## Protocolo.h

Dicho código como se mencionó anteriormente realiza todas las operaciones de codificar y decodificar los datos que se enviaran tanto desde cliente a servidor como de servidor a cliente, de esta manera garantizando un uso más eficiente del almacenamiento como del transporte, este código está tanto adaptado para funcionar en C como en C++, para de esta forma funcionar como un intermediario entre las dos (Interfaz), de igual manera se encarga de manejar el estado del servidor y las sesiones entre jugadores así como almacenar los datos importantes tales como posiciones de los barcos o estado del tablero.

![image](https://github.com/user-attachments/assets/d4a827b9-f9dd-496d-9782-c8a36f3a5be1)


    decode: Extrae datos de barcos comprimidos en bits y los guarda dentro del struct ship.
    
    encode: Codifica los datos de barcos dentro del struct ship en un arreglo compacto de bits.
    
    receive_encoded_ships: Lee datos codificados desde un socket, los decodifica y los guarda en un arreglo de barcos.
    
    decodeAttack: Extrae las coordenadas x y y de un ataque desde un solo byte codificado.
    
    encodeAttack: Codifica las coordenadas de un ataque (x, y) en un solo byte.

    parse_config: Lee un archivo de configuración y extrae la IP del servidor y el puerto.

    connect_to_server: Crea un socket y se conecta al servidor usando la IP y puerto dados.

    send_turn_messages: Informa a los jugadores cuál debe jugar ("turn") y cuál debe esperar ("wait").

    setup_server: Configura y arranca el servidor en la IP y puerto especificados, dejándolo listo para aceptar conexiones.


### Log

El log es una función que está tanto en servidor como en cliente, guarda un registro de todas las acciones hechas dentro del juego, con la diferencia que el cliente guarda las acciones hechas por el jugador-usuario mientras que el servidor guarda tanto el inicio de sesión de los jugadores, estado del juego, acciones hechas por ambos jugadores, etc.

    safe_log: Abre, bloquea y escribe de forma segura un mensaje con marca de tiempo en el archivo de log especificado.

En general, el log sigue la estructura de *<date> <time> <clientIP> <query> <responseIP>*

• date: indica la fecha en que se efectuó la transacción.

• time: indica la hora en que se realizó la transacción.

• clientIP: la dirección IP del cliente que realiza la petición.

• query: indica el tipo de mensaje que se está realizando.

• response: mensaje de respuesta del servidor.


# Aspectos logrados y No logrados:

En cuanto a las metas conseguidas según las rúbricas puestas en clases, hemos conseguido lo siguiente:

## Metas conseguidas

### Cliente:
    El tablero propio con la posición de los barcos del jugador.
    El tablero del oponente mostrando los disparos realizados y sus resultados.
    Información del estado actual del juego.
    Colocar sus barcos al inicio de la partida (puede ser manual o aleatoria).
    Seleccionar coordenadas para disparar durante su turno.
    Ver el historial de movimientos de la partida.
    Rendirse o solicitar un nuevo juego.
    Tablero propio: Mostrar la posición de los barcos y los disparos recibidos.
    Tablero del oponente: Mostrar los disparos realizados y sus resultados.
    Conectarse al servidor e implementar el protocolo de comunicación.
    Gestionar errores y desconexiones de manera adecuada.

### Servidor:
    Aceptar conexiones de múltiples clientes simultáneamente.
    Emparejar jugadores para iniciar partidas.
    Mantener el estado del juego para cada partida activa.
    Validar todas las acciones de los jugadores según las reglas del juego.
    Notificar a los clientes sobre el resultado de sus acciones y los cambios en el estado del juego.
    Gestionar turnos entre los jugadores.
    Detectar y gestionar desconexiones de clientes.
    Registrar en un archivo de log todas las acciones importantes que ocurren en el servidor.
    Implementar un temporizador para limitar la duración de cada turno a 30 segundos.
    Finalizar las partidas cuando se cumpla la condición de victoria.

## Metas no conseguidas:

### Cliente:
    No podemos actualizar el tiempo en el cliente, además de que el turno no pasa.

### Servidor:
    Todos los requisitos fueron cumplidos.

# Conclusiones:

Nuestro proyecto implementa un juego de batalla naval. Para esto utilizamos sockets para la comunicación entre cliente y servidor, teniendo un archivo .h en el cual se implementa la codificación y decodificación del protocolo con el objetivo de enviar los datos de manera eficiente.

    Tanto el cliente como el servidor utilizan la API de Berkeley para la comunicación. Esto incluye funciones como socket(), bind(), listen(), accept(), connect(), send() y recv().
    
    La implementación está basada en TCP, ya que TCP garantiza la entrega confiable de datos.

## Flujo:

### Cliente:

    El cliente se conecta al servidor, registra al usuario (nombre y correo electrónico) y envía la configuración inicial de los barcos.
    
    Durante el juego, el cliente envía ataques al servidor y recibe respuestas que indican si el ataque fue un acierto, un fallo, o si hundió un barco.

    El cliente maneja eventos como rendición (Ctrl + C), tiempo agotado, y finalización del juego (ganar o perder).

### Servidor:

    El servidor maneja múltiples sesiones de juego utilizando hilos (pthread), lo que permite que varios jugadores jueguen simultáneamente.
    
    Valida los ataques recibidos del cliente y actualiza el estado del juego (tablero, barcos, etc.).
    
    Envía respuestas al cliente para informar sobre el resultado del ataque y el estado del juego.



