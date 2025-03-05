#!/bin/bash

# Clean and build everything
make clean > /dev/null
make > /dev/null

echo "Running Writer Client Test..."

# Start the server in the background
./server > lvl2_test.txt &

# Let the server spin up
sleep 2

# Create a named pipe for writer input
pipe_name="/tmp/writer_pipe"
rm -f "$pipe_name"
mkfifo "$pipe_name"

# Start the writer and redirect the named pipe to its stdin
./client writer < "$pipe_name" > writer_output.txt &  

# Wait for the writer to initialize
sleep 3  

# Send the SQL query into the named pipe
echo "SELECT * FROM users;" > "$pipe_name"

# Give it time to process
sleep 2  

# Don't care about the client PID portion, just want to make sure that the query is the same
actual_output=$(sed '1,3d' query_results.txt)  # Removes the first two lines

# Expected output
expected_output=$(cat <<EOF
id = 1
name = Alice
age = 30
id = 2
name = Bob
age = 25
id = 3
name = Charlie
age = 35
EOF
)

# Compare output
if [[ "$actual_output" == "$expected_output" ]]; then
    echo "Test Passed: Query executed and returned expected results"
else
    echo "Test Failed: Query results do not match expected output"
    echo "Expected:"
    echo "$expected_output"
    echo "Actual:"
    echo "$actual_output"
fi

# Cleanup
rm -f "$pipe_name" writer_output.txt lvl2_test.txt query_results.txt
