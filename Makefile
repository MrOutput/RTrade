default:
	cc -g --pedantic -o rtrade -lcurl -loauth -lm src/rtrade.c src/lib/cJSON.c

release:
	cc -O2 -s -o rtrade -lcurl -loauth -lm src/rtrade.c src/lib/cJSON.c

clean:
	rm ./rtrade
