include ./Makefile.inc

SHARED_SOURCES=$(wildcard src/shared/*.c)
SERVER_SOURCES=$(wildcard src/server/*.c src/server/manager/*.c  src/server/manager/manager_states_definition/*.c)
STATES_SOURCES=$(wildcard src/server/states_definition/*.c)
CLIENT_SOURCES=$(wildcard src/management/*.c src/management/client/*.c)

OUTPUT_FOLDER=./bin
MANAGEMENT_OUTPUT_FOLDER=./bin/management
MANAGER_OUTPUT_FOLDER=./bin/management/manager
CLIENT_OUTPUT_FOLDER=./bin/management/client
OBJECTS_FOLDER=./obj

SHARED_OBJECTS=$(SHARED_SOURCES:src/%.c=obj/%.o)
SERVER_OBJECTS=$(SERVER_SOURCES:src/%.c=obj/%.o)
STATES_OBJECTS=$(STATES_SOURCES:src/%.c=obj/%.o)
CLIENT_OBJECTS=$(CLIENT_SOURCES:src/%.c=obj/%.o)
MANAGEMENT_OBJECTS=$(MANAGER_SOURCES:src/%.c=obj/%.o)

SERVER_OUTPUT_FILE=$(OUTPUT_FOLDER)/pop3
CLIENT_OUTPUT_FILE=$(MANAGEMENT_OUTPUT_FOLDER)/client
MANAGER_OUTPUT_FILE=$(MANAGEMENT_OUTPUT_FOLDER)/manager

all: server manager_client

server: $(SERVER_OUTPUT_FILE)
manager_client: $(CLIENT_OUTPUT_FILE)

$(SERVER_OUTPUT_FILE): $(SERVER_OBJECTS) $(SHARED_OBJECTS) $(STATES_OBJECTS)
	mkdir -p $(OUTPUT_FOLDER)
	$(COMPILER) $(CFLAGS) $(LDFLAGS) $(SERVER_OBJECTS) $(SHARED_OBJECTS) $(STATES_OBJECTS)  -o $(SERVER_OUTPUT_FILE)

$(CLIENT_OUTPUT_FILE): $(CLIENT_OBJECTS)
	mkdir -p $(MANAGEMENT_OUTPUT_FOLDER)
	$(COMPILER) $(CFLAGS) $(LDFLAGS) $(CLIENT_OBJECTS) -o $(CLIENT_OUTPUT_FILE)

obj/%.o: src/%.c
	mkdir -p $(OBJECTS_FOLDER)/server
	mkdir -p $(OBJECTS_FOLDER)/server/states_definition
	mkdir -p $(OBJECTS_FOLDER)/management
	mkdir -p $(OBJECTS_FOLDER)/management/client
	mkdir -p $(OBJECTS_FOLDER)/management/manager
	mkdir -p $(OBJECTS_FOLDER)/server/manager
	mkdir -p $(OBJECTS_FOLDER)/server/manager/manager_states_definition
	mkdir -p $(OBJECTS_FOLDER)/shared
	$(COMPILER) $(COMPILERFLAGS) -c $< -o $@

clean:
	rm -rf $(OUTPUT_FOLDER)
	rm -rf $(OBJECTS_FOLDER)

.PHONY: all server client clean