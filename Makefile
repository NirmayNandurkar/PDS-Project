all:
	g++ -I src/include -L src/lib -o main contra.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer 