# goatd
Trusted Unix Server Daemon

# Installation
sudo apt-get install libsqlite3-dev
sudo apt install sqlite3 

# Details 
Steps to run: 
0. Ensure that you have all software installed using: `make install` 
1. Compile the database writer using: `make write` 
2. Run the database writer in one terminal session using: `./db &` 
3. Compile the server and the client using: `make` 
4. Ensure that the server is running using: `./server` 
5. Run as many clients using: `./client`

This will ensure that the server creates the domain socket that the clients can communicate with it from, and receive the file descriptor to the respective results file.

# Documentation


## Who Did What
- Derek: Setting up the Domain Socket, Accepting Clients, Polling, Testing
- Dylan: Client Validation, Testing
- Faris: Passing File Descriptors, SQLite to Text File, Testing


## Test Case Documentation
- `domain_socket_test.sh` - This tests that a domain socket is created with a known path
- 