INCLDIR = ./include
CC = gcc
CFLAGS = -O2 -I$(INCLDIR) -include stdio.h

OBJDIR = obj

_DEPS = main.h task2_hash.h task2_count.h task2_sum.h
DEPS = $(patsubst %,$(INCLDIR)/%,$(_DEPS))

_OBJS = main.o task2_hash.o task2_count.o task2_sum.o
OBJS = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

$(OBJDIR)/%.o: %.c $(DEPS)
	@mkdir -p $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o *~ core $(INCLDIR)/*~
