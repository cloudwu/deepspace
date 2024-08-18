ROOT=..
GAME=$(ROOT)/bin/game.dll
TEST=$(ROOT)/test/game.dll
LUAINC=`pkgconf lua --cflags`
LUALIB=`pkgconf lua --libs`

all : $(GAME) $(TEST)

$(GAME) : scene_lua.c pathmap.c build.c container_g.c powergrid.c array.c list.c group.c handle.c
	gcc -Wall -g --shared -o $@ $^ $(LUAINC) $(LUALIB)
	

$(TEST) : $(GAME)
	cp $< $@
	
clean :
	rm -f $(GAME) $(TEST) test.exe
