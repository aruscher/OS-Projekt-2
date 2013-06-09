install: shell

shell: dfs

dfs: dfs.c
	gcc -o dfs dfs.c

