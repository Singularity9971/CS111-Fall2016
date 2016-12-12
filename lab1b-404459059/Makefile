default:
	gcc -o client client.c -lmcrypt -pthread -g
	gcc -o server server.c -lmcrypt -pthread -g
client:
	gcc -o client client.c -lmcrypt -pthread -g
server:
	gcc -o server server.c -lmcrypt -pthread -g
clean:
	@rm *.gz server client
dist:
	@tar -czf lab1b-404459059.tar.gz server.c client.c Makefile README my.key
