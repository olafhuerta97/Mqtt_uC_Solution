CC=gcc
CFLAGS= -pthread -Wall 
PROGRAM_NAME = mqtt_bbb_cust

output/mqtt_bbb_cust: 
	$(CC) mqttlib/*.c src/*.c -o Output/mqtt_bbb_cust $(CFLAGS) 

.PHONY: clean

clean:
	rm -f Output/mqtt_bbb_cust


all: clean mqtt_bbb_cust

.PHONY: all

run:  ./Output/${PROGRAM_NAME}   
	./Output/${PROGRAM_NAME} 
        
.PHONY: run
