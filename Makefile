all:
	@echo "Building the project..."
	gcc -o client client.c
	gcc -o server server.c -lsqlite3
	gcc -o multiple_clients multiple_client_test.c
	
clean:
	@echo "Cleaning up..."
	rm -f client 
	rm -f server
	rm -f db_server
	rm -f query_results.txt

install:
	sudo apt-get install libsqlite3-dev
	sudo apt install sqlite3

