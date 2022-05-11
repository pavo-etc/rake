
CC              = cc
CFLAGS          = -O -Wall -Werror -pedantic

OBJ		= strsplit.o

rake-c:	rake-c.c $(OBJ)
	$(CC) $(CFLAGS) -o rake-c rake-c.c actionset.c $(OBJ)

%.o:	%.c
	$(CC) $(CFLAGS) -c $<


clean:
	@rm -f tester $(OBJ)
