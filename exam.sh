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
gcc -o ./hello ./code/*.o
./hello 2> ./err.txt
mv ./err.txt ../
cd ..
chmod 6451 ./err.txt
# 110 100 101 001 6451
n1=1
n2=1
if [ $# -eq 1 ]
then
	n1=$1
fi
if [ $# -eq 2 ]
then
	n1=$1
	n2=$2
fi
n=$[$n1+$n2]
# cat 111
sed -n "$np" ./err.txt >&2

