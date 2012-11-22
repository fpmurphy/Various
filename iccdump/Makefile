SHELL = /bin/bash
CC = gcc -O
OBJS = icc.o iccdump.o iccstd.o
TARGET = iccdump
LDFLAGS = -lm

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) 

clean:
	rm -f $(TARGET) $(OBJS)
