## THIS SOFTWARE COMES WITH NO GUARANTEES 
## IF IT LOSES ALL YOUR COINS OR MAKES 
## YOUR TOASTER EXPLODE, DONT BLAME ME.
## USE IT AT YOUR OWN RISK ONLY.

### I CAN assure it respects your privacy so it doesn't &quot;phone
### home&quot; and of course it won't steal your coins.  There is no
### malicious intentions behind this code.  I publish it in the hope it will
### be as useful for others as it is for myself. No further guarantees.

# Luabot

## a fast gambling bot for 999dice.com

### work in progress. there is no release version yet

I've written this program for myself,
and for a long time I've been its only
user. When you use software every day
especially when you've written it and know
how it works (and how it doesn't) you might
get blind about missing features or even about
bugs. However I'm sure it could be useful for
others, too. Even in its current state,
It just needs more documentation and review.
However, I publish this today in the hope it
will help in process to make it even better,
and for that I need your feedback and wishes,
critics, I'm even open for pull requests.
So let's make this a great bot.

## How to build

### Get the Source Code
Of course you can just download a snapshot of this repository using
the download button and chose tar.gz or zip archive but due to the
the software is updated quite often and the ease of keeping track
of changes and updating to newer versions, I recommend using git.
For Linux there is most likely a package git offered in your distro.
For Windows you can get it from https://git-scm.com/downloads

#### How to use git with Tor
To use git with tor you need to configure it for using with the socks5
proxy tor provides. If you want to use the torprocess you TorBrowser
started, you can do this like this:

	git config --global http.proxy "socks5h://127.0.0.1:9150"

This way git is going to use tor to checkout the repository and 
even resolves hostnames using Tor (which is needed to have onion service
working) (Make sure your TorBrowser is still open as it provides
the socks5 proxy in this case)

The next step would be 

	git clone http://elele7lab3jug3cbuzmanp7tzqwc37f2ujlveaokmmhorbjjaf6avqad.onion:3000/Elele/luabot.git

This will create a local directory luabot with the code of this repository
To update later you would just do

	cd luabot
	git pull

For more information about using git see https://git-scm.com/doc

### Linux
Here's what you need to build luabot from sourcode:

- bc - An arbitrary precision calculator language  (used in stash script)
- libncurses-dev - developer's libraries for ncurses
- libcurl4-openssl-dev - development files and documentation for libcurl (OpenSSL flavour)
- libssl-dev - development files for openssl (for result calculation and bet verification)
- liblua5.3-dev - Development files for the Lua language version 5.3
- make - utility for directing compilation
- gcc - GNU C compiler

the package names might be slightly different in your distro

To build the binary in Linux, just run 

		make

### Android / termux
Android is a Linux too, so the above requirements still apply. Termux
already comes with most of the requirements, just run

	pkg install git build-essential liblua54
	git config --global http.proxy socks5h://127.0.0.1:9050    # requires orbot running on your android
	git clone http://elele7lab3jug3cbuzmanp7tzqwc37f2ujlveaokmmhorbjjaf6avqad.onion:3000/Elele/luabot.git
	cd luabot
	make termux


### Windows
If you run Windows, you will have to install Cygwin from https://cygwin.com 
Chose the following packages to be able to build luabot:
- libncurses-devel
- libcurl-devel
- lua-devel (5.3.5)
- libssl-devel
- make
- gcc-core
- procps-ng (pidof)
- bc
- mc
- wget

To build luabot.exe run
	make luabot.exe



## How to use luabot
example:

			./luabot -H 1000 mymethod.lua

or

			./luabot.exe -H 1000 mymethod.lua

this wil run the bot using your script mymethod.lua and it defines a stop
point as an absolute value of 1000. 

This will make the bot stop and back in case the next bet would go below the
maximum balance reached minus 1000 Doge.  When it asks back you will have
the option to continue or quit.  Why Doge?  because that's the default
currency, if you want to use another you need to to specify, if you use
doge, you don't.

## list of command line options 

### -s &lt;sessioncookie&gt;
d'oh this option is obsolete event though it would work, today the bot uses 
a file account.data to read the session cookie and asks to login if that file 
does not exist it asks to login and creates that file. just ignore -s 

### -c &lt;currency&gt;
this sets the currency. it defaults to doge, if you want to play another
currency you can specify it like this: -c btc

### -H &lt;stopamount&gt;
makes luabot stop and ask back if the nextbet would go below 
your maximum balance reached minus the given amount.
luabot goes interactive then. also see explanation about that below

### -h &lt;percent&gt;
just like -H but you specify a percentage, so to send luabot into
interactive/ask-back mode after losing 2 percent you woudl use -h2

## note: specifying a stop point is obligatory, if you don't want a stop
## point at all, specify -h 100 to have it after losing 100% 

### -S 
with this option luabot will exit instead of going interactive when
it reaches a stop point given with -H or -h 
Mainly useful for wrapper scripts.

### -n
beside the stop point specified by -H or -h luabot has a builtin 
loss cut, this is hardcoded for now and might appear strange to you
just keep in mind the program was originally not written for the world
and I personally like this behaviour. When luabot makes profit of
2000*basebet you used it will exit in case it loses half of this profit
again. so if you started with 0.01 doge bets, once it reached 20 doge profit
it wont go down more than 10 doge from that. it will continue to play, 
but it will stop, if losing next bet would go below half of current session
profit. As I use wrapper that restarts the bot after that, think of it 
as kind of some reset everything condition. This is default behaviour, 
and if you don't like it, you can disable this using the -n switch

### -O "command" 
set a command to run on going interactive or running into stop point.
This option overrides the OnStopCommand from account.data

### -P &lt;amoun&gt;
additionally to the hardcoded profit ensurance activated by profit larger
than 2000*basebet, with -P you can set an amount that, if won, activate the same
behaviour of stopping in case next bet would go below half of the profit if
lost. This is especially useful for scripts using a rather high basebet
and therefore might not reach 2000*basebet that easily.

### -N 
this option just disables the output of the bets. saves some traffic
if you run luabot remote, not very useful otherwise

### -r &lt;num&gt;
luabot traces the number of rolls and wins and puts out some statistiscs 
about it. this option exists you could initiate those counters with
values of some former session, so -r 100000 would initiate the betcounter to
10000 and

### -w &lt;num&gt;
this would initiate the wincounter with a value. so if you give -w 10000 it
would initiate the wincounter with 10000.

neither -r nor -w will influence the betting in any way, it's just about
statistics

### -t &lt;number_of_bets&gt;
with this option you can set a maximum number of bets to be done with this
call of luabot. this is not precise, actually once the betcounter reached
the given value it will exit when it reaches a new max balance. I use that
with a wrapper script that switches methods frequently.

### -g &lt;amount&gt;
this sets a goal, once balance is above this goal the bot exits

### -G &lt;amount&gt;
this sets a goal, too, but as an amount you want to win. so -G 100 would
stop after winning 100 doge 

### -k 
keep the betlog - without this option luabot renames a betlog to betlog.old 
once it exceeds 10MB and starts a new betlog to save disk space. 

### -m
enter manual mode from cmdline

## About interactive mode

When luabot runs into a stop point specified by -h or -H it goes interactive
and will show something like this:

			96% of maxbalance. Press c to continue or q for quit

if you press c now, it will do another bet following the rules of you lua
script, if that bet leads to a balance above stop point it will continue
betting otherwise you will be asked about continue or quit again
Beside pressing c or q, you can press + to increase the stop point slighty,
this will make some more bets not only one. 

When luabot enters interactive mode it's possible to run an arbitrary
command, for example to get some audio-notification, or maybe send an sms or
call a refil script - no limits what it could do. To use that feature,
you can set a variable "OnStopCommand" in you file account.data

For example if you use
	OnStopCommand="printf '\a'"
you get a terminal beep.

You can override the OnStopCommand in accoun.data by using option -O on
cmdline

### if another bot instance (or manual bets using gamble) reach former
### maximum balance while the bot is in interactive mode it 
### automatically quits
I use this with wrapper that also kills gamble
and other bots using the same account, so the original session 
takes back control as soon as the the loss is recovered.


# Keyboard usage during non-interactive betting

While luabot is running and doing bets there are some more keys that
influence its behaviour:

- x activates 'Exit On MaxBalance', so it will stop on reaching new max
- s activates the loss cut, even when not already active or disabled by -n switch
- p to pause betting
- c to continue betting after you paused
- Q will quit 

# The manual betting mode

In manual mode you can just do bets manually. It comes with a config file
manual.lua where you can define many keys to set certain values like setting
a chance a betsize etc.

Some keys are coded in C though.

- l makes a bet at current betsize and chance
- L continues betting at curent betsize until 9 consecutive reds happen
- j sets betsize to 1/60 of difference between current balance and max balance
- p will do as many bets as would be expected to see 20 hits
	for example 400 bets at 5% or 200 bets at 10% or 40 bets at 50%
	then double betsize if less than 19 hits in these bets
	but stop betting if max balance is reached
- O continues betting at current betsize until 15 consecutive reds
- H continues betting at current betsize until 5 consecutive reds
- r continues betting at current betsize for up 15 bets but stop on first win
- y continues betting at current betsize for up 6 bets but stop on first win
- e continues betting at current betsize for up 3 bets but stop on first win
- q continues betting at current betsize for up to 50 bets, but stop on win,
	if that win reaches new maxbalance resets to basebet but double
	double betsize otherwise

# manual.lua

contains some keys already doing something by default but can and should be
changed by yourself to fit your needs.
Just look into the file, I guess you get the idea, if you have any questions
about it, you know where to find me ;)

# Auto-Withdraw
If you don't want to use a wrapper script for auto-withdraw luabot now
supports a function withdraw() in Lua scripts. The function takes only
one parameter - the amount to withdraw.
Example:
		if balance>1100 then withdraw(100) end

To have this work, you either need to add a destination in file account.data
like this:

		WithdrawAddress="0000000"

or use command line parameter -A &lt;address&gt; 

# I can't access 999dice.com due to censorship at my ISP
You can either give an altenative URL by using the -U option like this:

		luabot -U https://www.999doge.com

As you probably want to do that all the time, you can even put an entry to
your your account.data file:

		Server="https://www.999doge.com"

# luabot
# luabots
