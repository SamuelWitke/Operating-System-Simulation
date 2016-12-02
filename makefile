a.out: os.o sos.o
	g++ os.o sos.o
os.o: os.cpp PCB.h
	g++ -c os.cpp
clean:
	rm a.out os.o 
