default: clean
	mkdir bin
	cc src/rtrade.c lib/cJSON.c src/auth.c -o bin/rtrade -lssl -lm -loauth -g --pedantic

release: clean
	mkdir bin
	cc src/rtrade.c lib/cJSON.c src/auth.c -o bin/rtrade -lssl -loauth -O2 -s -lm

clean:
	rm -rf bin
