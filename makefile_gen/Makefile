CC = g++
CFLAGS = -Wall -Og -DNDEBUG -s -ffunction-sections -fdata-sections -Wl,--gc-sections
TARGET = makefile_gen
SRC = src/main.cpp src/IniParser.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -std=c++17 -static -lboost_system -lboost_filesystem -lboost_regex -lboost_program_options -lpthread -lfmt
	
clean:
	rm -f $(TARGET)