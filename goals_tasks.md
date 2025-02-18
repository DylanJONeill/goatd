Tasks:
1. Use UNIX domain sockets for communication with a known path for the _name_ of the socket. [Done]
2. Use `accept` to create a connection-per-client as in [here](https://gwu-cs-advos.github.io/sysprog/#communication-with-multiple-clients). [Done]
3. Use event notification (epoll, poll, select, etc...) to manage concurrency as in [here](https://gwu-cs-advos.github.io/sysprog/#event-loops-and-poll).
	- Poll for any new connections as well as exited connections.  
4. Use domain socket facilities to get a trustworthy identity of the client (i.e. user id). [Derek]
	- On connection client provides some form of their identity (whether it be user id or PID or some value we determine) [Dylan]
5. Pass file descriptors between the service and the client.
	- Providing the file descriptors from the text files from our SQL server and then allowing them to read into it (can do inputting as well later) [Faris]

Additional Notes: 
- Daemon that runs  in the background on startup
- Spins infinitely
- Run one program (service.c) at the start, and that starts up the server
- Other processes can start up (client), and that’ll connect them to the server
- Separate database file
- When the client connects, it’ll open the file
    - Implementing Flisk
- For this, you'll implement a trusted UNIX daemon that provides services to the rest of the system. Does the Daemon need to run in a specific operating system?
- inetd, init
- Different file descriptors for different permissions
- When they connect, we give them the file descriptor to the database
- Goal:
    - Set up the client server so that a user can connect to the server
    - Passing the file descriptor back to the client
    - On the same device, pass back to the client
    - Input sql query and read or write to the database
        - Insert SQL Query
    - Default read-write permissions. Once basic and tests are successful, then we can increment progress
- Additional Goals:
    - If time permits, try with multiple clients on different machines.  
