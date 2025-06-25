CC = g++
CFLAGS = -Wall -Os -ffunction-sections -fdata-sections
TARGET = makefile_gen
SRC = src/main.cpp src/IniParser.cpp


all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -std=c++17 -static \
	-lboost_system -lboost_filesystem -lboost_regex -lboost_thread \
	-lpthread -lboost_program_options -lfmt \
	-Wl,--gc-sections
	strip $(TARGET)

clean:
	rm -f $(TARGET)
