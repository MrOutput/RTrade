#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <oauth.h>
#include "lib/cJSON.h"

struct oauth_creds {
	char *c_key;
	char *c_secret;
	char *t_tok;
	char *t_secret;
};

struct quote {
	char *sym;
	int len;
	double price;
};

struct oauth_creds creds;
struct quote *quotes;
int nquotes;
int sdelay = 3;
char *usage = "usage: rtrade -k consumer_key -t consumer_secret [-d delay_sec] STOCK1 STOCK2 ...\n";
char *quote_url;

void xerror(char *m);
int parse_args(int argc, char **argv);
void parse_stocks(char **stocks);
int strupper(char *s);
char * oauth_get(char *url);
void oauth_creds_get(char *url);
void * xstrdup(const char *s);
void * xmalloc(size_t s);
int readin_str(char **line);
void update();
void render();
void set_quote_url();

int
main(int argc, char *argv[])
{
	if (argc < 6) {
		xerror(usage);
	}
	int progargs = parse_args(argc, argv);
	nquotes = argc - progargs;
	if (nquotes > 25) {
		xerror("Only 25 stocks are allowed.");
	}
	quotes = xmalloc(sizeof(struct quote) * nquotes);
	parse_stocks(&argv[progargs]);
	set_quote_url();

	oauth_creds_get("https://etwssandbox.etrade.com/oauth/request_token?oauth_callback=oob");

	char fmt[] = "https://us.etrade.com/e/t/etws/authorize?key=%s&token=%s";
	int max = sizeof(fmt) + strlen(creds.c_key) + strlen(creds.t_tok);
	char *buf = xmalloc(max);
	snprintf(buf, max, fmt, creds.c_key, creds.t_tok);
	printf("%s\n\n", buf);

	char *code = NULL;
	printf("Code: ");
	int len = readin_str(&code);

	char atfmt[] = "https://etwssandbox.etrade.com/oauth/access_token?oauth_verifier=%s";
	max = sizeof(atfmt) + len;
	buf = xmalloc(max);
	snprintf(buf, max, atfmt, code);
	free(code);

	oauth_creds_get(buf);

	for (;;) {
		update();
		render();
		sleep(sdelay);
	}
	return 0;
}

void
xerror(char *m)
{
	fputs(m, stderr);
	exit(EXIT_FAILURE);
}

int
parse_args(int argc, char **argv)
{
	struct option options[] = {
		{ "key", required_argument, NULL, 'k' },
		{ "secret", required_argument, NULL, 's' },
		{ "delay", required_argument, NULL, 'd' }
	};
	int i, c;
	int hasdelay = 0;
	while ((c = getopt_long(argc, argv, "k:s:d:", options, &i)) != -1) {
		switch (c) {
		case 'k':
			creds.c_key = optarg;
			break;
		case 's':
			creds.c_secret = optarg;
			break;
		case 'd':
			sdelay = atoi(optarg);
			hasdelay = 1;
			break;
		}
	}
	return ((hasdelay) ? 7 : 5);
}

void
parse_stocks(char **stocks)
{
	int len;
	for (int i = 0; i < nquotes; i++) {
		len = strupper(stocks[i]);
		quotes[i].sym = stocks[i];
		quotes[i].price = 0.0;
		quotes[i].len = len;
	}
}

int
strupper(char *s)
{
	int i;
	for (i = 0; *s != '\0'; i++, s++) {
		*s = toupper(*s);
	}
	return i;
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
	char * resp = oauth_http_get2(signed_url, NULL, NULL);
	free(signed_url);
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
	char *reply;
	cJSON *root = NULL, *qdata = NULL;

	reply = oauth_get(quote_url);
	root = cJSON_Parse(reply);

	qdata = root->child->child;

	//traverse into array if need be
	if (nquotes > 1) {
		qdata = qdata->child;
	}

	for (int i = 0; qdata != NULL; i++) {
		quotes[i].price = cJSON_GetObjectItem(qdata->child->next, "lastTrade")->valuedouble;
		qdata = qdata->next;
	}

	free(reply);
	cJSON_Delete(root);
}

void
render()
{
	printf("\n");
	for (int i = 0; i < nquotes; i++) {
		printf("%6s %10.2f\n", quotes[i].sym, quotes[i].price);
	}
}

void *
xmalloc(size_t s)
{
	char *ptr = malloc(s);
	if (ptr == NULL) {
		exit(EXIT_FAILURE);
	}
	return ptr;
}

void
set_quote_url()
{
	char base[] = "https://etwssandbox.etrade.com/market/sandbox/rest/quote/";
	char suf[] = ".json&detailFlag=INTRADAY";
	char *midbuf;

	int size = nquotes;//init for commas and \0

	for (int i = 0; i < nquotes; i++) {
		size += quotes[i].len;
	}
	midbuf = xmalloc(size);
	int len = sizeof(base) - 1 + sizeof(suf) - 1 + size + 1;
	quote_url = xmalloc(len);

	char *m = midbuf;
	for (int i = 0; i < nquotes; i++) {
		memcpy(m, quotes[i].sym, quotes[i].len);
		m += quotes[i].len;
		*m = ',';
		m++;
	}
	midbuf[size - 1] = '\0';
	snprintf(quote_url, len, "%s%s%s", base, midbuf, suf);
	free(midbuf);
}

void
oauth_creds_get(char *url)
{
	char *resp = oauth_get(url);
	char **params = NULL;
	int len = oauth_split_url_parameters(resp, &params);

	if (creds.t_tok) {
		free(creds.t_tok);
	}
	if (creds.t_secret) {
		free(creds.t_secret);
	}
	creds.t_tok = xstrdup(&params[0][12]);
	creds.t_secret = xstrdup(&params[1][19]);

	oauth_free_array(&len, &params);
	free(resp);
}
