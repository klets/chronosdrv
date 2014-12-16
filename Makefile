INCDIR = include
SRCDIR = src
LIBDIR = lib
BINDIR = bin

OUT_DIR = $(BINDIR) $(LIBDIR)

CC=gcc

CFLAGS=-g -Wall -pedantic -I$(INCDIR)

TARGET = chronosdrv

MKDIR_P = mkdir -p

.PHONY: directories

all: directories $(TARGET) $(EXAMPLES)


# $(OBJDIR)/%.o: $(SRCDIR)/%.c
# 	$(CC) $(CFLAGS) $< -o $@


chronosdrv: $(SRCDIR)/*.c
	$(CC) $(CFLAGS) $^ -o $(BINDIR)/$@

directories: ${OUT_DIR}

${OUT_DIR}:
	${MKDIR_P} ${OUT_DIR}

clean:
	rm -rf $(OUT_DIR)