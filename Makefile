default:
	cc rtrade.c cJSON.c auth.c -o rtrade -lssl -lm -loauth -lreadline -g --pedantic

release:
	cc rtrade.c cJSON.c auth.c -o rtrade -lssl -loauth -lreadline -O2 -s -lm
