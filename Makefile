CC = gcc 
CFLAGS = -g -Wall -std=c99 

INCLUDES = -I./

LIBS = -lncurses -lm
SRCS = ctui.c arena.c hash.c str.c 
OBJS = $(SRCS:.c=.o)

MAIN = ctui

.PHONY: depend clean

all: $(MAIN) 
	@echo Simple compiler named ctui has been compiled

$(MAIN): $(OBJS) 
		$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LIBS)
	
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
		makedepend $(INCLUDES) $^

