all : clean serial

serial : serial.cpp
	g++ -lpthread serial.cpp -o ImageFilters.out
clean : 
	rm -f *.out *output*.bmp
