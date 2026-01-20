INCLDIR = ./include
CC = gcc
CFLAGS = -O2 -I$(INCLDIR) -Isocket

OBJDIR = obj

_DEPS = client.h 
DEPS = $(patsubst %,$(INCLDIR)/%,$(_DEPS))

_OBJS = client.o
OBJS = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

$(OBJDIR)/%.o: %.c $(DEPS)
	@mkdir -p $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o *~ core $(INCLDIR)/*~
