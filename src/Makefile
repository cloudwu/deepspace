ROOT=..
LUA54=$(ROOT)/bin/lua54.dll
GAME=$(ROOT)/bin/game.dll
TEST=$(ROOT)/test/game.dll
LUAINC=`pkgconf lua --cflags`
LUALIB=`pkgconf lua --libs`

all : $(LUA54) $(GAME) $(TEST)

$(LUA54) : forward_lua.c
	gcc -Wall -O2 --shared -o $@ $^ -DTARGET_NAME=demo.exe
	
$(GAME) : game.c
	gcc -Wall -g --shared -o $@ $^ $(LUAINC) $(LUALIB)
	

$(TEST) : $(GAME)
	cp $< $@
