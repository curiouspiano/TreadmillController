treadmake: src/tread.c
	gcc -pthread -lwiringPi -o tread src/tread.c