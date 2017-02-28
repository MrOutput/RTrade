#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <getopt.h>
#include <string.h>

#include <ctype.h>

#include "../lib/cJSON.h"
#include "auth.h"

extern char *c_key;
extern char *c_secret;

enum {
	ERROR,
	SUCCESS
};

int stock_count;
struct quote {
	char *symbol;
	double price;
};
struct quote *stocks;
char *final_uri;

char * quote_uri()
{
	char uri[] = "https://etwssandbox.etrade.com/market/sandbox/rest/quote/goog,aapl.json&detailFlag=INTRADAY";
	char *c = malloc(sizeof(uri));
	memcpy(c, uri, sizeof(uri));
	return c;
}

int show_quotes()
{
	char *reply;
	cJSON *root = NULL, *quote = NULL;

	reply = get(final_uri);
	if (!reply) {
		return ERROR;
	}
	root = cJSON_Parse(reply);
	if (!root) {
		return ERROR;
	}
	quote = root->child->child->child;
	if (!quote) {
		return ERROR;
	}
	while (quote) {
		printf(
			"%s %12f\n",
			cJSON_GetObjectItem(quote->child->next->next, "symbol")->valuestring,
			cJSON_GetObjectItem(quote->child->next, "lastTrade")->valuedouble
		);
		quote = quote->next;
	}
	printf("\n");
	free(reply);
	cJSON_Delete(root);
	return SUCCESS;
}

void errusage(void)
{
	char *m = "usage: rtrade -k consumer_key -t consumer_secret STOCK1 STOCK2 ...\n";
	fputs(m, stderr);
	exit(EXIT_FAILURE);
}

void parse_args(int argc, char **argv)
{
	struct option options[] = {
		{ "key", required_argument, NULL, 'k' },
		{ "secret", required_argument, NULL, 't' }
	};
	int i, c;
	while ((c = getopt_long(argc, argv, "k:t:", options, &i)) != -1) {
		switch (c) {
		case 'k':
			c_key = strdup(optarg);
			break;
		case 't':
			c_secret = strdup(optarg);
			break;
		case '?':
			errusage();
		}
	}
	if (c_key == NULL || c_secret == NULL) {
		errusage();
	}
}

void strupper(char *s)
{
	while (*s != '\0') {
		*s = toupper(*s);
		s++;
	}
}

void parse_stocks(char *stock[])
{
	stocks = malloc(sizeof(struct quote) * stock_count);

	for (int i = 0; i < stock_count; i++) {
		stocks[i].symbol = strdup(stock[i]);
		strupper(stocks[i].symbol);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 6) {
		errusage();
	}
	int status = SUCCESS;
	parse_args(argc, argv);
	stock_count = argc - 5;
	parse_stocks(&(argv[5]));
	final_uri = quote_uri();
	if (!authorize_app()) {
		puts("ERROR: Could not authorize.");
		return 1;
	}
	for (;;) {
		if (show_quotes()) {
			sleep(3);
		} else {
			return 1;
		}
	}
	return 0;
}
