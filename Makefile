all: render_eye.out

objloader.o: objloader.cpp objloader.hpp
	g++ -c  -std=c++11 objloader.cpp

shader.o: shader.cpp shader.hpp
	g++ -c -std=c++11 shader.cpp

#~ loader.o: loader.cpp loader.h stb_image.h
#~ 	g++ -c -std=c++11 loader.cpp -lGL -lGLU -lGLEW

render_eye.o: render_eye.cpp
	g++ -c render_eye.cpp

render_eye.out: render_eye.o objloader.o shader.o
	g++ render_eye.o objloader.o shader.o -lglfw -lGL -lGLU -lGLEW  -o render_eye.out

clean:
	rm *.o
	rm *.out
