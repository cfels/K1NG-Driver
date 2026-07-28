# compiler flags
CC = gcc
CFLAGS = -Wall -Wextra -Iincludes -Isrc

# source & output
SRC = main.c src/driver.c src/data.c
OBJ = main.o src/driver.o src/data.o
TARGET = k1ng_driver
LDFLAGS = -lusb-1.0

# TARGET
all: $(TARGET)

# link
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# c to object files
main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

src/driver.o: src/driver.c
	$(CC) $(CFLAGS) -c src/driver.c -o src/driver.o

src/data.o: src/data.c
	$(CC) $(CFLAGS) -c src/data.c -o src/data.o

# cleanup build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)
