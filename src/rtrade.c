#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <getopt.h>
#include <string.h>

#include "../lib/cJSON.h"
#include "auth.h"

extern char *c_key;
extern char *c_secret;

void show_quotes()
{
	char *reply, *uri;
	cJSON *root, *quote;

	uri = "https://etwssandbox.etrade.com/market/sandbox/rest/quote/GOOG,AAPL,INTC.json&detailFlag=INTRADAY";
	reply = get(uri);

	root = cJSON_Parse(reply);
	quote = root->child->child->child;

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

int main(int argc, char *argv[])
{
	parse_args(argc, argv);
	if (!authorize_app()) {
        return 1;
    }
	for (;;) {
		show_quotes();
		sleep(3);
	}
	return 0;
}
