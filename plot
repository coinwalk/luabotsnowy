#!/bin/bash
title=$(pwd|rev|cut -d/ -f1|rev)

currency=$(tail -n 2 betlog | grep -m1 -Eo "(btc|doge|ltc|eth)")

tail -f -n ${1:-1000} bnow.${currency} | stdbuf -i0 -o0 awk -F'[: ]'  -v s=${1:-1000} -v currency="${currency^^}" -v title="$title" '
	BEGIN { 
		printf "set xtics textcolor \"#ffff99\" font \"arial, 20\"\n"
		printf "set ytics textcolor \"#ffff99\"\n"
		printf "set style data lines\n"
		printf "set ylabel \"%s\" tc rgb \"#ffffaa\" font \"arial,20\"\n",currency
		printf "set grid lc rgb \"#666666\"\n"
		printf "set term x11\n"
		printf "set key tc rgb \"white\"\n"
		printf "set key outside right top vertical Right noreverse noenhanced autotitle\n"
		printf "set object 1 rectangle from screen 0,0 to screen 1,1 fillcolor rgb \"#1A1A1A\" behind\n"
		printf "set terminal x11 title \"bird1\" noraise\n"
		printf "set autoscale\n"
		f=-1-s; t=-1
	}
	{
		f++; t++;
		data[t%s]=$2
		if (t>s) {
			printf "set xrange [%i:%i]\n",f,t
			printf "plot \"-\" title \"%s\" lw 2 lc rgb \"#FF9900\"\n",title
			if (f<0) { f2=0 } else { f2=f };
			for (i=t;i>f2;i--) print i" "data[i%s]
			print "e"
		}
	}' | gnuplot -

