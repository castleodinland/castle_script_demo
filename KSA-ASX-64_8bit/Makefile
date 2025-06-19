CC = g++
CFLAGS = -Wall -g
TARGET = KSA-ASX-64_8bit
SRC = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -std=c++17 -static -lboost_system -lboost_filesystem -lboost_regex -lboost_thread -lpthread -lfmt
	
clean:
	rm -f $(TARGET)