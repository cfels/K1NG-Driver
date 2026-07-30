# compiler flags
CC = gcc
CFLAGS = -Wall -Wextra -Iincludes -Isrc -Iincludes/tomlc17-R260618/src

# source & output
SRC = main.c src/driver.c src/data.c src/config.c src/help.c includes/tomlc17-R260618/src/tomlc17.c
OBJ = main.o src/driver.o src/data.o src/config.o src/help.o includes/tomlc17-R260618/src/tomlc17.o
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

src/config.o: src/config.c
	$(CC) $(CFLAGS) -c src/config.c -o src/config.o

src/help.o: src/help.c
	$(CC) $(CFLAGS) -c src/help.c -o src/help.o

includes/tomlc17-R260618/src/tomlc17.o: includes/tomlc17-R260618/src/tomlc17.c
	$(CC) $(CFLAGS) -c includes/tomlc17-R260618/src/tomlc17.c -o includes/tomlc17-R260618/src/tomlc17.o

# cleanup build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)
