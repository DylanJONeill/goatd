#make everything
make clean

make 

#start the server
./server &

#let the server spin up
sleep 2


#check if a socket was create
ls | grep db_server >> lvl1.txt


if diff lvl1.txt lvl1_test.txt; then
    echo "Level 1 Passed"
else
    echo "Level 1 Failed: No Socket"
fi

rm -rf lvl1.txt







