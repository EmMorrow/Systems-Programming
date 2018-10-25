CC = c++

all: generate

generate: main.o random.o graph.o dot.o
	$(CC) main.o random.o graph.o dot.o -o generate

main.o: main.cpp
	$(CC) -c main.cpp

random.o: random.cpp
	$(CC) -c random.cpp

graph.o: graph.cpp
	$(CC) -c graph.cpp

dot.o: dot.cpp
	$(CC) -c dot.cpp


