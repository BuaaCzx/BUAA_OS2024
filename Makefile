
init: 
	gcc hello.c -o hello

run: init
	./hello

clean:
	rm hello
