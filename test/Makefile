runtests: clear core_tests
	./core_tests

core_tests: battle_engine.o test_battle_engine.o
	gcc $? -o core_tests

battle_engine.o: ../src/battle_engine.c
	gcc -c ../src/battle_engine.c -g

test_battle_engine.o: test_battle_engine.c
	gcc -c test_battle_engine.c -g

clear:
	rm -f *.o core_tests
