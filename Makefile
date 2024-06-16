guava.o: src/guava.c
	mkdir -p build
	gcc -c src/guava.c -o build/guava.o

tcp-server: guava.o
	mkdir -p build
	gcc examples/tcp.c build/guava.o -o build/tcp-server

timer: guava.o
	mkdir -p build
	gcc examples/timer.c build/guava.o -o build/timer

clean:
	rm -rf build