/* a simple bot for 999dice to run dicebot style lua scripts 
    Copyright (C) 2020 Elele <echo "ZWxlbGVAc2VjbWFpbC5wcm8K" | base64 -d>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <curl/curl.h>
#include <fcntl.h>
#include <inttypes.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <ncurses.h>
#include <openssl/sha.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

bool	alarmOn = 1;
char	autowd[200]="";
int64_t balance = 0;	// balance in Satoshi
double  basebet = 0;	// used for stop point calc
int64_t BasePayIn=0;
struct timeval begin, end, now;
int betcount = 0;	// Bets done in this session
int BetCount=0;		// this is for server replies on autobet, not to confuse with betcount
int64_t BetId = 0;		// from server reply
WINDOW *betregion;	// ncurses region for bet output
int64_t before=0;
double	bmax = 0;
double	bnow = 0;
char	bnowfile[12];
char	botname[40];
int64_t bprofit = 0;
double	bstart = -1;	// start balance;
double	chance = 5.0;
int	color = 0;
int	color2 = 3;
CURL *curl;
char	currency[5] = "doge";
double	currentprofit = 0;	// profit of last bet
int	currentstreak = 0;	// if positive, winning streak, if negative, losing streak
char	cwd[PATH_MAX];
char	dirname[25];
int	exitOnMax = 0;
int	exitOnMismatch = 0;
char	exitReason[80] = "";
int64_t gain;
double	globalmax = 0;
double	globalmin;
double	globalprofit = 0;
double	goal = 0;	// when to count a run as success
double	goalDelta=0;
int	hardstop=0;
int	height, width;
int	heightavail;
int	high;
int	High = 0;
double	IncreaseOnLosePercent=0.0;
double	IncreaseOnWinPercent=0.0;
double	infoval = 0.0;
char	kbd = ' ';
int	keeplogfile=0;
lua_State *L;
lua_State *Lman;
int	lastmax = 0;
int	maxincrcount=0;
int	lastmaxmax = 0;
int	laststreak = 0;
FILE *logfile;
int	losecount = 0;
int	low;
int	Low = 0;
int	luaopen=0;
int	manualSession=0;
int	manualActive=0;
int	manualline=0;
int64_t	maxbalance = 0;
int	maxbets = 0;
int	MaxBets=0;	// for PlaceAutomatedBets (max.200)
int	maxcount = 0;
int64_t	maxdiff = 0;
int64_t	MaxPayIn = 0;
int64_t	AutoMaxPayIn = 0;
int	maxstreak = 0;
int64_t	minbalance = 9999999999999999;
int	minstreak = 0;
WINDOW *msgregion;
double	nextbet = 0.00001;
int	nobetdisplay = 0;
int	noprofitensure = 0;
double	oncein=1;
bool	OnStopCmd=1;
bool	overrideOnStopCmd=0;
bool	overrideAutowd=0;
bool	overrideServer=0;
char	OnStopCommand[256];
int64_t	PayIn = 0;
int64_t	PayOut = 0;
double	percentualStop=0;
int64_t profitensurance = 0;
double	previousbet = 0;
double	sessionprofit = 0; // session profit
int	recovery=0;
char	request[300];
bool	ResetOnLose=0;
bool	ResetOnLoseMaxBet=0;
bool	ResetOnWin=0;
int	secret = 0;
char	servername[128] = "https://www.999dice.com";
char	serverseed[65];
char	nextserverseed[65]="";
int32_t	clientseed;
char	sessioncookie[33] = "00000000000000000000000000000000";
int	show = 1000000;
int64_t	StartingBalance;
int64_t	StartingPayIn=0;
WINDOW *statregion;
int64_t	stopamount = 200000000;
int	stopbetting = 0;
int	stopifwin=0;	// stop if win in interactive mode
int	profitensure = 0;
int64_t	StopMinBalance=0;
int64_t	StopMaxBalance=0;
int	StopOnLoseMaxBet=0;
int	streakwin[512];
int	streaklose[512];
int	seconds;
int	oldseconds;
int	streaks[300];
int	streamprefix=0;	// for bnow file (and feed driveGnuPlots.pl)
struct termios term, term_orig;
char	termtitle[80] = "";
int64_t	TmpMaxPayIn = 0;
char	url[200];
int	turnlimit = 0;
char	version[15] = "luabot/0.3";
int64_t	wagered = 0;
int64_t wdsum=0;
int	win;
int	wincount = 0;
int	WinCount=0;	// for AutoBet requests
int Bet (double betsize);
int msg(lua_State *L);
void updateStatus();
// for curl
struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback (void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *) userp;

  mem->memory = realloc (mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */
    printf ("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy (&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

// make lua errors exit program
void bailout (lua_State * L, char *msg, int code) {
  mvprintw(height,0,"\n");
  endwin ();
  fprintf (stderr, "\nFATAL ERROR:\n  %s: %s\n\n", msg, lua_tostring (L, -1));
  lua_close (L);
  exit (code);
}

// the stop() function for Lua scripts
int stoplua (lua_State * L) {
  bailout(L, "\nstop() called\n",5);
  return 1; // not reached
}

void verifySeed(char *serverseed, char *hash) {
  char binaryseed[32];
  char binaryhash[32];
  unsigned char hashcalc[32];
  char hashstr[65];
  SHA256_CTX ctx;

  char * spos = serverseed;
  for (int i=0;i<32;i++) {
      sscanf(spos,"%2hhx",&binaryseed[i]);
      spos+=2;
  }
  spos = hash;
  for (int i=0;i<32;i++) {
      sscanf(spos,"%2hhx",&binaryhash[i]);
      spos+=2;
  }
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, binaryseed, 32);
  SHA256_Final(hashcalc, &ctx);

  sprintf(hashstr,"%02x",hashcalc[0]);  // convert hashresult back to str
  for (int p=1;p<32;p++) {
    char tmpstr[3];
    sprintf(tmpstr,"%02x",hashcalc[p]);
    strcat(hashstr,tmpstr);
  }

  if (strcmp(hashstr,hash)) {
    wprintw(msgregion,"BetId %"PRIi64": server seed sha256sum mismatch?\n",BetId);
    wprintw(msgregion,"  expected: %s\n",hash);
    wprintw(msgregion,"calculated: %s\n",hashstr);
    wprintw(msgregion,"Did you or another bot bet on the same account?\n");
    wrefresh(msgregion);
    if (exitOnMismatch==1) bailout(L, "server seed hash mismatch in BetId %"PRIi64,BetId);
  }
}
int Withdraw(lua_State * L){
  int64_t wdamount = (int64_t) (lua_tonumber(L,1)*100000000);
  wdsum+=wdamount;
  if (strlen(autowd)>0) {
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc (1);
      chunk.size = 0;
      if (curl) {
        sprintf (request,"a=Withdraw&s=%s&Amount=%lu&Address=%s&Currency=%s",sessioncookie,wdamount,autowd,currency);
        curl_easy_setopt (curl, CURLOPT_URL, url);
        curl_easy_setopt (curl, CURLOPT_POSTFIELDS, request);
        curl_easy_setopt (curl, CURLOPT_USERAGENT, version);
        curl_easy_setopt (curl, CURLOPT_TIMEOUT, 60L);
        curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *) &chunk);
        res = curl_easy_perform (curl);

        if (res != CURLE_OK) {
          // this is a critical error so better bailout
          endwin();
          for (int n=0;n<height;n++) printf("\n");
          fprintf (stderr, "Withdraw(): curl_easy_perform() error %i: %s\n", res,curl_easy_strerror (res));
          exit(9);
        } else {
          // chunk.memory has result
          // and it looks like {"Pending":1234567890}
          if (strstr (chunk.memory, "Pending")) {
            wattron (msgregion, COLOR_PAIR(5));
            wprintw (msgregion, "%12i: ", betcount);
            wattroff (msgregion, COLOR_PAIR(5));
            wprintw(msgregion,"Withdraw to %s: %s\n",autowd,chunk.memory);
            balance-=wdamount;
            maxbalance=balance;
            wrefresh(msgregion);
            lua_pushnumber (L, (long double) (balance / 1e8));
            lua_setglobal (L, "balance");
            logfile = fopen (bnowfile, "a+");
            fprintf (logfile, "%i:%.8f\n", streamprefix, (double) balance / 1e8);
            fclose (logfile);
          } else {
            wattron (msgregion, COLOR_PAIR(5));
            wprintw (msgregion, "%12i: ", betcount);
            wattroff (msgregion, COLOR_PAIR(5));
            printf("Withdraw failed. Server replied: %s\n",chunk.memory);
            wrefresh(msgregion);
            
            endwin();
            exit(9);
          }
        }
      }
    } else {
      wprintw(msgregion,"withdraw() failed - no address given\n");
      wrefresh(msgregion);
    }
    return(1);
}


int BetResult(char *serverSeed, int clientSeed, int betNumber) {
    char clientstr[9];
    char betnumstr[9];
    unsigned char binaryseed[40];
    unsigned char hash[64];
    int result;
    int i;
    SHA512_CTX ctx;

    sprintf(clientstr,"%08x",clientSeed);
    sprintf(betnumstr,"%08x",betNumber);

    char * spos = serverSeed;
    for (i=0;i<32;i++) { // ServerSeed [32] + ClientSeed [4] + Betnumber [4]
        sscanf(spos,"%2hhx",&binaryseed[i]);
        spos+=2;
    }
    spos = clientstr;
    for (i=0;i<4;i++) {
        sscanf(spos,"%2hhx",&binaryseed[32+i]);
        spos+=2;
    }
    spos = betnumstr;
    for (i=0;i<4;i++) {
        sscanf(spos,"%2hhx",&binaryseed[36+i]);
        spos+=2;
    }
    SHA512_Init(&ctx);  // Double SHA2-512 hash the result 
    SHA512_Update(&ctx, binaryseed, 40); // ServerSeed [32] + ClientSeed [4] + Betnumber [4]
    SHA512_Final(hash, &ctx);

    SHA512_Init(&ctx);
    SHA512_Update(&ctx, hash, 64); // second SHA2-512 hash
    SHA512_Final(hash, &ctx);
    while (1) {
        int pos=0;
        for (pos=0;pos<=61;pos+=3) { // "Keep taking groups of 3 bytes and converting to an integer"
            result=(hash[pos]<<16) | (hash[pos+1]<<8) | (hash[pos+2]);
            if (result<16000000) {  // "until a value less than 16 million is found."
                return result%1000000;  // "the modulus of 1 million is the result"
            }
        }
        SHA512_Init(&ctx); // "If you run out of bytes, hash it again and start over."
        SHA512_Update(&ctx, hash, 64);
        SHA512_Final(hash, &ctx);
    }
}



int getBalanceFromFile () {
  FILE *fd;
  sprintf(bnowfile,"./bnow.%s",currency);
  static const long max_len = 55 + 1;
  char buff[max_len + 1];
  if ((fd = fopen (bnowfile, "rb")) != NULL) {
    fseek (fd, -max_len, SEEK_END);
    fread (buff, max_len - 1, 1, fd);
    fclose (fd);
    buff[max_len - 1] = '\0';
    char *last_newline = strrchr (buff, '\n');
    char *last_space = strrchr (buff, '.');
    char *last_line = last_newline + 1;
    char *last_balance = last_line+2;
    balance = strtod (last_balance, &last_space - 1) * 1e8;
  }
  return 0;
}

// GetBalance from API or bnow file if that recently changed
int getBalance() {
  struct stat attrib;
  int mtime;
  char *token;
  CURLcode res;
  struct MemoryStruct chunk;
  sprintf(bnowfile,"./bnow.%s",currency);
  stat(bnowfile,&attrib);
  mtime=attrib.st_mtime;
  gettimeofday(&now,(struct timezone *)0);
  if ((mtime+120)<now.tv_sec) {
    //  if bnow file is older than 2 minutes we use API GetBalance to have a reliable value
    chunk.memory = malloc (1);    /* will be grown as needed by the realloc above */
    chunk.size = 0;               /* no data at this point */
    if (curl) {
      sprintf (request,"a=GetBalance&s=%s&Currency=%s",sessioncookie,currency);
      curl_easy_setopt (curl, CURLOPT_URL, url);
      curl_easy_setopt (curl, CURLOPT_POSTFIELDS, request);
      curl_easy_setopt (curl, CURLOPT_USERAGENT, version);
      curl_easy_setopt (curl, CURLOPT_TIMEOUT, 60L);
      curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
      curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *) &chunk);
      res = curl_easy_perform (curl);

      if (res != CURLE_OK) {
        // this is a critical error so better bailout
                 endwin();
                 for (int n=0;n<height;n++) printf("\n");
                 fprintf(stderr,"getBalance():curl_easy_perform() error %i: %s\e[K\n", res, curl_easy_strerror (res));
                 exit(9);
      } else {
        // chunk.memory has result
        // and it looks like {"Balance":1234567890}
        if (strstr (chunk.memory, "Balance")) { 
          token = strtok (chunk.memory, ":");
          token = strtok (NULL, "}");
          balance=strtoll(token,NULL,10);
        } else {
          printf("GetBalance failed. Server replied: %s\n",chunk.memory);
          endwin();
          exit(9);
        }
      }
    }
  } else getBalanceFromFile(); // bnow recently changed so we rely on its content
  return 0;
}

// when receiving signal USR1 exit on max
void signalHandlerUSR1 () {
  exitOnMax = 1;
  wattron (msgregion, COLOR_PAIR(7));
  wprintw (msgregion, "%12i: received SIGUSR1 - going to exit with next max balance", betcount);
  wattroff (msgregion, COLOR_PAIR(5));
}

void betlog(char *line) {
  struct stat st;
  logfile = fopen ("./betlog", "a+");
  fprintf (logfile, "%s\n", line);
  fclose (logfile);  
  stat("./betlog",&st);
  if ((st.st_size>10485760) && (keeplogfile==0)) { // dont grow betlog forever
    rename("./betlog","./betlog.old");
  }
}


void keypress (char *f) {
  lua_pushnumber (Lman, (long double) balance/1e8);
  lua_setglobal (Lman, "balance");

  lua_pushnumber (Lman, (double) maxbalance/1e8);
  lua_setglobal (Lman, "maxbalance");

  lua_pushnumber (Lman, secret);
  lua_setglobal (Lman, "secret");

  lua_pushboolean (Lman, win);
  lua_setglobal (Lman, "win");

  lua_pushnumber (Lman, sessionprofit);
  lua_setglobal (Lman, "profit");

  lua_pushnumber (Lman, currentprofit);
  lua_setglobal (Lman, "currentprofit");

  lua_pushnumber (Lman, currentstreak);
  lua_setglobal (Lman, "currentstreak");

  lua_pushnumber (Lman, nextbet);
  lua_setglobal (Lman, "previousbet");

  lua_pushnumber (Lman, betcount);
  lua_setglobal (Lman, "bets");

  lua_pushnumber (Lman, wincount);
  lua_setglobal (Lman, "wins");

  lua_pushnumber (Lman, losecount);
  lua_setglobal (Lman, "losses");

  lua_pushnumber (Lman, chance);
  lua_setglobal (Lman, "chance");

  lua_pushnumber (Lman, nextbet);
  lua_setglobal (Lman, "nextbet");

  lua_pushnumber (Lman, basebet);
  lua_setglobal (Lman, "basebet");

  lua_pushcfunction (Lman, msg);
  lua_setglobal(Lman, "print");

  lua_getglobal (Lman, f);
  if (lua_pcall (Lman, 0, 0, 0))
    bailout (Lman, "lua_pcall() failed",1);

  lua_getglobal (Lman, "nextbet");
  nextbet = (long long) (lua_tonumber (Lman, -1)*1e8) / 1e8;

  lua_getglobal (Lman, "basebet");
  basebet = lua_tonumber (Lman, -1);

  lua_getglobal (Lman, "info");
  infoval = lua_tonumber (Lman, -1);

  lua_getglobal (Lman, "chance");
  chance = lua_tonumber (Lman, -1);

  lua_settop (Lman, 0);
}

int manualMode() {
  // in manual mode most keys are defined via manual.lua
  // if luabot is not called with -m to enter manual betting
  // then this function will exit as soon as a new max balance
  // is reached (i.e. the loss of the script that run in to stop point
  // is recovered)
  int cred;
  int localcount;
  manualActive=1;
  Lman = luaL_newstate ();
  luaL_openlibs (Lman);
  before=balance;
  if (manualSession==1) {
    if (strcmp(currency,"doge")==0) nextbet=0.01;
    if (strcmp(currency,"ltc")==0) nextbet=0.0001;
    basebet=nextbet;
  } else {
    recovery=1;
  }
  updateStatus();
  if (luaL_loadfile (Lman, "manual.lua"))
    bailout (Lman, "luaL_loadfile() failed",1);
  if (lua_pcall (Lman, 0, 0, 0))
    bailout (Lman, "lua_pcall() failed",1);

  while ((kbd != 'Q') && (kbd != 'D') && (kbd!='X')) {
    if ((exitOnMax==1) && (balance>=maxbalance)) {
      kbd='Q';
    }
    attron(COLOR_PAIR(5));
    if (chance>=10.0) mvprintw (manualline,30,"%.2f%%",chance);
    else  mvprintw (manualline,30," %.2f%%",chance);
    mvprintw (manualline,43,"%15.8f",nextbet);
    if (balance>before) mvprintw (manualline,69,"+%.8f",(float) (balance-before)/1e8);
    else mvprintw (manualline,69,"%.8f",(float) (balance-before)/1e8);
    attroff(COLOR_PAIR(5));
    refresh();
    timeout (5);
    lrand48 ();
    kbd = getch ();
    gettimeofday(&now,(NULL));
    srand48 (time (NULL) + now.tv_usec);
    switch (kbd) {
      case 'l':
        attron(COLOR_PAIR(5));
        if (chance>=10.0) mvprintw (manualline,30,"%.2f%%",chance);
          else  mvprintw (manualline,30," %.2f%%",chance);
        mvprintw (manualline,43,"%15.8f",nextbet);
        mvprintw (manualline,59,"·");
        attroff(COLOR_PAIR(5));
        refresh();
        Bet(nextbet);
        mvprintw (manualline,59," ");
        refresh();
        break;
      case 'L':
        move(manualline,0);
        clrtoeol();
        refresh();
        cred=9;
        while ((cred==9) || (currentstreak%9!=0))  {
          Bet(nextbet);
          if (balance==maxbalance) cred=5;
          if (PayOut==0) cred--;
          else cred=9;
          if ((recovery) && (balance==maxbalance)) { cred=0 ;  currentstreak=-9; }
        }
        break;
      case 'j': nextbet=(float) (maxbalance-balance)/6000000000; break;
      case 'p':
        move(manualline,0);
        clrtoeol();
        before=balance;
        refresh();
        cred=100/chance*20;
        localcount=0;
        while (cred>0) {
          Bet(nextbet);
          if (PayOut>0) { 
            localcount++;
            if ((recovery) && (balance==maxbalance)) cred=0;
          }
          cred--;
          if ((balance==maxbalance) && (nextbet>basebet) ) {
            cred=0;
            nextbet=(float) balance/1000000000000;
            basebet=nextbet;
          }
        }
         attron(COLOR_PAIR(5));
         if (localcount==1) mvprintw (manualline,20,"1 win  ");
         else mvprintw (manualline,20,"%i wins ",localcount);
         attroff(COLOR_PAIR(5));
         if ((localcount<19) && (balance<maxbalance)) nextbet*=2;
        break;
      case 'O':
        move(manualline,0);
        clrtoeol();
        refresh();
        cred=15;
        while ((cred==15) || (currentstreak%15!=0))  {
          Bet(nextbet);
          if (balance==maxbalance) cred=5;
          if (PayOut==0) cred--;
          else cred=9;
          if ((recovery) && (balance==maxbalance)) currentstreak=-15;
        }
        break;
      case 'o':
        move(manualline,0);
        clrtoeol();
        before=balance;
        refresh();
        cred=20;
        localcount=0;
        while (cred>0) {
          Bet(nextbet);
          if (PayOut>0) { 
            localcount++;
            if ((recovery) && (balance==maxbalance)) cred=0;
          }
          cred--;
          if ((balance==maxbalance) && (nextbet>basebet) ) {
            cred=0;
            basebet=nextbet;
          }
        }
         attron(COLOR_PAIR(5));
         if (localcount==1) mvprintw (manualline,20,"1 win  ");
         else mvprintw (manualline,20,"%i wins ",localcount);
         attroff(COLOR_PAIR(5));
        break;
      case 'H':
        move(manualline,0);
        clrtoeol();
        refresh();
        cred=5;
        while ((cred==5) || (currentstreak%5!=0))  {
          Bet(nextbet);
          if (balance==maxbalance) cred=5;
          if (PayOut==0) cred--;
          else cred=9;
          if ((recovery) && (balance==maxbalance)) { cred=0 ; currentstreak=-5; }
        }
        break;
      case 'r': // up to 15 rolls but stop when win
        move(manualline,0);
        clrtoeol();
        refresh();
        cred=15;
        while (cred>0) {
          Bet(nextbet);
          if (PayOut>0) cred=0;
          else cred--;
        }
        break;
      case 'y': // up to 6 rolls but stop when win
        move(manualline,0);
        clrtoeol();
        refresh();
        cred=6;
        while (cred>0) {
          Bet(nextbet);
        if (PayOut>0) cred=0;
          else cred--;
        }
        break;
      case 'e': // up to 3 bets until win
        move(manualline,0);
        clrtoeol();
        refresh();
        cred=3;
        while (cred>0) {
          Bet(nextbet);
          if (PayOut>0) cred=0;
          else cred--;
        }
        break;
      case 'q':
        move(manualline,0);
        clrtoeol();
        refresh();
        cred=50;
        while (cred>0) {
          Bet(nextbet);
          if (PayOut>0) {
            cred=0;
            if (balance==maxbalance) nextbet=basebet;
          } else cred--;
        }
        if (balance<maxbalance) nextbet*=2;
      break;
      // lots of keys to be configured in Lua code
      case '1': keypress("key_1"); break;
      case '2': keypress("key_2"); break;
      case '3': keypress("key_3"); break;
      case '4': keypress("key_4"); break;
      case '5': keypress("key_5"); break;
      case '6': keypress("key_6"); break;
      case '7': keypress("key_7"); break;
      case '8': keypress("key_8"); break;
      case '9': keypress("key_9"); break;
      case '0': keypress("key_0"); break;

//      case 'q': keypress("key_w"); break;
      case 'w': keypress("key_w"); break;
//      case 'e': keypress("key_w"); break;
//      case 'r': keypress("key_w"); break;
      case 't': keypress("key_t"); break;
//      case 'y': keypress("key_y"); break;
      case 'u': keypress("key_u"); break;
      case 'i': keypress("key_i"); break;
//      case 'o': keypress("key_o"); break;
//      case 'p': keypress("key_p"); break;
      case '[': keypress("key_bracketleft"); break;
      case ']': keypress("key_bracketright"); break;

      case 'a': keypress("key_a"); break;
      case 'A': keypress("key_A"); break;
      case 's': keypress("key_s"); break;
      case 'd': keypress("key_d"); break;
      case 'f': keypress("key_f"); break;
      case 'g': keypress("key_g"); break;
      case 'h': keypress("key_h"); break;
//      case 'j': keypress("key_j"); break;
      case 'k': keypress("key_k"); break;
//      case 'l': keypress("key_l"); break;
      case ';': keypress("key_semicolon"); break;
      case 39: keypress("key_apostrophe"); break;
      case '#': keypress("key_numbersign"); break;
      case 'z': keypress("key_z"); break;
      case 'x': keypress("key_x"); break;
      case 'c': keypress("key_c"); break;
      case 'v': keypress("key_v"); break;
      case 'b': keypress("key_b"); break;
      case 'n': keypress("key_n"); break;
      case 'm': keypress("key_m"); break;
      case ',': keypress("key_comma"); break;
      case '.': keypress("key_dot"); break;
      case '/': keypress("key_slash"); break;
    }
  }
  manualActive=0;
  return(1);
}
int InteractiveMode() {
  // ask back to user what to do next
  if ((OnStopCmd==1) && (alarmOn==1)) { // run OnStopCmd once
    system(OnStopCommand);
    alarmOn=0;
  }
  goal=(double) maxbalance/1e8;
  timeout (5);
  char ekey = ' ';
  while ((ekey != 'q') &&
      (ekey != 'c') &&
      (ekey != 'm') &&
      (ekey != '+') &&
      (ekey != 'w')) {
    lrand48 ();
    ekey = getch ();
    srand48 (time (NULL) + 327980);
    if (hardstop==1) ekey='q';
    //       ekey='q';
    getBalanceFromFile();
    if (balance > maxbalance) {
      ekey = 'q';
    }
  }
  if (ekey == 'q') {
    kbd = 'Q';
    return 0;
  }
  if (ekey == 'm') { 
    manualMode();
    return 0;
  }
  if (ekey == 'w') stopifwin=1;
  if (ekey == 'c') stopifwin=0;

  if (ekey == '+') {
    stopamount = stopamount*110/100;
  }
  return(0);
}


int Bet(double betsize) {
  int c;
  char *token;
  CURLcode res;
  struct MemoryStruct chunk;
  PayIn = (int64_t) (betsize * 1e8);
  if (PayIn > MaxPayIn)
    MaxPayIn = PayIn;
  if (PayIn > TmpMaxPayIn)
    TmpMaxPayIn = PayIn;
  if (noprofitensure == 0) {
    if ((balance > (bstart * 2)) && (bstart > 0)) {
      profitensure = 1;
      attron (A_BOLD);
      attron (COLOR_PAIR (3));
      mvprintw (1, 29, "STOP2 @");
      attroff (COLOR_PAIR (3));
      attroff (A_BOLD);
    }

    if ((maxdiff > 2000 * basebet*1e8) && (balance > (bstart + 2 * maxdiff))
        && (bstart > 0)) {
      profitensure = 1;
      attron (A_BOLD);
      attron (COLOR_PAIR (3));
      mvprintw (1, 29, "STOP3 @");
      attroff (COLOR_PAIR (3));
      attroff (A_BOLD);
    }
    if ((maxbalance - 2000 * basebet*1e8 > bstart) && (bstart > 0)) {
      profitensure = 1;
      attron (A_BOLD);
      attron (COLOR_PAIR (3));
      mvprintw (1, 29, "STOP4 @");
      attroff (COLOR_PAIR (3));
      attroff (A_BOLD);
    }
    if ((profitensurance>0) && (maxbalance-profitensurance>bstart) && (bstart>0)) {
      profitensure = 1;
      attron (A_BOLD);
      attron (COLOR_PAIR (3));
      mvprintw (1, 29, "STOP5 @");
      attroff (COLOR_PAIR (3));
      attroff (A_BOLD);
    }
  }

  if (manualActive==0) {
    if (((double) balance / 1e8 >= goal) && (goal > 0)) {
      sprintf(exitReason, "GOAL REACHED %.8f >= %.8f",(double) (balance) / 1e8, goal);
      kbd='Q';
      return 0;
    }
    if ((balance - PayIn) < ((bstart + maxbalance) / 2) && (profitensure == 1)) {
      sprintf (exitReason, "PROFIT ENSURANCE %.8f < %.8f",
               (double) (balance - PayIn) / 1e8,
               (double) (balance + maxbalance) / 200000000);
      kbd = 'Q';
      return 0;
    }
    if (((balance - PayIn < maxbalance - stopamount) && (stopamount > 0)
        && (maxbalance > 0)) || stopifwin==1) {
  //      kbd='D';
  //      return 0;
      wattron(betregion,COLOR_PAIR(5));
      wprintw (betregion,"\nNext bet: %.8f - now at %i%% of maxbalance. Press ", betsize, balance * 100 / maxbalance);
      wattroff(betregion,COLOR_PAIR(5));
      wattron(betregion,COLOR_PAIR(2));
      attron (A_BOLD);
      wprintw (betregion,"c");
      attroff (A_BOLD);
      wattroff(betregion,COLOR_PAIR(2));
      wattron(betregion,COLOR_PAIR(5));
      wprintw (betregion,"ontinue / ");
      wattroff(betregion,COLOR_PAIR(5));
      wattron(betregion,COLOR_PAIR(2));
      attron (A_BOLD);
      wprintw (betregion,"q");
      attroff (A_BOLD);
      wattroff(betregion,COLOR_PAIR(2));
      wattron(betregion,COLOR_PAIR(5));
      wprintw (betregion,"uit / ");
      wattroff(betregion,COLOR_PAIR(5));
      wattron(betregion,COLOR_PAIR(2));
      attron (A_BOLD);
      wprintw (betregion,"m");
      attroff (A_BOLD);
      wattroff(betregion,COLOR_PAIR(2));
      wattron(betregion,COLOR_PAIR(5));
      wprintw (betregion,"anual\n");
      wattroff(betregion,COLOR_PAIR(5));
      exitOnMax = 1;
      wrefresh (betregion);
      if (balance < bstart || stopifwin==1) {
        InteractiveMode();
        if (balance>bstart) {
          kbd = 'Q';
          return 0;
        }
      } else { // still in profit
        kbd = 'Q';
        return 0;
      }
    }
  } // not manual
  wagered += PayIn;
  chunk.memory = malloc (1);    /* will be grown as needed by the realloc above */
  chunk.size = 0;               /* no data at this point */
  if (curl) {
    c = (int) (chance * 10000 - 1);
    Low = abs ((int) mrand48 ()) % (999999 - c);
    clientseed = mrand48 ();
    High = Low + c;
    sprintf (request,
             "a=PlaceBet&s=%s&PayIn=%"PRIi64"&Low=%i&High=%i&ClientSeed=%"PRIi32"&Currency=%s&ProtocolVersion=2",
             sessioncookie, PayIn, Low, High, clientseed, currency);
    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_POSTFIELDS, request);
    curl_easy_setopt (curl, CURLOPT_USERAGENT, version);
    curl_easy_setopt (curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *) &chunk);
    res = curl_easy_perform (curl);
    for (int i=15;i<43;i++) request[i]='x';
    betlog(request);
    mvprintw (manualline,60," ");

    if (res != CURLE_OK) {
      endwin();
      for (int n=0;n<height;n++) printf("\n");
      fprintf (stderr, "Bet(): curl_easy_perform() error %i: %s\n", res,curl_easy_strerror (res));
      exit(9);
    }
    else {
      // chunk.memory has result
      // looks like this:
      // {"BetId":129898866219,"PayOut":24047912075,"Secret":862172,"StartingBalance":578920384883,"ServerSeed":"3463f9c5d513a429f95d0d05c2b588f8d1edc88b81e5c9d263c045d6f97e6859","Next":"4de685c66dc78356c85ed82f6663075edee13c855ebd5827d42c1456b3c33062"}
      betlog(chunk.memory);

      if (strstr (chunk.memory, "BetId")) {     // → bet successful
        token = strtok (chunk.memory, ":");     // BetId
        token = strtok (NULL, ",");
        BetId = strtoll (token, NULL, 10);

        token = strtok (NULL, ":");     // PayOut
        token = strtok (NULL, ",");
        PayOut = strtoll (token, NULL, 10);

        token = strtok (NULL, ":");     // Secret
        token = strtok (NULL, ",");
        secret = strtoll (token, NULL, 10);

        token = strtok (NULL, ":");     // StartingBalance
        token = strtok (NULL, ",");
        StartingBalance = strtoll (token, NULL, 10);
        if (bstart == -1)
          bstart = StartingBalance;

        token = strtok (NULL, ":");     // ServerSeed
        token = strtok (NULL, ",");
        strncpy(serverseed,token+1,64);
        if (strlen(nextserverseed)>0) verifySeed(serverseed,nextserverseed);

        token = strtok (NULL, ":");     // Next
        token = strtok (NULL, "}");
        strncpy(nextserverseed,token+1,64);

        // verify secret
        int bres=BetResult(serverseed,clientseed,0);
        if (bres!=secret) {
          wprintw(msgregion,"result calculation mismatch:\n");
          wprintw(msgregion,"BetId: %"PRIi64", expected: %i but server presents %i\n",BetId,bres,secret);
          // this shouldn't happen at all. let's exit
          bailout(L, "result mismatch in BetId %"PRIi64"?",BetId);
          wrefresh(msgregion);
        }

        balance = StartingBalance - PayIn + PayOut;
        bprofit = PayOut - PayIn;
        currentprofit = PayOut - PayIn;
        if (PayOut > 0) { // win
          color = 1;
          color2 = 1;
          win = 1;
          if (currentstreak < 0) {
            laststreak = currentstreak;
            streaklose[abs(currentstreak)]++;
            currentstreak = 1;
            oncein=100/chance;
          } else {
            currentstreak++;
            oncein*=100/chance;
            if ((oncein+0.9>=8000)) {
              if ((nobetdisplay==0) || (manualActive==1)) {
                wattron (msgregion,COLOR_PAIR(5));
                wprintw (msgregion, "%12i: %.8f %.8f streak %3i (once in %lu)\n", betcount,
                       (double) balance / 1e8,
                       (double) gain / 1e8, currentstreak,
                       (long long) (oncein+0.9));
                wattroff (msgregion,COLOR_PAIR(5));
                wrefresh (msgregion);
              }
            }
          }
          wincount++;
        } else {  // lose
          color2 = 3;
          win = 0;
          if (currentstreak > 0) {
            laststreak = currentstreak;
            streakwin[currentstreak]++;
            currentstreak = -1;
            oncein=100/(100-chance);
          }
          else {
            currentstreak--;
            oncein*=100/(100-chance);
            if ((oncein+0.9>=8000)) {
              if ((nobetdisplay==0) || (manualActive==1)) {
                wattron (msgregion,COLOR_PAIR(5));
                wprintw (msgregion, "%12i: %.8f %.8f streak %3i (once in %lu)\n", betcount,
                       (double) balance / 1e8,
                       (double) gain / 1e8, currentstreak,
                       (long long) (oncein+0.9));
                wattroff (msgregion,COLOR_PAIR(5));
                wrefresh (msgregion);
              }
            }
          }
         losecount++;

        }
        if (currentstreak < minstreak) minstreak = currentstreak;
        if (currentstreak > maxstreak) maxstreak = currentstreak;
        betcount++;
        if (balance >= maxbalance) {
          gain = balance - maxbalance;
          maxbalance = balance;
          if (percentualStop>0) stopamount=balance*percentualStop/100;
          maxbets = betcount - lastmax;
          maxincrcount++;
          wattron (betregion, A_BOLD);
/*          if (nobetdisplay == 0) {
            wprintw (msgregion, "%i %.8f %.8f %3i\n", ++maxcount,
                     (double) balance / 1e8,
                     (double) gain / 1e8, laststreak);
            wrefresh (msgregion);
          } */
          TmpMaxPayIn = 0;
          lastmax = betcount;
          if ((exitOnMax)
              || ((turnlimit > 0) && (betcount > turnlimit))) {
            strcpy (exitReason, "EXIT on MAX requested");
            kbd = 'Q';
          }

        }
        if (balance < minbalance)
          minbalance = balance;
        if ((maxbalance - balance) > maxdiff)
          maxdiff = maxbalance - balance;
        if ((nobetdisplay==0) || (manualActive==1)) {
          color = balance*100/maxbalance;
          if (color<86) color=86;
          wattron (betregion, COLOR_PAIR (color2));
          if (balance<maxbalance) {
            wprintw (betregion, "%4i %4i ",abs(currentstreak),betcount-lastmax);
            wattron (betregion, COLOR_PAIR (color));
            wprintw (betregion, "%18.8f  ",(double) balance / 1e8);
            wattron (betregion, COLOR_PAIR (color2));
            wprintw(betregion, "%5.2f%%  %20.8f  %20.8f   \n",chance,betsize,(double) bprofit / 1e8);
          } else
            wprintw (betregion, "%4i %4i %18.8f  %5.2f%%  %20.8f  %20.8f  +%.8f\n",
                    abs(currentstreak),betcount-lastmax,
                   (double) balance / 1e8, chance, betsize,
                   (double) bprofit / 1e8,
                   (double) gain/1e8);
          wattroff (betregion, COLOR_PAIR (color));
          wattroff (betregion, A_BOLD);
          wrefresh (betregion);
        }
        updateStatus();
        logfile = fopen (bnowfile, "a+");
        if (infoval>0.0) 
          fprintf (logfile, "%i:%.8f %.8f\n", streamprefix, (double) balance / 1e8, infoval);
        else
          fprintf (logfile, "%i:%.8f\n", streamprefix, (double) balance / 1e8);
        
        fclose (logfile);
//        printf("Balance: %.8f  %.2f%%  bet: %.8f  profit: %.8f  won %i of %i (%.2f%%) lastmax:%i diff: %.8f\n",(double) balance/1e8,chance,betsize,profit/1e8,wincount,betcount,(double) wincount/betcount*100,betcount-lastmax,(double) (balance-maxbalance)/1e8);

      }
      
      else {                    // something went wrong
        endwin();
        for (int n=0;n<height;n++) printf("\n");
        printf("\033[1m\033[31mERROR?!\n\033[33mRequest sent: \033[37m%s\n\033[33mServer answered: \033[36m%s\033[0m\n",
           request, chunk.memory);
        kbd = 'D';              // quit
      }
    }
    printf ("\033[0m");
    return 1;
  }
  return 1;
}



int autobets() {
  int c;
  char *token;
  char option[50];
  int secret;
  CURLcode res;
  struct MemoryStruct chunk;
  chunk.memory = malloc (1);    /* will be grown as needed by the realloc above */
  chunk.size = 0;               /* no data at this point */
  if (BasePayIn > MaxPayIn)
    MaxPayIn = BasePayIn;
  attron(COLOR_PAIR(5));
  mvprintw (9, 18,"balance     chance    #wins            BasePayIn             profit");
  attroff(COLOR_PAIR(5));
  
  if (curl) {
    if (((balance-BasePayIn*MaxBets)>(maxbalance-stopamount)) && ((balance-BasePayIn)>StopMinBalance)) {
      c = (int) (chance * 10000 - 1);
      Low = abs ((int) mrand48 ()) % (999999 - c);
      clientseed = mrand48 ();
      High = Low + c;
      sprintf (request,"a=PlaceAutomatedBets&s=%s",sessioncookie);
      sprintf (option,"&BasePayIn=%"PRIi64,BasePayIn);
      strcat(request,option);
      sprintf (option,"&Low=%i&High=%i",Low, High);
      strcat(request,option);
      sprintf (option,"&MaxBets=%i",MaxBets);
      strcat(request,option);
      if ((StopMinBalance>0) && (StopMinBalance>maxbalance-stopamount))
        sprintf (option,"&StopMinBalance=%"PRIi64,StopMinBalance);
      else sprintf (option,"&StopMinBalance=%"PRIi64,maxbalance-stopamount);
      strcat(request,option);
      if (StopMaxBalance>0) {
        sprintf (option,"&StopMaxBalance=%"PRIi64,StopMaxBalance);
        strcat(request,option);
      }

      if (ResetOnWin) strcat(request,"&ResetOnWin=true");
      if (ResetOnLose) strcat(request,"&ResetOnLose=true");

      if (IncreaseOnWinPercent>0) {
        sprintf(option,"&IncreaseOnWinPercent=%.6f",IncreaseOnWinPercent);
        strcat(request,option);
      }
      if (IncreaseOnLosePercent>0) {
        sprintf(option,"&IncreaseOnLosePercent=%.6f",IncreaseOnLosePercent);
        strcat(request,option);
      }

      if (AutoMaxPayIn>0) {
        sprintf(option,"&MaxPayIn=%"PRIi64,AutoMaxPayIn);
        strcat(request,option);
      }

      if (ResetOnLoseMaxBet) strcat(request,"&ResetOnLoseMaxBet=true");
      if (StopOnLoseMaxBet) strcat(request,"&StopOnLoseMaxBet=true");
     
      if (StartingPayIn>0) {
        sprintf(option,"&StartingPayIn=%"PRIi64,StartingPayIn);
        strcat(request,option);
      }
      sprintf (option,"&ClientSeed=%"PRIi32,clientseed);
      strcat(request,option);
     
      sprintf (option,"&Currency=%s",currency);
      strcat(request,option);
     
      strcat(request,"&Compact=true&ProtocolVersion=2");

      curl_easy_setopt (curl, CURLOPT_URL, url);
      curl_easy_setopt (curl, CURLOPT_POSTFIELDS, request);
      curl_easy_setopt (curl, CURLOPT_USERAGENT, version);
      curl_easy_setopt (curl, CURLOPT_TIMEOUT, 60L);
      curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
      curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *) &chunk);

      res = curl_easy_perform (curl);
      if (res != CURLE_OK) {
        endwin();
        for (int n=0;n<height;n++) printf("\n");
        fprintf(stderr,"autobets(): curl_easy_perform() error %i: %s\e[K\n", res, curl_easy_strerror (res));
        exit(9);
      } else {  // Got a server reply - chunk.memory has result
  /*
  {"BetId":129889029664,"BetCount":24,"Seed":"9950CEDA4D53E15DC6B292D8C7EBD9D0D7C5EAA21D9C635EEB8A2E2147E42708","PayIn":-2410400,"PayOut":2609800,"StartingBalance":571793585947,"Next":"5332805df62fbbda96eb7b45294e1547e19532cc086a3608c5c9b6d9d08b3eb2"}
  */
        for (int i=25;i<53;i++) request[i]='x';
        betlog(request);
        betlog(chunk.memory);

        if (strstr (chunk.memory, "BetId")) {     // → bet successful
          token = strtok (chunk.memory, ":");     // BetId
          token = strtok (NULL, ",");
          BetId = strtoll (token, NULL, 10);

          token = strtok (NULL, ":");     // BetCount
          token = strtok (NULL, ",");
          BetCount = strtoll (token, NULL, 10);
          betcount+=BetCount;

          token = strtok (NULL, ":");     // ServerSeed
          token = strtok (NULL, ",");
          strncpy(serverseed,token+1,64);
          if (strlen(nextserverseed)>0) verifySeed(serverseed,nextserverseed);

          token = strtok (NULL, ":");     // PayIn
          token = strtok (NULL, ",");
          PayIn = -strtoll (token, NULL, 10);
          wagered+=PayIn;
          
          token = strtok (NULL, ":");     // PayOut
          token = strtok (NULL, ",");
          PayOut = strtoll (token, NULL, 10);

          token = strtok (NULL, ":");     // StartingBalance
          token = strtok (NULL, ",");
          StartingBalance = strtoll (token, NULL, 10);

          token = strtok (NULL, ":");     // Next
          token = strtok (NULL, ",");
          strncpy(nextserverseed,token+1,64);

          // Calculate the number of wins in this request
          WinCount=0;
          for (int w=0;w<BetCount;w++) {
            secret=BetResult(serverseed,clientseed,w);
            if ((secret>=Low) && (secret<=High)) {
              WinCount++;
              if (currentstreak<0) {
                laststreak=currentstreak;
                streaklose[abs(currentstreak)]++;
                currentstreak=0;
              }
              currentstreak++;
            } else {
              if (currentstreak>0) {
                laststreak=currentstreak;
                streakwin[currentstreak]++;
                currentstreak=0;
              }
              currentstreak--;
            }
            if (currentstreak < minstreak) minstreak = currentstreak;
            if (currentstreak > maxstreak) maxstreak = currentstreak;
          }

          wincount+=WinCount;

          balance = StartingBalance - PayIn + PayOut;
          bprofit = PayOut - PayIn;
          currentprofit = PayOut - PayIn;
          if (PayOut>PayIn) { // win
            color = 1;
            color2 = 1;
            win = 1;
          } else {
            color2 = 3;
            win = 0;
          }
          if (balance >= maxbalance) {
            gain = balance - maxbalance;
            maxbalance = balance;
            maxbets = betcount - lastmax;
            maxincrcount++;
            wattron (betregion, A_BOLD);
            TmpMaxPayIn = 0;
            lastmax = betcount;
            if ((exitOnMax)
                || ((turnlimit > 0) && (betcount > turnlimit))) {
              strcpy (exitReason, "EXIT on MAX requested");
              kbd = 'Q';
            }

          }
          if (balance < minbalance)
            minbalance = balance;
          if ((maxbalance - balance) > maxdiff)
            maxdiff = maxbalance - balance;
          if ((nobetdisplay == 0) || (manualActive==1)) {
            color = balance*100/maxbalance;
            if (color<86) color=86;
            wattron (betregion, COLOR_PAIR (color2));
            if (balance<maxbalance) {
              wprintw (betregion, "   %6i ",betcount-lastmax);
              wattron (betregion, COLOR_PAIR (color));
              wprintw (betregion, "%18.8f  ",(double) balance / 1e8);
              wattron (betregion, COLOR_PAIR (color2));
              wprintw(betregion, "%5.2f%%  %3i/%3i %20.8f  %20.8f   \n",chance,WinCount,BetCount,(double) BasePayIn/1e8,(double) bprofit / 1e8);
            } else
              wprintw (betregion, "   %6i %18.8f  %5.2f%%  %3i/%3i %20.8f  %20.8f  +%.8f\n",
                      betcount-lastmax,
                     (double) balance / 1e8, chance, WinCount, BetCount,(double)  BasePayIn/1e8,
                     (double) bprofit / 1e8,
                     (double) gain/1e8);
            wattroff (betregion, COLOR_PAIR (color));
            wattroff (betregion, A_BOLD);
            wrefresh (betregion);
          }
          updateStatus();
          logfile = fopen (bnowfile, "a+");
          if (infoval>0) fprintf (logfile, "%i:%.8f %.8f\n", streamprefix, (double) balance / 1e8,infoval);
          else fprintf (logfile, "%i:%.8f\n", streamprefix, (double) balance / 1e8);
          fclose (logfile);
        } else {
          // we didn't get a bet Id, so we got an error
           endwin ();
           fprintf (stderr, "\nFATAL ERROR:\n  %s\n\n", chunk.memory);
           exit (9);
        }
      }
    } else {
      if (sessionprofit>0) {
        kbd='Q';
        return 1;
      }
      wattron(betregion,COLOR_PAIR(5));
      wprintw (betregion,"\nNext BasePayIn: %.8f - now at %i%% of maxbalance. Press ", (double) BasePayIn/1e8, balance * 100 / maxbalance);
      wattroff(betregion,COLOR_PAIR(5));
      wattron(betregion,COLOR_PAIR(2));
      attron (A_BOLD);
      wprintw (betregion,"+");
      attroff (A_BOLD);
      wattroff(betregion,COLOR_PAIR(2));
      wattron(betregion,COLOR_PAIR(5));
      wprintw (betregion," to increase risk balance / ");
      wattroff(betregion,COLOR_PAIR(5));
      wattron(betregion,COLOR_PAIR(2));
      attron (A_BOLD);
      wprintw (betregion,"q");
      attroff (A_BOLD);
      wattroff(betregion,COLOR_PAIR(2));
      wattron(betregion,COLOR_PAIR(5));
      wprintw (betregion,"uit / ");
      wattroff(betregion,COLOR_PAIR(5));
      wattron(betregion,COLOR_PAIR(2));
      attron (A_BOLD);
      wprintw (betregion,"m");
      attroff (A_BOLD);
      wattroff(betregion,COLOR_PAIR(2));
      wattron(betregion,COLOR_PAIR(5));
      wprintw (betregion,"anual\n");
      wattroff(betregion,COLOR_PAIR(5));
      exitOnMax = 1;
      wrefresh (betregion);

      InteractiveMode();
      return 0;
    } 
  } // if (curl)
  return 1;
}

// function for msg and print
int msg(lua_State *L)  {
  wattron (msgregion, COLOR_PAIR(5));
  wprintw (msgregion, "%12i: ", betcount);
  wattroff (msgregion, COLOR_PAIR(5));
  wattron (msgregion, COLOR_PAIR (2));
  wprintw (msgregion, " %s\n", lua_tostring(L,1));
  wattroff (msgregion, COLOR_PAIR (2));
  wrefresh (msgregion);
  return 0;
}

// Output the status in upper part of the terminal
void updateStatus() {
  double profitPerSecond;
  double betsPerSecond;
  attron (COLOR_PAIR (2));
  mvprintw (0, 0, " minimum: %18.8f", (double) minbalance / 1e8);
//  if (exitOnMax)  mvprintw (0, 60, "EXIT on MAX %i",exitOnMaxLine);
  if (stopamount==maxbalance) {
     mvprintw (1, 0, " current: %18.8f              ", (double) balance / 1e8);
  } else {
    if (strcmp(currency,"doge")) 
      mvprintw (1, 0, " current: %18.8f (%.8f)       ", (double) balance / 1e8, (double) (maxbalance-stopamount)/100000000);
    else
      mvprintw (1, 0, " current: %18.8f (%i)       ", (double) balance / 1e8, (maxbalance-stopamount)/100000000);
  }   
  mvprintw (2, 0, "    diff: %18.8f (max. %.8f)   ",
            (double) (maxbalance - balance) / 1e8,
            (double) maxdiff / 1e8);
  mvprintw (3, 0, " maximum: %18.8f", (double) maxbalance / 1e8);
  if (goal>0) mvprintw (3,29,"(goal: %.8f) ",goal);
  if (wdsum>0) {
    if (strcmp(currency, "doge")) {
      mvprintw (4, 0, "  profit: %18.8f (%.3f%%) [wd: %.8f] ",
                (double) (balance - bstart) / 1e8,
                (double) balance*100/bstart-100,
                (double) wdsum/1e8);
    } else {
      mvprintw (4, 0, "  profit: %18.8f (%.3f%%) [wd: %.2f] ",
                (double) (balance - bstart) / 1e8,
                (double) balance*100/bstart-100,
                (double) wdsum/1e8);
    }
  } else {
    mvprintw (4, 0, "  profit: %18.8f (%.3f%%) ",
              (double) (balance - bstart) / 1e8,
              (double) balance*100/bstart-100);
  }  
  if (MaxBets>0)  mvprintw (5, 0, "MaxBasePayIn: %14.8f", (double) MaxPayIn / 1e8);
  else  mvprintw (5, 0, "MaxPayIn: %18.8f", (double) MaxPayIn / 1e8);

  mvprintw (6, 0, " wagered: %18.8f (ref. %.8f)   ",
            (double) wagered / 1e8,
            (double) wagered / 1e8 / 2000);
  attroff (COLOR_PAIR(2));
  attron (COLOR_PAIR(5));
  gettimeofday(&end,(struct timezone *)0);
  seconds = end.tv_sec - begin.tv_sec;
  if (seconds>0) {
    betsPerSecond=(double) betcount/seconds;
    profitPerSecond=(balance-bstart+wdsum)/seconds;
  } else {
    betsPerSecond=0;
    profitPerSecond=0;
  }
  int minutes = seconds/60;
  int hours = minutes/60;
  minutes=minutes%60;
  seconds=seconds%60;
  mvprintw (1, 58, "won %i of %i bets (%.2f%%) %.2f bets/s ", wincount, betcount, (double) wincount / betcount * 100, betsPerSecond);
  if ((betcount - lastmax) > lastmaxmax)
    lastmaxmax = betcount - lastmax;
  mvprintw (2, 58, "last max: %4i (%i) (%.0f avg)  %.2f%%  ",betcount - lastmax, 
            lastmaxmax, (double) betcount/maxincrcount, (double) balance * 100 / maxbalance);
  mvprintw (3, 58, "streak: %4i (min %i  max %i)       ",
            currentstreak, minstreak, maxstreak);
  if ((seconds!=oldseconds)) {
    mvprintw (4, 58, "%i:%02i:%02i  %.2f/min %.2f/hour    ",hours,minutes,seconds, profitPerSecond*60/1e8, profitPerSecond*3600/1e8);
    oldseconds=seconds;
  }
  mvprintw (6, 58, "info: %8f        ", infoval);
  // losing streaks
  int d=0;
  for (int n=0;n<7;n++) {
    if (minstreak<-7) while ((streaklose[abs(minstreak+n+d)]==0) && (d<abs(minstreak))) d++;
    if ((minstreak+n+d)<0) {
      attron(COLOR_PAIR(3));
      mvprintw(7, 1+n*7,"%5i ",minstreak+n+d);
      attroff(COLOR_PAIR(3));
      attron(COLOR_PAIR(5));
      mvprintw(8, 1+n*7,"%5i ",streaklose[abs(minstreak+n+d)]);
    }
  }
  // winning streaks
  d=0;
  for (int n=0;n<7;n++) {
    while ((streakwin[maxstreak-n-d]==0) && (d<maxstreak)) d++;
    if (maxstreak-n-d>0) {
      attron(COLOR_PAIR(1));
      mvprintw(7, width-16-n*7,"%5i ",maxstreak-n-d);
      attroff(COLOR_PAIR(1));
      attron(COLOR_PAIR(5));
      mvprintw(8, width-16-n*7,"%5i ",streakwin[maxstreak-n-d]);
    }
  }

  if (profitensure == 1) mvprintw (1, 43, "%.8f", (double) (bstart + maxbalance) / 200000000);
  attroff(COLOR_PAIR(5));
  refresh ();
}

void login(){
  char username[50];
  char password[100];
  char totp[10];
  char *token;
  char currency[5]="btc";
  char *ptr;
  double balance;
  char filename[10];
  FILE *fd;
  CURLcode res;
  struct MemoryStruct chunk;
  sprintf(url,"%s/api/web.aspx",servername);
  echo();
  curs_set(1);
  wattron(betregion,COLOR_PAIR(5));
  wprintw(betregion,"\nWelcome!\nPlease login to 999dice.com\n\n");
  wprintw(betregion,"Note: No login data is saved to your disk. What IS SAVED to your disk\n");
  wprintw(betregion,"is a SessionCookie to file account.data in your current working directory,\n");
  wprintw(betregion,"If you want the bot to logout just delete the file account.data\n\n");
  wprintw(betregion,"\nUsername: ");
  wattroff(betregion,COLOR_PAIR(5));
  wrefresh(betregion);
  wgetnstr(betregion,username,49);
  wattron(betregion,COLOR_PAIR(5));
  wprintw(betregion,"Password: ");
  wattroff(betregion,COLOR_PAIR(5));
  wrefresh(betregion);
  noecho();
  wgetnstr (betregion,password,49);
  wattron(betregion,COLOR_PAIR(5));
  wprintw(betregion,"TOTP (2FA) [Leave empty if you don't use 2FA]: ");
  wattroff(betregion,COLOR_PAIR(5));
  wrefresh(betregion);
  echo();
  wgetnstr(betregion,totp,9);
  curs_set(0);
  chunk.memory = malloc (1); 
  chunk.size = 0; 
  if (curl) {
    if (strlen(totp)==0)
      sprintf (request,"a=Login&Key=f2268504b4a04d5e9e75f6cc199a4763&Username=%s&Password=%s",username,password);
    else
      sprintf (request,"a=Login&Key=f2268504b4a04d5e9e75f6cc199a4763&Username=%s&Password=%s&Totp=%s",username,password,totp);
    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_POSTFIELDS, request);
    curl_easy_setopt (curl, CURLOPT_USERAGENT, version);
    curl_easy_setopt (curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *) &chunk);
    res = curl_easy_perform (curl);
    if (res != CURLE_OK) {
      endwin();
      for (int n=0;n<height;n++) printf("\n");
      fprintf(stderr,"login(): curl_easy_perform() error %i: %s\e[K\n", res, curl_easy_strerror (res));
      exit(9);
    } else {
      // chunk.memory has result
      // and it looks like 
      // {"SessionCookie":"12345678901234567890123456789012","MaxBetBatchSize":200,"ClientSeed":0,"ReferredById":null,"BetCount":117690,"BetPayIn":-3075313,"BetPayOut":2994301,"BetWinCount":41129,"AccountId":1485915,"Balance":0,"Email":null,"EmergencyAddress":null,"DepositAddress":"1LLFFt47uviu14S9VAd1KSMej9E45iqtzM","Doge":{"BetCount":359274,"BetPayIn":-434555444618186,"BetPayOut":420227517351790,"BetWinCount":25353,"Balance":33102001998,"DepositAddress":"DS9auodJVYpXmvuB4ipTB7J9x4zpoToqE8"},"LTC":{"BetCount":20748,"BetPayIn":-12053652057,"BetPayOut":11664803985,"BetWinCount":3936,"Balance":0,"DepositAddress":null},"ETH":{"BetCount":20,"BetPayIn":0,"BetPayOut":0,"BetWinCount":3,"Balance":0,"DepositAddress":null}}
      if (strstr (chunk.memory, "SessionCookie")) { 
        token = strtok (chunk.memory, "\"");
        while (token != NULL) {
          if (strstr(token,"SessionCookie")) {
            token = strtok (NULL, "\"");
            token = strtok (NULL, "\"");
            if ((fd=fopen("account.data","w+")) != NULL) {
              fprintf(fd,"SessionCookie=\"%s\"   -- NEVER SHARE THIS IN PUBLIC\n",token);
              fclose(fd);
            }
          }
          if (strstr(token,"DepositAddress")) {
            token = strtok (NULL, "\"");
            token = strtok (NULL, "\"");
            if ((fd=fopen("account.data","a+")) != NULL) {
              fprintf(fd,"%sAddress=\"%s\"\n",currency,token);
              fclose(fd);
            }
          }
          if (strstr(token,"MaxBetBatchSize")) {
            token = strtok (NULL, ",");
            if ((fd=fopen("account.data","a+")) != NULL) {
              fprintf(fd,"MaxBetBatchSize=%s\n",token+1);
              fclose(fd);
            }
          }
          if (strstr(token,"AccountId")) {
            token = strtok (NULL, ",");
            if ((fd=fopen("account.data","a+")) != NULL) {
              fprintf(fd,"AccountId=%s\n",token+1);
              fclose(fd);
            }
          }
          if (strstr(token,"Balance")) {
            token = strtok (NULL, ",");
            balance=strtod(token+1,&ptr);
            sprintf(filename,"bnow.%s",currency);
            if ((fd=fopen(filename,"a+")) != NULL) {
              fprintf(fd,"%i:%.8f\n",streamprefix,balance/1e8);
              fclose(fd);
            }
          }
          if (strstr(token,"Doge")) strcpy(currency,"doge");
          if (strstr(token,"LTC")) strcpy(currency,"ltc");
          if (strstr(token,"ETH")) strcpy(currency,"eth");
          token = strtok (NULL, "\"");
        }
        token = strtok (chunk.memory, ":");
        token = strtok (NULL, "\"");
        endwin();
        if (strlen(totp)!=0) {
          if ((fd=fopen("account.data","a+")) != NULL) {
            fprintf(fd,"TOTP=yes\n");
            fclose(fd);
          }
        }
        if ((fd=fopen("account.data","a+")) != NULL) {
          fprintf(fd,"Server=\"%s\"\n",servername);
          fclose(fd);
        }
        printf ("\nLogin successful. If you ever get \"Invalid Session\" from server, delete the file »account.data« to relogin.\n" );
        exit(0);
      } else {
         endwin();
         fprintf(stderr, "Login didn't work. Server returned:\n%s\n",chunk.memory);
         exit(1);
      }
    }
  }
}

int securitycheck() {
  FILE *fp;
  int line_num = 1;
  char line[512];

  if((fp = fopen(botname, "r")) == NULL) return(-1);
  while(fgets(line, 512, fp) != NULL) {
    if((strstr(line, "os.execute")) != NULL) {
                  printf("WARNING! This lua-script contain an os.execute() call on line: %d\n", line_num);
                  printf("\n%s\n", line);
                  printf("\nThis is a potential security risk. Please examine the line closely\n");
                  printf("Press CTRL-c to exit or wait 15 seconds\n");
                  sleep(15);
          }
          line_num++;
  }
  fclose(fp);
  return(0);
}


int main (int argc, char *argv[]) {
  gettimeofday(&begin,(struct timezone *)0);
  int opt;
  srand48 (time (NULL) + begin.tv_usec);
  getcwd (cwd, sizeof (cwd));
  char *base = strrchr(cwd, '/');
  strncpy(dirname,base+1,25);
  printf ("\ek%s\e\\", cwd);
  // install USR1 handler → exit on next max balance
  signal (10, signalHandlerUSR1);
  for (int n=0;n<511;n++) {
    streakwin[n]=0;
    streaklose[n]=0;
  }
  // cmdline parsing
  if (argc < 2) {
    printf("\nluabot is a fast 999dice bot capable to handle DiceBot style lua files\n\n");
    printf("Copyright (C) 2020 Elele <elele@secmail.pro>\n");
    printf("This program comes with ABSOLUTELY NO WARRANTY\n");
    printf("This is free software, and you are welcome to redistribute it\n");
    printf("under conditions of GPL3 or later. See file LICENSE for details.\n");
    printf("\n");
    printf("  call it as: luabot [options] <name_of_your_lua_file>\n");
    printf("\noptions:\n");
    printf(" -c <currency>  # set currency, to btc, doge (default), ltc or eth\n");
    printf(" -H <amount>    # enter interactive mode after losing <amount>\n");
    printf(" -h <percent>   # enter interactive mode after losing <percent>%%\n");
    printf(" -O \"command\"   # override OnStopCommand from account.data\n");
    printf(" -P <amount>    # set an amount of proft that should enable profit ensurance\n");
    printf(" -n             # disable stop on losing half profit after winning 2000*basebet\n");
    printf(" -g <amount>    # stop when this balance has been reached\n");
    printf(" -G <amount>    # stop when this profit has been reached\n");
    printf(" -t <turns>     # after running for <turns> bets, exit on reaching max. balance\n");
    printf(" -k             # keep betlog file (no matter how large it grows)\n");
    printf(" -S             # exit instead of interactive mode when run into stoppoint\n");
    printf(" -m             # start a manual session\n");
    printf(" -X             # exit when detecting a server seed hash mismatch\n");
    printf(" -r             # set betcount (useful for stats when resuming a session\n");
    printf(" -w             # set wincount (useful for stats when resuming a session\n");
    printf(" -p             # set a streamprefix for bnow file (useful with driveGnuPlots.pl)\n");
    printf(" -U <URL>       # set an alternative URL, defaults to https://www.999dice.com\n"); 
    printf("\n\nSetting a bottom stop with -h or -H is mandatory, if you don't want luabot\n");
    printf("to handle stop points (or going interactive) at all use luabot -h 100 -n \n");
    printf("\nGood Luck!\n\n");
    return 1;
  }

  while ((opt = getopt(argc, argv, ":s:c:h:H:SkmnNr:w:t:g:G:Xp:O:P:A:U:")) != -1) {
    switch (opt) {
      case 's':	strncpy(sessioncookie,optarg,32);
        break;
      case 'c': strncpy(currency,optarg,4);
        break;
      case 'H': stopamount=(long long) (atof(optarg)*1e8);
        break;
      case 'h': percentualStop=atof(optarg);
        break;
      case 'k': keeplogfile=1;
        break;
      case 'S': hardstop=1; alarmOn=0;
        break;
      case 'm': manualSession=1;
        break;
      case 'n': noprofitensure=1;
        break;
      case 'N': nobetdisplay++;;
        break;
      case 'r': betcount=atoi(optarg);
        break;
      case 'w': wincount=atoi(optarg);
        break;
      case 't': turnlimit=atoi(optarg);
        break;
      case 'g': goal=atof(optarg);
        break;
      case 'G': goalDelta=atof(optarg);
        break;
      case 'X': exitOnMismatch=1;
        break;
      case 'p': streamprefix=atoi(optarg);
        break;
      case 'O': overrideOnStopCmd=1;
        strncpy(OnStopCommand,optarg,255);
        OnStopCmd=1;
        break;
      case 'A': overrideAutowd=1;
        strncpy(autowd,optarg,199);
        break;
      case 'P': profitensurance=(long long) (atof(optarg)*1e8);
        break;
      case 'U': overrideServer=1;
        strncpy(servername,optarg,128);
        break;
      case '?': printf("unknown option: %c\n",optopt);
        break;
     } 
  }
  // last parameter is bot name
  for(; optind < argc; optind++){
    strncpy(botname, argv[optind],39);
  }

  // check for os.execute()
  securitycheck();

  // INIT CURL
  curl_global_init (CURL_GLOBAL_ALL);
  curl = curl_easy_init ();

  /* init ncurses */
  initscr ();
  curs_set (0);
  cbreak ();
  noecho ();
  nodelay (stdscr, TRUE);
  start_color ();
  getmaxyx (stdscr, height, width);
  heightavail=height-10;
  if ((heightavail%2==0)) {
    betregion = newwin (heightavail/2+1,width, 10, 0);
    msgregion = newwin (heightavail/2-2,width, heightavail/2+11,0);
  } else {
    betregion = newwin (heightavail/2+1,width, 10, 0);
    msgregion = newwin (heightavail/2-2,width, heightavail/2+11,0);
  }
  manualline = heightavail/2+10;
  scrollok (betregion, TRUE);
  scrollok (msgregion, TRUE);
  use_default_colors();
  init_pair (1, COLOR_GREEN, -1);
  init_pair (2, COLOR_YELLOW, -1);
  init_pair (3, COLOR_RED, -1);
  init_pair (4, COLOR_CYAN, -1);
  init_pair (5, COLOR_WHITE, -1);
  init_pair (100, COLOR_GREEN, -1); // the following pairs are for color gradient
  init_pair (99, 48, -1); // on 256 color terminal. TODO: check if colors availble
  init_pair (98, 82, -1);
  init_pair (97,118, -1);
  init_pair (96,154, -1);
  init_pair (95,190, -1);
  init_pair (94,226, -1);
  init_pair (93,220, -1);
  init_pair (92,214, -1);
  init_pair (91,208, -1);
  init_pair (90,202, -1);
  init_pair (89,196, -1);
  init_pair (88,160, -1);
  init_pair (87,124, -1);
  init_pair (86, 88, -1);

  if ((nobetdisplay == 0) || (manualActive==1)) {
    attron(COLOR_PAIR(5));
    mvprintw (9, 18, "balance     chance             betsize               profit");
    attroff(COLOR_PAIR(5));
  }
  attron(COLOR_PAIR(5));
  mvprintw (0, 58, "%s: %s",dirname,botname);
  attroff(COLOR_PAIR(5));

  // read account.data or call login
  L = luaL_newstate ();
  luaL_openlibs (L);
  if (luaL_loadfile (L, "account.data")) {
    // we couldn't open account.data so we need to make a new one
    login();
    exit(0);
  }
  if (lua_pcall (L, 0, 0, 0))
    bailout (L, "lua_pcall() failed",1);
  lua_getglobal (L, "SessionCookie");
  strncpy(sessioncookie,lua_tostring(L,-1),32);
  if (overrideOnStopCmd==0) {
    lua_getglobal (L, "OnStopCommand");
    if (lua_tostring(L,-1) != NULL) {
      strncpy(OnStopCommand,lua_tostring(L,-1),255);
      OnStopCmd=1;
    }
  }
  if (overrideAutowd==0) {
    lua_getglobal (L, "WithdrawAddress");
    if (lua_tostring(L,-1) != NULL) {
      strncpy(autowd,lua_tostring(L,-1),199);
    }
  }

  if (overrideServer==0) {
    lua_getglobal (L, "Server");
    if (lua_tostring(L,-1) != NULL) {
      strncpy(servername,lua_tostring(L,-1),128);
    }
  }
  lua_close(L);
  
  sprintf(url,"%s/api/web.aspx",servername);

  /* get balance from file or api before init botscript */
  getBalance();
  maxbalance=balance;
  minbalance=balance;
  bstart=balance;
  if (goalDelta>0) goal=((double) balance / 1e8)+goalDelta;
  if (percentualStop>=100) percentualStop=100;
  if (percentualStop>0) stopamount=balance*percentualStop/100;
  if (manualSession==1) manualMode();
  else {
    /* init Lua */
    L = luaL_newstate ();
    luaL_openlibs (L);
    if (luaL_loadfile (L, botname))
      bailout (L, "luaL_loadfile() failed",1);
    luaopen=1;
    lua_pushnumber (L, (double) (balance / 1e8));
    lua_setglobal (L, "balance");
    lua_pushnumber (L, MaxBets);
    lua_setglobal (L, "MaxBets");
    lua_pushstring (L, currency);
    lua_setglobal (L, "currency");
    if (lua_pcall (L, 0, 0, 0))
      bailout (L, "lua_pcall() failed",1);
    lua_getglobal (L, "MaxBets");
    MaxBets = lua_tonumber(L, -1);
    if (MaxBets==0) { 
      // for single bets
      lua_getglobal (L, "chance");
      chance = lua_tonumber (L, -1);
      lua_getglobal (L, "nextbet");
      nextbet = (long long) (lua_tonumber (L, -1)*1e8) / 1e8;
      basebet = nextbet;
      lua_pushcfunction (L, msg);
      lua_setglobal(L, "print");
      lua_pushcfunction (L, Withdraw);
      lua_setglobal(L, "withdraw");
      lua_pushcfunction (L, stoplua);
      lua_setglobal (L, "stop");
      lua_pushnumber (L, bnow);
      lua_setglobal (L, "balance");
    } else {  // for PlaceAutomatedBets
      lua_pushcfunction (L, msg);
      lua_setglobal(L, "print");
      lua_pushcfunction (L, stoplua);
      lua_setglobal (L, "stop");
      lua_pushcfunction (L, Withdraw);
      lua_setglobal(L, "withdraw");
      lua_getglobal (L, "BasePayIn");
      BasePayIn=(int64_t) (lua_tonumber(L,-1)*1e8);
      lua_getglobal (L, "chance");
      chance = lua_tonumber (L, -1);
      lua_getglobal (L, "ResetOnWin");
      ResetOnWin= lua_toboolean (L, -1);
      lua_getglobal (L, "ResetOnLose");
      ResetOnLose= lua_toboolean (L, -1);
      lua_getglobal (L, "IncreaseOnWinPercent");
      IncreaseOnWinPercent=lua_tonumber(L, -1);
      lua_getglobal (L, "IncreaseOnLosePercent");
      IncreaseOnLosePercent=lua_tonumber(L, -1);
      lua_getglobal (L, "MaxPayIn");
      AutoMaxPayIn=(int64_t) (lua_tonumber(L, -1)*100000000);
      lua_getglobal (L, "ResetOnLoseMaxBet");
      ResetOnLoseMaxBet=lua_toboolean(L,-1);
      lua_getglobal (L, "StopOnLoseMaxBet");
      StopOnLoseMaxBet=lua_toboolean(L,-1);
      lua_getglobal (L, "StopMaxBalance");
      StopMaxBalance=(int64_t) (lua_tonumber(L,-1)*100000000);
      lua_getglobal (L, "StopMinBalance");
      StopMinBalance=(int64_t) (lua_tonumber(L,-1)*100000000);
      lua_getglobal (L, "StartingPayIn");
      StartingPayIn=(int64_t) (lua_tonumber(L,-1)*100000000);
      lua_getglobal (L, "info");
      infoval=lua_tonumber(L,-1);
    }
          

    // USER INTERFACE        
    while ((kbd != 'Q') && (kbd != 'D')) {
      timeout (0);
      kbd = getch ();
      switch (kbd) {
      case 'h':
        stopamount = stopamount*110/100;
        stopbetting = 0;
        break;
      case 'c':
        stopbetting = 0;
        break;
      case 'm':
        manualMode();
        break;
      case 'p':
        stopbetting = 1;
        break;
      case 's':
        profitensure = 1;
        attron (A_BOLD);
        attron (COLOR_PAIR (3));
        mvprintw (1, 29, "STOP5 @");
        attroff (COLOR_PAIR (3));
        attroff (A_BOLD);
        break;
      case 'x':
        attron (A_BOLD);
        attron (COLOR_PAIR (4));
        mvprintw (0, 32, "EXIT on MAX balance");
        attroff (COLOR_PAIR (4));
        attroff (A_BOLD);
        exitOnMax = 1;
        break;
      }
      if (((double) balance / 1e8 > goal) && (goal > 0)) {
        exitOnMax = 1;
      }
      if (stopbetting == 0) { // betting loop
      // is this single or autobet?
        if (MaxBets<2) Bet(nextbet);
        else {
          int rv=autobets();
          if (rv==2) {
            nextbet=(double) BasePayIn/1e8;
            MaxBets=0;
            exitOnMax=1;
            manualMode();
          }
        }
        sessionprofit = balance - bstart;
        // https://steemit.com/dicebot/@seuntjie/dicebot-programmer-mode-tutorial-1-1-variables
        if (MaxBets>0) {
          lua_pushnumber (L, WinCount);  // with autobets set WinCount to the number of wins of last betrow
          lua_setglobal(L, "WinCount");
          lua_pushnumber (L, BetCount);  // set BetCount to the number of wins of last betrow
          lua_setglobal(L, "BetCount");
        }

        lua_pushnumber (L, (double) balance / 100000000);
        lua_setglobal (L, "balance");

        lua_pushnumber (L, secret);
        lua_setglobal (L, "secret");

        lua_pushboolean (L, win);
        lua_setglobal (L, "win");

        lua_pushnumber (L, sessionprofit/1e8);
        lua_setglobal (L, "profit");

        lua_pushnumber (L, currentprofit/1e8);
        lua_setglobal (L, "currentprofit");

        lua_pushnumber (L, currentstreak);
        lua_setglobal (L, "currentstreak");

        lua_pushnumber (L, nextbet);
        lua_setglobal (L, "previousbet");

        lua_pushnumber (L, betcount);
        lua_setglobal (L, "bets");

        lua_pushnumber (L, wincount);
        lua_setglobal (L, "wins");

        lua_pushnumber (L, losecount);
        lua_setglobal (L, "losses");

        // chance: to be set by lua script
        // bethigh: not supported
        // lastBet: not supported
        // currencies: not supported 
        // currency: not supported
        // enablesrc: not supported 
        // enablezz: not supported

        lua_getglobal (L, "dobet");
        if (lua_pcall (L, 0, 0, 0))
          bailout (L, "lua_pcall() failed",1);      // run dobet()
        lua_getglobal (L, "MaxBets");
        MaxBets = lua_tonumber(L, -1);
        if (MaxBets==0) { 
          // for single bets
          lua_getglobal (L, "chance");
          chance = lua_tonumber (L, -1);
          lua_getglobal (L, "nextbet");
          nextbet = (long long) (lua_tonumber (L, -1)*1e8) / 1e8;
          basebet = nextbet;
          lua_getglobal (L, "info");
          infoval = (double) lua_tonumber (L, -1);
          lua_pushcfunction (L, msg);
          lua_setglobal(L, "print");
          lua_pushcfunction (L, stoplua);
          lua_setglobal (L, "stop");
          lua_pushnumber (L, bnow);
          lua_setglobal (L, "balance");
        } else {  // for PlaceAutomatedBets
          lua_getglobal (L, "BasePayIn");
          BasePayIn=(int64_t) (lua_tonumber(L,-1)*1e8);
          lua_getglobal (L, "chance");
          chance = lua_tonumber (L, -1);
          lua_getglobal (L, "ResetOnWin");
          ResetOnWin= lua_toboolean (L, -1);
          lua_getglobal (L, "ResetOnLose");
          ResetOnLose= lua_toboolean (L, -1);
          lua_getglobal (L, "IncreaseOnWinPercent");
          IncreaseOnWinPercent=lua_tonumber(L, -1);
          lua_getglobal (L, "IncreaseOnLosePercent");
          IncreaseOnLosePercent=lua_tonumber(L, -1);
          lua_getglobal (L, "MaxPayIn");
          AutoMaxPayIn=(int64_t) (lua_tonumber(L, -1)*1e8);
          lua_getglobal (L, "ResetOnLoseMaxBet");
          ResetOnLoseMaxBet=lua_toboolean(L,-1);
          lua_getglobal (L, "StopOnLoseMaxBet");
          StopOnLoseMaxBet=lua_toboolean(L,-1);
          lua_getglobal (L, "StopMaxBalance");
          StopMaxBalance=(int64_t) (lua_tonumber(L,-1)*100000000);
          lua_getglobal (L, "StopMinBalance");
          StopMinBalance=(int64_t) (lua_tonumber(L,-1)*100000000);
          lua_getglobal (L, "StartingPayIn");
          StartingPayIn=(int64_t) (lua_tonumber(L,-1)*100000000);
          lua_getglobal (L, "info");
          infoval = (double) lua_tonumber (L, -1);
        }
        lua_settop (L, 0);
      }
    }
  }
  curl_easy_cleanup (curl);
  curl_global_cleanup ();

  if (luaopen) lua_close (L);  // not a manual session only
  endwin ();

  logfile = fopen ("luabot.log", "a+");
  fprintf (logfile, "%lu ", (unsigned long) time (NULL));
  fprintf (logfile, "won %i of %i bets (%.2f%%) at %.2f%% chance, ", wincount,
           betcount, (double) wincount / betcount * 100, chance);
  fprintf (logfile, "Balance: %.8f %s, ", (double) balance / 1e8, currency);
  fprintf (logfile, "%.8f refPayment,  ", (double) (wagered / 1e8 / 2000));
  fprintf (logfile, " bmax: %.8f ", (double) maxbalance / 1e8);
  fprintf (logfile, " lastmaxmax: %i ", lastmaxmax);
  fprintf (logfile, " MaxPayIn: %.8f  ", (double) MaxPayIn / 1e8);
  fprintf (logfile, " minstreak: %i, maxstreak: %i, maxdiff: %.8f", minstreak, maxstreak, (double) maxdiff / 1e8);
  fprintf (logfile, ", %s, ", botname);
  fprintf (logfile, "%s \n", exitReason);

  fclose (logfile);
  seconds = end.tv_sec - begin.tv_sec;
  printf ("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\033[1m\n\nluabot exit: %s\n", exitReason);
  printf (" Balance: %13.8f %s\n\033[0m", (double) balance / 1e8, currency);
  printf (" Profit: %13.8f %s (%.2f%%)\n\033[0m", (double) (balance-bstart+wdsum) / 1e8, currency, (double) (balance-bstart)*100/bstart);
  printf ("\n %i bets done in %i seconds (%.2f bets/sec) ", betcount,seconds,(double) betcount/seconds);
  printf ("\n ref payment: %.8f\n", (double) (wagered / 1e8 / 2000));
  printf (" bmax: %.8f ", (double) maxbalance / 1e8);
  printf (" MaxPayIn: %.8f\n", (double) MaxPayIn / 1e8);
  printf (" minstreak: %i, maxstreak: %i, ", minstreak, maxstreak);
  printf (" maxdiff: %.8f \n", (double) maxdiff / 1e8);

  if (kbd == 'D')
    exit (9);
  exit (0);
}
