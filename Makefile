SOURCE	:= Main.c Trace.c Mem_System.h Controller.h Queue.h
CC	:= gcc
TARGET	:= Main
LINK	:= -lm

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -std=c11 -o $(TARGET) $(SOURCE) $(LINK)

clean:
	rm -f $(TARGET)
