install: shell

shell: shell.c
	gcc shell.c parser.c -g -o shell -lreadline -lncurses

