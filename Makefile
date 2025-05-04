CC = clang
CFLAGS = -Wall -Wextra -std=c11 -g
SRC = src
OUT = dist

.PHONY: all clean

all: $(OUT)/raylib

$(OUT):
	mkdir -p $(OUT)
	cp $(SRC)/assets $(OUT)/ -r

$(OUT)/raylib: $(OUT)/raylib.o $(OUT)/chess.o | $(OUT)
	$(CC) $(CFLAGS) -lraylib -o $@ $^

$(OUT)/raylib.o: $(SRC)/raylib.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/chess.o: $(SRC)/chess.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OUT)
