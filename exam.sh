mkdir test
cp -r ./code ./test
cat ./code/14.c
a=1
while [ $a -ne 16 ]
do
	gcc ./code/$a.c -o ./code/$a.o 
	a=$[$a+1]
done

