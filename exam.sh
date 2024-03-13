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
chmod rw-r-xr-x ./err.txt
n1=1
n2=1
if $# -eq 1
	n1=$1
if $# -eq 2
	n1=$1
	n2=$2
n=$n1+$n2
sed -n '$np' >&2
