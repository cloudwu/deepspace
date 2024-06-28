ROOT=..
GAME=$(ROOT)/bin/game.dll
TEST=$(ROOT)/test/game.dll
LUAINC=`pkgconf lua --cflags`
LUALIB=`pkgconf lua --libs`

all : $(GAME) $(TEST)

$(GAME) : game.c
	gcc -Wall -g --shared -o $@ $^ $(LUAINC) $(LUALIB)
	

$(TEST) : $(GAME)
	cp $< $@
