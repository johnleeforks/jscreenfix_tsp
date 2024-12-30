# Variables de configuración
CROSS_COMPILE = /opt/trimui-toolchain/usr/aarch64-linux-gnu
CC = /opt/trimui-toolchain/usr/bin/aarch64-linux-gnu-gcc
CFLAGS = -Wall -O2 -I$(CROSS_COMPILE)/sysroot/usr/include
LDFLAGS = -L$(CROSS_COMPILE)/sysroot/usr/lib -lSDL2

# Archivos del programa
TARGET = fixpixels
SRC = main.c
OBJ = $(SRC:.c=.o)

# Reglas de compilación
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJ) $(TARGET)
