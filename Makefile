NAME = maze

CC = gcc
CFLAGS = -Wall -Ofast -DNDEBUG
DBCFLAGS = -Wall -O0 -g -DDEBUG
LIBS =

STRIP = strip

$(NAME): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)
	$(STRIP) $@

debug: main.c
	$(CC) $(DBCFLAGS) -o $(NAME) $< $(LIBS)

clean:
	$(RM) $(NAME)
