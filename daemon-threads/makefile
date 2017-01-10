all: threads

CC       = gcc
CFLAGS   = -Wall -Wextra -pthread
LDFLAGS = -o

ifdef DEBUG
CFLAGS += -ggdb3
else
CFLAGS += -O2
endif

SDIR   = src

SOURCES  := $(wildcard $(SDIR)/*.c)
HEADERS := $(wildcard $(SDIR)/*.h)
OBJECTS  := $(SOURCES:$(SDIR)/%.c=$(SDIR)/%.o)

threads: $(OBJECTS)
	$(CC) $(LDFLAGS) $@ $(CFLAGS) $(OBJECTS)

$(OBJECTS): $(SDIR)/%.o : $(SDIR)/%.c
	$(CC) $(CFLAGS) -c $< $(LDFLAGS) $@

.PHONEY: clean
clean:
	rm -f $(OBJECTS) threads
