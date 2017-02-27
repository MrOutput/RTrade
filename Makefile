default:
	mkdir bin
	cc src/rtrade.c lib/cJSON.c src/auth.c -o bin/rtrade -lssl -lm -loauth -lreadline -g --pedantic

release:
	mkdir bin
	cc src/rtrade.c lib/cJSON.c src/auth.c -o bin/rtrade -lssl -loauth -lreadline -O2 -s -lm

clean:
	rm -rf bin
