INCLDIR = ./include
CC = gcc
CFLAGS = -O2 -I$(INCLDIR) -Isocket

OBJDIR = obj

_DEPS = ewpdef.h
DEPS = $(patsubst %,$(INCLDIR)/%,$(_DEPS))

_OBJS = server.o
OBJS = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

$(OBJDIR)/%.o: %.c $(DEPS)
	@mkdir -p $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o *~ core $(INCLDIR)/*~
