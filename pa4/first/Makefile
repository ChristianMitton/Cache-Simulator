all: one

one: first.c
	gcc -lm -Wall -Werror -fsanitize=address -std=c99 first.c -o first

clean:
	rm -rf first
