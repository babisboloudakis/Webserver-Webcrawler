all: webserver webcrawler

webserver: webserver.o intList.o
	@echo '* Compiling webserver ...'
	@gcc -o webserver webserver.o intList.o -pthread
	@echo '* Done with webserver ...'

webcrawler: webcrawler.o wordList.o
	@echo '* Compiling webcrawler ...'
	@gcc -o webcrawler webcrawler.o wordList.o -pthread
	@echo '* Done with webcrawler ...'

webcrawler.o: ./src/webcrawler.c
	@gcc -c ./src/webcrawler.c

webserver.o: ./src/webserver.c
	@gcc -c ./src/webserver.c

intList.o: ./src/intList.c
	@gcc -c ./src/intList.c

wordList.o: ./src/wordList.c
	@gcc -c ./src/wordList.c

clean:
	@echo '* Cleaning ...'
	@rm -f ./webserver *.o ./webcrawler
	@echo '* Done ...'