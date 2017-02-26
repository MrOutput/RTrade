#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cJSON.h"
#include "auth.h"

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

int main(int argc, char const* argv[])
{
	if (authorize_app() == 1) {
		return 1;
	}
	for (;;) {
		show_quotes();
		sleep(3);
	}
	return 0;
}
