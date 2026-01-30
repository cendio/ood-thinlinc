SRC = pam_tlpasswd.c
TARGET = pam_tlpasswd.so

CC = gcc
CFLAGS = -fPIC -fno-stack-protector -shared
LDLIBS = -lcrypt

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDLIBS)

clean:
	rm -f $(TARGET)
