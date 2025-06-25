CC = g++
CFLAGS = -Wall -Os -ffunction-sections -fdata-sections
TARGET = copyLibs
SRC = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -std=c++17 -static \
	-lboost_system -lboost_filesystem -lboost_regex -lboost_thread \
	-lpthread -lfmt \
	-Wl,--gc-sections
	strip $(TARGET)

clean:
	rm -f $(TARGET)
