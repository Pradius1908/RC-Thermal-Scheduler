CC = gcc
CFLAGS = -Wall -O2
TARGET = rc_sched

SRC = src/rc_thermal_scheduler.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) -lm

clean:
	rm -f $(TARGET)

