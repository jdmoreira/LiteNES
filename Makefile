CC = gcc
CFLAGS = -MD -O2 -Wall -Werror
LDFLAGS = -lSDL2

CFILES = $(shell find src/ -name "*.c")
OBJS = $(CFILES:.c=.o)

litenes: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o litenes

-include $(patsubst %.o, %.d, $(OBJS))

.PHONY: clean

clean:
	rm -f litenes $(OBJS) $(OBJS:.o=.d)
