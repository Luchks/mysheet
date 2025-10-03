CC=gcc
CFLAGS=-Wall -Iinclude
LDFLAGS=-lncurses
OBJDIR=obj
SRCDIR=src
OBJS=$(OBJDIR)/cell.o $(OBJDIR)/csv.o $(OBJDIR)/formula.o $(OBJDIR)/utils.o $(OBJDIR)/ui.o $(OBJDIR)/main.o
TARGET=mysheet

all: $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -f $(OBJDIR)/*.o $(TARGET)
