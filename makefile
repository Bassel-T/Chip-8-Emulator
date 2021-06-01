run:
	g++ -std=c++17 main.cpp chip8.h -I"include" -L"lib" -Wall -lSDL2 -lSDL2_image -o Main
	./Main $(FILENAME)