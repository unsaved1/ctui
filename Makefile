CC = gcc 
CFLAGS = -g -Wall -std=c99 

INCLUDES = -I./

LIBS = -lncurses -lm
SRCS = ctui.c arena.c hash.c str.c 
OBJS = $(SRCS:.c=.o)

MAIN = ctui

.PHONY: depend clean

all: $(MAIN) 
	@echo CTUI has been compiled

$(MAIN): $(OBJS) 
		$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN).out $(OBJS) $(LIBS)
	
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) *.o *~ $(MAIN).out

depend: $(SRCS)
		makedepend $(INCLUDES) $^

