CC = g++
CFLAGS = -Wall -Og -DNDEBUG -s -ffunction-sections -fdata-sections -Wl,--gc-sections
TARGET = adapt_files_gen
SRC = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -std=c++17 -static -lboost_system -lboost_filesystem -lboost_regex -lboost_thread -lboost_program_options -lpthread -lfmt
	
clean:
	rm -f $(TARGET)