INCLUDE=-I./include/ -I./shared_include/

all: clear battle_srv

battle_srv: main.o battle_core.o battle_srv.o log.o
	gcc $? -o battle_srv

main.o: main.c
	gcc $(INCLUDE) -c -g main.c

battle_core.o: battle_engine.c
	gcc $(INCLUDE) -c -g  $? -o battle_core.o

battle_srv.o: battle_server.c
	gcc $(INCLUDE) -c -g $? -o battle_srv.o

log.o: log.c
	gcc $(INCLUDE) -c -g  $? -o log.o

clear:
	rm -f *.o battle_srv
