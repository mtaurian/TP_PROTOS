
# 72.07 Protocolos de Comunicación - Trabajo Especial - 2024/2

## Aclaraciones importantes:
- El servidor POP3 por defecto corre en el puerto 1080 y en la dirección `0.0.0.0.`.
- La aplicación que implementa el protocolo MGMT corre por defecto en el puerto 8080 y en la dirección `127.0.0.1.`.
Estos valores se puede cambiar por parámetro al ejecutar el servidor. Ver sección "Ejecución del Servidor" para más información.

## Compilación
Para crear los archivos ejecutables, se debe ejecutar el comando `make all` en carpeta raíz del proyecto.
Luego, los archivos generados se encontrarán dentro del directorio `./bin`.

## Ejecución
### Ejecución del Servidor
Para ejecutar el servidor se debe correr los siguientes comandos dentro del directorio `bin`:
```bash
./pop3 -d <maildir_path> -U <user>:<pass> [[-u <user>:<pass>]...] [OPTION]...
```
donde: 
- `-h` imprime la ayuda y termina.
- `-l <POP3 addr>` es la dirección donde servirá el servidor POP3. Si no se especifica, su valor default es `0.0.0.0.`.
- `-L <MGMT addr>` es la dirección donde servirá el servicio de management. Si no se especifica, su valor default es `127.0.0.1.`.
- `-p <POP3 port>` es el puerto donde correrá el servidor. Si no se especifica, su valor default es 1080.
- `-P <MGMT port>` es el puerto entrante conexiones configuracion. Si no se especifica, su valor default es 8080.
- `-u <user>:<password>` son los distintos usuarios que hay en el servidor. En esta opción se deben incluir TODOS los usuarios que aparecen dentro del directorio mails y asignarles una contraseña.
- `-U <user>:<password>` es el usuario y contraseña del administrador del servidor. Estas credenciales son las que se deben mandar siempre que se manden requests con el cliente desarrollado.
- `-v` imprime información sobre la versión versión y termina.
- `-d <Maildir path>` es el directorio de mails del servidor. Aquí dentro se encontrarán los directorios de los distintos usuarios. Este respeta una cierta estructura, ver sección "Estructura del directorio de mails" para más información.
- `-t <cmd>` es el ejecutable para llevar a cabo transformaciones.

### Ejecución del Cliente
Para ejecutar el cliente se debe correr el siguiente comando:
```bash
./manager_client -S <user>:<password> [OPTION]...
```
donde:
- `-S <user>:<password>` es el usuario y contraseña del administrador del servidor. Este debió haber sido especificado al ejecutar el servidor con el flag `-U`.
- `-h` imprime la ayuda y termina.
- `-H <host>` es la dirección donde servirá el servicio de management. Si no se especifica, su valor default es `127.0.0.1.`.
- `-P <port>` es el puerto entrante conexiones configuracion. Si no se especifica, su valor default es 8080.
- `-U` envia una solicitud para obtener los usuarios registrados.
- `-A <user>:<password>` envia una solicitud para agregar un usuario al servidor.
- `-D <user>` envia una solicitud para eliminar un usuario al servidor.
- `-M` envia una solicitud para obtener métricas específicas del servidor.
- `-L` envia una solicitud para obtener los logs del servidor.

## Estructura del directorio de mails
Los directorios de los usuarios deben estar dentro de la carpeta que se especifique con la opción `-d` al ejecutar `./pop3`.
Dicho directorio debe contener únicamente directorios (los correspondientes a los usuarios). Asimismo, los directorios de los usuarios pueden contener solo tres directorios:
- new: donde se encuentran los mails a leer. Si no se encuentra este directorio, se considera que no hay mails nuevos.
- cur: donde se encuentran los mails eliminados. Si no se encuentra este directorio y se elimina un mail, se agrega.
- tmp
Por ejemplo, una estructura válida sería la siguiente:
```bash
mail
├── user1
│    ├── cur
│   ├── new
|   |    └── mail1
│   └── tmp
└── user2
    ├── cur
    ├── new
    └── tmp
```

## Estructura del proyecto
El proyecto se estructura de la siguiente manera:
```bash
TP_PROTOS
├── docs
└── src
    ├── management
    |   └── client
    └── server
    │   ├── manager
    │   |    ├── include
    |   |    └── manager_states_definition
    |   └── pop3
    │   |    ├── include
    |   |    └── states_definition
    └── shared
    └── testing
```
- Dentro de `src` se encuentra todo el código fuente.
- `docs` contiene el informe del trabajo.
- `management/client` contiene el código desarrollado para la aplicación cliente.
- `server` contiene todo el código relacionado con el servidor.
    - `manager` ontiene el código relacionada a la aplicación que implementa el protocolo creado por el equipo.
    - `pop3` ontiene el código relacionada a la aplicación que implementa el protocolo POP3.
- `shared` contiene el buffer de escritura/lectura provisto por la cátedra, el parser de comandos del servidor y distintas funciones útiles que se utilizan en los otros directorios.


## Autores: 

- Badin, Diego            63551
- Rabinovich, Diego       63155
- Taurian, Magdalena      62828
- Techenski, Julieta      62547
