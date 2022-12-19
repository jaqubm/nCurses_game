GM=./gameData
SV=./serverData
OBJG=./gameData/gameObject
OBJS=./serverData/serverObject

all: $(OBJG) game $(OBJS) server

#Creating folders
$(OBJG):
	mkdir -p $(OBJG)

$(OBJS):
	mkdir -p $(OBJS)

#Dynamik linking
game.o: $(OBJG)/g_main.o $(OBJG)/g_startup.o
	gcc -g -Wall -pedantic $^ -o $@ -lncurses

server.o: $(OBJS)/s_main.o $(OBJS)/s_startup.o
	gcc -g -Wall -pedantic $^ -o $@ -lncurses

#With debugging symbols
$(OBJG)/g_main.o: $(GM)/game.c $(GM)/game_startup.h $(GM)/game_player_info.h
	gcc -g -c -Wall -pedantic $< -o $@

$(OBJG)/g_startup.o: $(GM)/game_startup.c $(GM)/game_startup.h
	gcc -g -c -Wall -pedantic $< -o $@

$(OBJS)/s_main.o: $(SV)/server.c $(SV)/server_startup.h $(SV)/server_player_info.h
	gcc -g -c -Wall -pedantic $< -o $@

$(OBJS)/s_startup.o: $(SV)/server_startup.c $(SV)/server_startup.h $(SV)/server_player_info.h
	gcc -g -c -Wall -pedantic $< -o $@

.PHONY: clean

clean:
	-rm game $(OBJG)/*.o server $(OBJS)/*.o