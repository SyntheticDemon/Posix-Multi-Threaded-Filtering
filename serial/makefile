all : clean serial

serial : serial.cpp
	g++ -lpthread serial.cpp -o serial.out
clean : 
	rm -f *.out *output*.bmp
