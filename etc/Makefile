# makefile for Lua etc

TOP= ..
LIB= $(TOP)/src
INC= $(TOP)/src
BIN= $(TOP)/src
SRC= $(TOP)/src
TST= $(TOP)/test

CC= gcc
CFLAGS= -O2 -Wall -I$(INC) $(MYCFLAGS)
MYCFLAGS= 
MYLDFLAGS= -Wl,-E
MYLIBS= -lm
#MYLIBS= -lm -Wl,-E -ldl -lreadline -lhistory -lncurses
RM= rm -f

default:
	@echo 'Please choose a target: min noparser one strict clean'

min:	min.c
	$(CC) $(CFLAGS) $@.c -L$(LIB) -linlua $(MYLIBS)
	echo "print('Hello there!')" | ./a.out

noparser: noparser.o
	$(CC) noparser.o $(SRC)/lua.o -L$(LIB) -linlua $(MYLIBS)
	$(BIN)/inluac $(TST)/hello.lua
	-./a.out inluac.out
	-./a.out -e'a=1'

one:
	$(CC) $(CFLAGS) all.c $(MYLIBS)
	./a.out $(TST)/hello.lua

strict:
	-$(BIN)/inlua -e 'print(a);b=2'
	-$(BIN)/inlua -lstrict -e 'print(a)'
	-$(BIN)/inlua -e 'f=[](b=2) f()'
	-$(BIN)/inlua -lstrict -e 'f=[](b=2) f()'

clean:
	$(RM) a.out core core.* *.o inluac.out

.PHONY:	default min noparser one strict clean
