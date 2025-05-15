CC = clang
CFLAGS = -Wall -Wextra -std=c11 -pg
SRC = src
OUT = dist

.PHONY: all clean

all: $(OUT)/main $(OUT)/libhuman.so $(OUT)/librandom.so $(OUT)/libminmax.so

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

$(OUT)/random.o: $(SRC)/random.c $(SRC)/chess.h $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/minmax.o: $(SRC)/minmax.c $(SRC)/chess.h $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/libhuman.so: $(OUT)/human.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o | $(OUT)
	$(CC) -lraylib -shared -fPIC $^ -o $@

$(OUT)/librandom.so: $(OUT)/random.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o | $(OUT)
	$(CC) -shared -fPIC $^ -o $@

$(OUT)/libminmax.so: $(OUT)/minmax.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o | $(OUT)
	$(CC) -shared -fPIC $^ -o $@

clean:
	rm -rf $(OUT)
