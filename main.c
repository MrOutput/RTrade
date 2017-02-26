#include <stdio.h>
#include <oauth.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

char *c_key	= "48a3488764a038f953bc59507a7fde83";
char *c_secret = "07b8a4a0e1debb0792d81dc0f994b652";
char *t_tok;
char *t_secret;

char * get(char *uri)
{
	char *req_url, *reply;

	req_url = oauth_sign_url2(uri, NULL, OA_HMAC, NULL, c_key, c_secret, t_tok, t_secret);
	reply = oauth_http_get(req_url, NULL);

	free(req_url);
	return reply;
}

int parse_auth_reply(char *reply)
{
	char **resp = NULL;
	int num = oauth_split_url_parameters(reply, &resp);
	if (num < 2) {
		return 1;
	}
	t_tok	= strdup(&(resp[0][12]));
	t_secret = strdup(&(resp[1][19]));

	free(resp);
	return 0;
}

int req_tok()
{
	char *req_tok_uri, *reply;

	req_tok_uri = "https://etwssandbox.etrade.com/oauth/request_token?oauth_callback=oob";
	reply = get(req_tok_uri);
	parse_auth_reply(reply);

	free(reply);
	return 0;
}

//https://us.etrade.com/e/etws/authorize?key={oauth_consumer_key}&token={oauth_token}
char * authorize()
{
	printf("AUTHORIZATION REQUIRED\nhttps://us.etrade.com/e/t/etws/authorize?key=%s&token=%s\n", c_key, t_tok);
	return readline("Verification Code: ");
}

//https://etws.etrade.com/oauth/access_token
//const char *req_tok_uri = "https://etwssandbox.etrade.com/oauth/request_token?oauth_callback=oob";
void acc_tok(char *v_code)
{
	char acc_tok_uri[] = "https://etwssandbox.etrade.com/oauth/access_token?oauth_verifier=";
	int len = strlen(v_code);
	char *uri = malloc(sizeof(acc_tok_uri) + len);
	memcpy(uri, acc_tok_uri, sizeof(acc_tok_uri) - 1);
	memcpy(uri + sizeof(acc_tok_uri) - 1, v_code, len);
	uri[sizeof(acc_tok_uri) + len - 1] = '\0';

	char *reply = reply = get(uri);

	free(t_tok);
	free(t_secret);
	parse_auth_reply(reply);

	free(v_code);
	free(uri);
	free(reply);
}

int authorize_app()
{
	req_tok();
	acc_tok(authorize());
	return 0;
}

int main(int argc, char const* argv[])
{
	char *reply;

	if (authorize_app() == 1) {
		return 1;
	}

	for (;;) {
		reply = get("https://etwssandbox.etrade.com/market/sandbox/rest/quote/GOOG,AAPL,INTC.json&detailFlag=FUNDAMENTAL");
		printf("%s\n\n", reply);
		free(reply);
		sleep(3);
	}

	return 0;
}
