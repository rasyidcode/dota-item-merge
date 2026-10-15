.PHONY: all test clean
all:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	cmake --build build --parallel
test:
	mkdir -p build
	$(CC) -std=c99 -Wall -Wextra -Wpedantic -Werror -Isrc tests/test_game.c src/game.c -lm -o build/physics-tests
	./build/physics-tests
clean:
	rm -rf build
