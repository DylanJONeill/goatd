all:
	@echo "Building the project..."
	gcc client.c -o client 
	gcc server.c -o server


write: 
	gcc -o db db.c -lsqlite3
	./db 
	rm -f db

clean:
	@echo "Cleaning up..."
	rm -f client 
	rm -f server
	rm -f db_server

