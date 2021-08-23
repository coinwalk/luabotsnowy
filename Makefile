all:	luabot.c
	make luabot
	
luabot: luabot.c
	gcc -o luabot luabot.c -Wall -I/usr/include/lua5.3 -llua5.3 -lcurl -lcurses -lcrypto
	strip luabot

luabot.exe: luabot.c
	gcc -o luabot luabot.c -Wall -I/usr/include/lua5.3 -llua -lcurl -lcurses -lcrypto
	strip luabot
	
termux: luabot.c
	gcc -o luabot luabot.c -Wall -I/data/data/com.termux/files/usr/include/lua5.4 -llua5.4 -lcurl -lcurses -lcrypto
	strip luabot
