CC = clang
CFLAGS = -Wall -Wextra -std=c11 -g
SRC = src
OUT = dist

.PHONY: all clean

all: $(OUT)/main

$(OUT):
	mkdir -p $(OUT)
	cp $(SRC)/assets $(OUT)/ -r

$(OUT)/main: $(OUT)/main.o $(OUT)/chess.o $(OUT)/game.o | $(OUT)
	$(CC) $(CFLAGS) -lraylib -o $@ $^

$(OUT)/main.o: $(SRC)/main.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/chess.o: $(SRC)/chess.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/game.o: $(SRC)/game.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OUT)
