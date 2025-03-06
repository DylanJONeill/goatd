# goatd
Trusted Unix Server Daemon

# Documentation
The `goatd` domain socket is used for communication with a sample database file, `test.db`, a SQLite 3 db. Communication is done via a named domain socket, and upon negotiation, is moved depending on if the client is a writer or a reader. 

There is no limit to the number of client writers and readers that you can run. For readers, you will gain access to the query_results.txt file via a file descriptor passed over the domain socket. Any changes and/or queries made in that file will be outputted to standard out. As for writers, you will have active communication with the server and can input any queries you want, with error checking included. Make queries to your heart content, and switch to a reader to view the results of your queries. 

Additionally, you can see what client made the SQL requests. 

# Installation
Prior to creating the server and client software, run the following:
```
sudo apt-get install libsqlite3-dev
sudo apt install sqlite3` 
```
This is to ensure that the SQLite 3 database will be linked to the server. 

# Getting Started
If you would like to run the domain socket software and test it for yourself:
1. Ensure that you have all software installed using: `make install` 
2. Compile the server and the client using: `make` 
3. Ensure that the server is running using: `./server`. This MUST be running prior to trying to run any clients, otherwise the domain socket won't instantiate. 
4. Run as many clients using `./client writer` to make queries or `./client reader` to read the output from the writers' queries. 

This will ensure that the server creates the domain socket that the clients can communicate with it from, and receive the file descriptor to the respective results file.

If you would like to plug in your own database and test it for yourself, ensure that you have the SQLite 3 command tool, with more details to install in the Installation section. 

# Implementation
Running `./server` starts a SQL server that serve incoming client connections. Using `send_pid` and `rec_pid`, clients can send their process ID and the server can validate it on their end. A client can connect as a writer or reader (cannot connect as both). If the client connects as a writer, they can send SQL query messages to the server, where the server will communicate with the database and retrieve select statements. If a client connects as a reader, they are returned a file descriptor to a text file, which serves as a live feed to the database. Whenever a writer writes to the database, it is updated to the log, where all the readers can see the current state of the database. 

A sample situtation that you can run is starting the server using `./server`. In one shell window connects as a reader using `./client reader` and in another window connect as a writer `./client writer`. In the writer window, input `select * from users` and view the reader window to see the select statement.




## Who Did What
- Derek: Setting up the Domain Socket, Accepting Clients, Polling, Testing
- Dylan: Client Validation, Testing
- Faris: Passing File Descriptors (via send_fd, recv_fd), Setup of SQLite database + Server Querying SQLite db, Writer client, Testing


## Test Case Documentation
- `domain_socket_test.sh` - This tests that a domain socket is created with a known path
- `write_read_test.sh` - Test to ensure that the writers and readers are working as expected and output queries from a sample table. 
- `mutliple_clients_test.sh` - Test to ensure that multiple clients (readers and writers) can connect to the server via the domain socket 