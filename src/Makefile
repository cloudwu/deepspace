ROOT=..
GAME=$(ROOT)/bin/game.dll
TEST=$(ROOT)/test/game.dll
LUAINC=`pkgconf lua --cflags`
LUALIB=`pkgconf lua --libs`

all : $(GAME) $(TEST)

$(GAME) : scene_lua.c pathmap.c build.c container.c powergrid.c list.c group.c handle.c machine.c
	gcc -Wall -g --shared -o $@ $^ $(LUAINC) $(LUALIB)
	

$(TEST) : $(GAME)
	cp $< $@
	
clean :
	rm -f $(GAME) $(TEST) test.exe
