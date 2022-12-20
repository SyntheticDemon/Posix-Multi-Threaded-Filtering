all : clean parallel

parallel : parallel.cpp
	g++ -lpthread parallel.cpp -o parallel.out
clean : 
	rm -f *.out *output*.bmp