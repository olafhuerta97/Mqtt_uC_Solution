CC=gcc
CFLAGS=-I -pthread -Wall 

mqtt_m1_test: 
	$(CC) mqttlib/*.c src/*.c -o output/mqtt_m1_test $(CFLAGS) 

.PHONY: clean

clean:
	rm -f output/mqtt_m1_test


all: clean mqtt_m1_test

.PHONY: all
