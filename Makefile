CC=/usr/bin/g++
CFLAGS=-Wall -g -pedantic-errors -Werror --coverage

MKMOD=${CC} ${CFLAGS} -c $^
MKEXE=${CC} ${CFLAGS} $^ -o $@

dotest:	test
	./test
	@make gcov 
	@make clean_gcov 2>&1 >/dev/null

test:	main.cpp crbUnit.o message.o
	${MKEXE}

onethread:
	${CC} -c -D _ONE_THREAD_ ${CFLAGS} crbUnit.cpp 
	make test

message.o:	message.cpp message.h
	${MKMOD}
crbUnit.o:	crbUnit.h crbUnit.cpp
	${MKMOD}
gcov:
	@gcov message.cpp | grep -A1 -e 'File.*message.cpp'

clean_gcov:	
	rm -f *.gcno *.gcda *.gcov

clean:	clean_gcov
	rm -f *.o *.gch
	rm -f test
