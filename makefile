CC = g++
LIBS = -fopenmp -lGL -lGLU -lglfw -lGLEW
DEPS = shader.hpp texture.hpp objloader.hpp
OBJ = shader.o texture.o objloader.o main.o


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(LIBS)

snowman: $(OBJ)
	$(CC) -o $@ $^ $(LIBS) -O3

clean:
	rm -f *.o *~ snowman
