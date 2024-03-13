mkdir test
cp -r ./code ./test
cat ./code/14.c
cd test
a=0
while [ $a -ne 16 ]
do
	gcc -o ./code/$a.o -c ./code/$a.c
	a=$[$a+1]
done

