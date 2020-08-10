#public: parse
#	g++ ast-public.c poise.tab.c lex.yy.c -ll -o ./poise-public --std=c++0x

poise: parse
	g++ ast-tofino.c poise.tab.c lex.yy.c -ll -o ./poise-tofino --std=c++0x

parse:
	bison -d poise.y
	flex poise.l

clean:
	rm -rf poise.tab.c poise.tab.h lex.yy.c
	rm -rf poise-public poise-tofino poise
