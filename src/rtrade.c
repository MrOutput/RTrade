#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <alloca.h>
#include <ctype.h>
#include <oauth.h>
#include <curl/curl.h>
#include "lib/cJSON.h"

struct oauth_creds {
	char *c_key;
	char *c_secret;
	char *t_tok;
	char *t_secret;
};

struct quote {
	char *sym;
	double price;
};

struct oauth_creds creds;
struct quote *quotes;
int nquotes;
CURL *curl;

char *usage = "usage: rtrade -k consumer_key -t consumer_secret STOCK1 STOCK2 ...\n";

void xerror(char *m);
void parse_args(int argc, char **argv);
void parse_stocks(char **stocks);
char * strupper(char *s);
size_t write_callback(char *ptr, size_t size, size_t nmemb, void **userdata);
char * oauth_get(char *url);
void * xstrdup(const char *s);
int readin_str(char **line);
void update();
void render();

int
main(int argc, char *argv[])
{
	if (argc < 6) {
		xerror(usage);
	}

	nquotes = argc - 5;
	quotes = alloca(sizeof(struct quote) * nquotes);

	parse_args(argc, argv);
	parse_stocks(&argv[5]);

	curl_global_init(CURL_GLOBAL_SSL);
	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

		char *resp = oauth_get("https://etwssandbox.etrade.com/oauth/request_token?oauth_callback=oob");
		char **params = NULL;
		int len = oauth_split_url_parameters(resp, &params);
		free(resp);
		if (len == 3) {
			creds.t_tok = xstrdup(&params[0][12]);
			creds.t_secret = xstrdup(&params[1][19]);
			oauth_free_array(&len, &params);

			char fmt[] = "https://us.etrade.com/e/t/etws/authorize?key=%s&token=%s";
			int max = sizeof(fmt) + strlen(creds.c_key) + strlen(creds.t_tok);
			char *buf = alloca(max);
			snprintf(buf, max, fmt, creds.c_key, creds.t_tok);
			printf("%s\n\n", buf);

			char *code = NULL;
			printf("Code: ");
			len = readin_str(&code);

			char atfmt[] = "https://etwssandbox.etrade.com/oauth/access_token?oauth_verifier=%s";
			max = sizeof(atfmt) + len;
			buf = alloca(max);
			snprintf(buf, max, atfmt, code);
			free(code);

			resp = oauth_get(buf);
			printf("BUF: %s\n", buf);
			printf("RESP: %s\n", resp);

			params = NULL;
			len = oauth_split_url_parameters(resp, &params);

			if (len == 2) {
				free(creds.t_tok);
				free(creds.t_secret);
				creds.t_tok = xstrdup(&params[0][12]);
				creds.t_secret = xstrdup(&params[1][19]);
				oauth_free_array(&len, &params);

				for (;;) {
					update();
					render();
					sleep(5);
				}
			}
			free(creds.t_tok);
			free(creds.t_secret);
		}
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();
	return 0;
}

void
xerror(char *m)
{
	fputs(m, stderr);
	exit(EXIT_FAILURE);
}

void
parse_args(int argc, char **argv)
{
	struct option options[] = {
		{ "key", required_argument, NULL, 'k' },
		{ "secret", required_argument, NULL, 's' }
	};
	int i, c;
	while ((c = getopt_long(argc, argv, "k:s:", options, &i)) != -1) {
		switch (c) {
		case 'k':
			creds.c_key = optarg;
			break;
		case 's':
			creds.c_secret = optarg;
			break;
		}
	}
}

void
parse_stocks(char **stocks)
{
	for (int i = 0; i < nquotes; i++) {
		quotes[i].sym = strupper(stocks[i]);
		quotes[i].price = 0.0;
	}
}

char *
strupper(char *s)
{
	char *org = s;
	while (*s != '\0') {
		*s = toupper(*s);
		s++;
	}
	return org;
}


void *
xstrdup(const char *s)
{
	char *ptr = strdup(s);
	if (ptr == NULL) {
		exit(EXIT_FAILURE);
	}
	return ptr;
}

size_t
write_callback(char *ptr, size_t size, size_t nmemb, void **userdata)
{
	size_t total = size * nmemb;
	*userdata = xstrdup(ptr);
	strcpy(*userdata, ptr);
	return total;
}

char *
oauth_get(char *url)
{
	char *signed_url = oauth_sign_url2(url, NULL, OA_HMAC, NULL,
			creds.c_key, creds.c_secret, creds.t_tok, creds.t_secret);
	char *resp = NULL;
	curl_easy_setopt(curl, CURLOPT_URL, signed_url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
	CURLcode res = curl_easy_perform(curl);
	return resp;
}

int
readin_str(char **line)
{
	size_t n = 0;
	ssize_t r = getline(line, &n, stdin);
	(*line)[r-1] = '\0';
	return r - 1;
}

void
update()
{
	char *url = "https://etwssandbox.etrade.com/market/sandbox/rest/quote/goog,aapl.json&detailFlag=INTRADAY";
	char *reply;
	cJSON *root = NULL, *qdata = NULL;

	reply = oauth_get(url);
	root = cJSON_Parse(reply);
	free(reply);
	qdata = root->child->child->child;

	for (int i = 0; qdata != NULL; i++) {
		quotes[i].price = cJSON_GetObjectItem(qdata->child->next, "lastTrade")->valuedouble;
		qdata = qdata->next;
	}

	cJSON_Delete(root);
}

void
render()
{
	for (int i = 0; i < nquotes; i++) {
		printf("%s %f\n", quotes[i].sym, quotes[i].price);
	}
}
