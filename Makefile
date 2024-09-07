server:
	gcc src/file-table.c -I include/ -o out/file-table.o -c
	gcc src/message-definition.c -I include/ -o out/message-definition.o -c
	gcc src/syncee.c -I include/ -o out/syncee.o -c
	gcc src/syncer.c -I include/ -o out/syncer.o -c
	gcc src/main.c out/file-table.o out/message-definition.o out/syncee.o out/syncer.o -I include/ -o out/deamon
client:
	gcc src/client-main.c -I include/ -o out/client
all: 
	make server
	make client
clean:
	rm -rf out/*
