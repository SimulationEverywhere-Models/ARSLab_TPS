CC=g++
CFLAGS=-std=c++17

INCLUDECADMIUM=-I ../../cadmium/include
INCLUDEDESTIMES=-I ../../DESTimes/include

#CREATE BIN AND BUILD FOLDERS TO SAVE THE COMPILED FILES DURING RUNTIME
bin_folder := $(shell mkdir -p bin)
build_folder := $(shell mkdir -p build)
results_folder := $(shell mkdir -p simulation_results)

#TARGET TO COMPILE ALL THE TESTS TOGETHER (NOT SIMULATOR)
impulse_message.o: data_structures/impulse_message.cpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) data_structures/impulse_message.cpp -o build/impulse_message.o

main_random_impulse_test.o: test/main_random_impulse_test.cpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) test/main_random_impulse_test.cpp -o build/main_random_impulse_test.o

main_ri_responder_test.o: test/main_ri_responder_test.cpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) test/main_ri_responder_test.cpp -o build/main_ri_responder_test.o

ri: main_random_impulse_test.o impulse_message.o
	$(CC) -g -o bin/RI_TEST build/main_random_impulse_test.o build/impulse_message.o

ri_resp: main_ri_responder_test.o impulse_message.o
	$(CC) -g -o bin/RI_RESP_TEST build/main_ri_responder_test.o build/impulse_message.o

#TARGET TO COMPILE EVERYTHING (ABP SIMULATOR + TESTS TOGETHER)
all: ri ri_resp

#CLEAN COMMANDS
clean:
	rm -f bin/* build/*