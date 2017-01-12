all: threads

CC       = gcc
CFLAGS   = -Wall -Wextra -pthread

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
	$(CC) -o $@ $(CFLAGS) $(OBJECTS)

$(OBJECTS): $(SDIR)/%.o : $(SDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONEY: clean
clean:
	rm -f $(OBJECTS) threads
