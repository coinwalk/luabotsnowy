sudo apt-get install bc libncurses-dev libcurl4-openssl-dev libssl-dev liblua5.3-dev make gcc vim git

git clone http://github.com/coinwalk/luabotsnowy/

cd luabotsnowy

make

./luabot -h 100 -n snowybot.lua
