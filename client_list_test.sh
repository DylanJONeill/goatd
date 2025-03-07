#make everything
make clean

make 

#start the server
./server &

#server activation window
sleep 2

#compile and start our client_list_test
gcc client_list_test.c -o client_list_test
./client_list_test

#End of test, get rid of server
pkill server

exit





