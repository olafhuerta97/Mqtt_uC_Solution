CC=gcc
CFLAGS=-I -pthread -Wall -W -Wextra

mqtt_m1_test: 
	$(CC) mqttlib/*.c src/*.c -o output/mqtt_m1_test $(CFLAGS) 

.PHONY: clean

clean:
	rm -f output/mqtt_m1_test