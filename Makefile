default: clean
	mkdir bin
	cc src/rtrade.c lib/cJSON.c -o bin/rtrade -lssl -lm -lcurl -loauth -g --pedantic

release: clean
	mkdir bin
	cc src/rtrade.c lib/cJSON.c -o bin/rtrade -lssl -loauth -O2 -lcurl -s -lm

clean:
	rm -rf bin
