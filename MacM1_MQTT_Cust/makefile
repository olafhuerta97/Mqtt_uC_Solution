CC=gcc
CFLAGS=-I -pthread -Wall 
ODIR=output
PROGRAM_NAME = $(ODIR)/mqtt_m1_test
OBJ = mqttlib/*.c src/*.c 

$(PROGRAM_NAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) 

.PHONY: clean

clean:
	rm -f output/mqtt_m1_test


.PHONY: all

all: clean mqtt_m1_test

run:    ${PROGRAM_NAME}
	./${PROGRAM_NAME} 
