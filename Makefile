.PHONY: all test clean
CMAKE ?= cmake
CMAKE_ARGS ?=
all:
	$(CMAKE) -S . -B build -DCMAKE_BUILD_TYPE=Release $(CMAKE_ARGS)
	$(CMAKE) --build build --parallel
test:
	$(CMAKE) -S . -B build-tests -DBUILD_GAME=OFF -DCMAKE_BUILD_TYPE=Debug $(CMAKE_ARGS)
	$(CMAKE) --build build-tests --parallel
	ctest --test-dir build-tests --output-on-failure
clean:
	$(CMAKE) -E remove_directory build
	$(CMAKE) -E remove_directory build-tests
