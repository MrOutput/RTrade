default:
	cc -g -Wall -Wpedantic -o rtrade -loauth -lm src/rtrade.c src/lib/cJSON.c

release:
	cc -O2 -s -o rtrade -loauth -lm src/rtrade.c src/lib/cJSON.c

clean:
	rm ./rtrade
