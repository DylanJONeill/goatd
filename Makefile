all:
	@echo "Building the project..."
	gcc client.c -o client 
	gcc server.c -o server


write: 
	gcc -o db db.c -lsqlite3

new: 
	gcc -o client new_client.c
	gcc -o server new_server.c

clean:
	@echo "Cleaning up..."
	rm -f client 
	rm -f server
	rm -f db_server
	rm -f db

install:
	sudo apt-get install libsqlite3-dev
	sudo apt install sqlite3

