# # Makefile for S.H.A.M. Protocol

# CC = gcc
# CFLAGS = -Wall -Wextra -g -pthread

# all: client server

# client: client.o
# 	$(CC) $(CFLAGS) -o client client.o

# server: server.o
# 	$(CC) $(CFLAGS) -o server server.o

# client.o: client.c sham.h
# 	$(CC) $(CFLAGS) -c client.c

# server.o: server.c sham.h
# 	$(CC) $(CFLAGS) -c server.c

# clean:
# 	rm -f client server *.o





# # Makefile for S.H.A.M. Protocol

# CC = gcc
# # CFLAGS = -Wall -Wextra -g -pthread
# CFLAGS = -Wall -Wextra -Wno-deprecated-declarations -g -pthread

# all: client server

# client: client.o
# 	$(CC) $(CFLAGS) -o client client.o

# server: server.o
# 	$(CC) $(CFLAGS) -o server server.o -lcrypto

# client.o: client.c sham.h
# 	$(CC) $(CFLAGS) -c client.c

# server.o: server.c sham.h
# 	$(CC) $(CFLAGS) -c server.c

# clean:
# 	rm -f client server *.o



CC = gcc
CFLAGS = -Wall -Wextra -Wno-deprecated-declarations -g -pthread

all: client server

client: client.o
	$(CC) $(CFLAGS) -o client client.o

server: server.o
	$(CC) $(CFLAGS) -o server server.o -lcrypto

client.o: client.c sham.h
	$(CC) $(CFLAGS) -c client.c

server.o: server.c sham.h
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f client server *.o