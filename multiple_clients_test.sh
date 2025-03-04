#make everything
make clean > /dev/null

make > /dev/null

echo "Running Multiple Client Test..."

#start the server
./server > lvl2_test.txt &

#let the server spin up
sleep 2

#run multiple clients
./multiple_clients > /dev/null & 

#let the clients setup and connect
sleep 5

#check the output of the server
output=$(cat lvl2_test.txt | grep READER)

#if the server asserted that a reader connected, we are good
if [[ -z "$output" ]]; then
    echo "Test Failed: Server did not poll/accept reader clients"
else
    echo "Test Passed: Server polled/accepted reader clients"
fi

#if the server assert that a reader connected we are good
output=$(cat lvl2_test.txt | grep WRITER)

if [[ -z "$output" ]]; then
    echo "Test Failed: Server did not poll/accept writer clients"
else
    echo "Test Passed: Server polled/accepted writer clients"
fi

rm -rf lvl2_test.txt



