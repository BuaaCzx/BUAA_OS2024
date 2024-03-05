.PHONY: clean

out: calc case_all
	./calc < case_all > out

calc:
	gcc calc.c -o calc

case_all: case_add case_sub case_mul case_div
	cat case_add case_sub case_mul case_div > case_all

case_add: gcc
	./casegen add 100 > case_add

case_sub: gcc
	./casegen sub 100 > case_sub

case_mul: gcc
	./casegen mul 100 > case_mul

case_div: gcc
	./casegen div 100 > case_div

gcc:
	gcc casegen.c -o casegen
 

clean:
	rm -f out calc casegen case_* *.o
