CC = clang
CFLAGS = -Wall -Wextra -std=c11 -g
SRC = src
OUT = dist

.PHONY: all clean

all: $(OUT)/main

$(OUT):
	mkdir -p $(OUT)
	cp $(SRC)/assets $(OUT)/ -r

$(OUT)/main: $(OUT)/main.o $(OUT)/chess.o $(OUT)/game.o $(OUT)/ds.o | $(OUT)
	$(CC) $(CFLAGS) -lraylib -o $@ $^

$(OUT)/main.o: $(SRC)/main.c $(SRC)/game.h $(SRC)/chess.h $(SRC)/ds.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/chess.o: $(SRC)/chess.c $(SRC)/chess.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/game.o: $(SRC)/game.c $(SRC)/game.h $(SRC)/chess.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/ds.o: $(SRC)/ds.c $(SRC)/ds.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OUT)
