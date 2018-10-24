CFLAGS = -O
CC = g++
SRC = FrequentlyUsed.cpp
OBJ = $(SRC:.cpp = .o)

FrequentlyUsed: $(OBJ)
	$(CC) $(CFLAGS) -o FrequentlyUsed $(OBJ) -lpthread

clean:
	rm -f core *.o
