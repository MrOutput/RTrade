default:
	cc main.c -lssl -loauth -lreadline -g --pedantic

release:
	cc main.c -lssl -loauth -lreadline -O2 -march=silvermont -s
