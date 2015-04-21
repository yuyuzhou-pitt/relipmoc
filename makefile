codeGen:  y.tab.c lex.yy.c table.c proj2.c proj3.c proj4.c seman.c traverse.c table.c gen.c driver.c
	cc  -w -g -o codeGen y.tab.c proj2.c proj3.c proj4.c seman.c traverse.c table.c gen.c driver.c -lfl
y.tab.c:  grammar.y
	yacc -v grammar.y
lex.yy.c: lex.l 
	flex lex.l
clean: 
	rm lex.yy.c y.tab.c codeGen y.output
