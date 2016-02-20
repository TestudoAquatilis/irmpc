CC=gcc
OPTFLAGS=-O0
CFLAGS=-c -ggdb -Wall -std=gnu99 $(OPTFLAGS)
LDFLAGS=$(OPTFLAGS)

LIBS=glib-2.0

CFLAGS+=$(shell pkg-config --cflags $(LIBS))
LDFLAGS+=$(shell pkg-config --libs $(LIBS))

SOURCES=playlist.c options.c main.c
EXECUTABLE=irmpc

OBJDIR=obj
OBJECTS=$(SOURCES:%.c=$(OBJDIR)/%.o)
DEPS=$(SOURCES:%.c=$(OBJDIR)/%.d)

.PHONY: all
all: $(SOURCES) $(EXECUTABLE)

-include $(OBJECTS:.o=.d)

$(EXECUTABLE): $(OBJECTS) Makefile
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(OBJDIR)/%.o: %.c Makefile | $(OBJDIR)
	$(CC) -MM $(CFLAGS) $*.c > $(OBJDIR)/$*.d
	sed -i -e "s/\\(.*\\.o:\\)/$(OBJDIR)\\/\\1/" $(OBJDIR)/$*.d
	$(CC) $(CFLAGS) $*.c -o $(OBJDIR)/$*.o

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(EXECUTABLE) $(OBJECTS) $(DEPS)
	rm -rf $(OBJDIR)
