CC = clang
CFLAGS = -Wall -Wextra -std=c11 -g
SRC = src
OUT = dist

.PHONY: all clean

all: $(OUT)/main $(OUT)/libhuman.so

$(OUT):
	mkdir -p $(OUT)
	cp $(SRC)/assets $(OUT)/ -r

$(OUT)/main: $(OUT)/main.o $(OUT)/game.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o | $(OUT)
	$(CC) $(CFLAGS) -lraylib -o $@ $^

$(OUT)/main.o: $(SRC)/main.c $(SRC)/game.h $(SRC)/chess.h $(SRC)/ds.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/chess.o: $(SRC)/chess.c $(SRC)/chess.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/game.o: $(SRC)/game.c $(SRC)/game.h $(SRC)/chess.h $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/util.o: $(SRC)/util.c $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/ds.o: $(SRC)/ds.c $(SRC)/ds.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/human.o: $(SRC)/human.c $(SRC)/chess.h $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/libhuman.so: $(OUT)/human.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o | $(OUT)
	$(CC) -lraylib -shared -fPIC $^ -o $@

clean:
	rm -rf $(OUT)
