NAME = maze

CC = gcc
CFLAGS = -Wall -Ofast -DNDEBUG
DBCFLAGS = -Wall -O0 -g -DDEBUG
DBPCFLAGS = -Wall -O0 -pg -DDEBUG
LIBS =

STRIP = strip

$(NAME): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)
	$(STRIP) $@

debug: main.c
	$(CC) $(DBCFLAGS) -o $(NAME) $< $(LIBS)

gprof: main.c
	$(CC) $(DBPCFLAGS) -o $(NAME) $< $(LIBS)
	@echo "Please execute '$(NAME)' once."
	@echo "Then you can run:"
	@echo "> gprof $(NAME) gmon.out | less"

clean:
	$(RM) $(NAME) perf.data gmon.out
