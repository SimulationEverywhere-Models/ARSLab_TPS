CC=g++
CFLAGS=-std=c++17

INCLUDECADMIUM=-I ../../cadmium/include
INCLUDEDESTIMES=-I ../../DESTimes/include
INCLUDEJSON=-I ../../cadmium/json/include
VARIABLES=#-DNDEBUG

#CREATE BIN AND BUILD FOLDERS TO SAVE THE COMPILED FILES DURING RUNTIME
bin_folder := $(shell mkdir -p bin)
build_folder := $(shell mkdir -p build)
results_folder := $(shell mkdir -p simulation_results)

#TARGET TO COMPILE ALL THE TESTS TOGETHER (NOT SIMULATOR)
message.o: data_structures/message.cpp data_structures/message.hpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) $(INCLUDEJSON) $(VARIABLES) data_structures/message.cpp -o build/message.o

main_random_impulse_test.o: test/main_random_impulse_test.cpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) $(INCLUDEJSON) $(VARIABLES) test/main_random_impulse_test.cpp -o build/main_random_impulse_test.o

main_ri_responder_test.o: test/main_ri_responder_test.cpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) $(INCLUDEJSON) $(VARIABLES) test/main_ri_responder_test.cpp -o build/main_ri_responder_test.o

main_ri_re_tr_test.o: test/main_ri_re_tr_test.cpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) $(INCLUDEJSON) $(VARIABLES) test/main_ri_re_tr_test.cpp -o build/main_ri_re_tr_test.o

main_iter_1_test.o: test/main_iter_1_test.cpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) $(INCLUDEJSON) $(VARIABLES) test/main_iter_1_test.cpp -o build/main_iter_1_test.o

ri: main_random_impulse_test.o message.o
	$(CC) $(VARIABLES) -g -o bin/RI_TEST build/main_random_impulse_test.o build/message.o

ri_re: main_ri_responder_test.o message.o
	$(CC) $(VARIABLES) -g -o bin/RI_RESP_TEST build/main_ri_responder_test.o build/message.o

ri_re_tr: main_ri_re_tr_test.o message.o
	$(CC) $(VARIABLES) -g -o bin/RI_RE_TR_TEST build/main_ri_re_tr_test.o build/message.o

iter_1: main_iter_1_test.o message.o
	$(CC) $(VARIABLES) -g -o bin/ITER_1_TEST build/main_iter_1_test.o build/message.o

#TARGET TO COMPILE EVERYTHING (ABP SIMULATOR + TESTS TOGETHER)
all: ri ri_re ri_re_tr iter_1

#CLEAN COMMANDS
clean:
	rm -f bin/* build/*