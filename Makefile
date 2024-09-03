all:
	gcc src/*.c -I include/ -o out/file-syncer

clean:
	rm -rf out/*
