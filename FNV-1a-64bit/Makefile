CC = g++
CFLAGS = -Wall -g
TARGET = FNV-1a-64bit
SRC = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -std=c++17 -static -lboost_system -lboost_filesystem -lboost_regex -lboost_thread -lpthread -lfmt
	
clean:
	rm -f $(TARGET)