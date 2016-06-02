#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <curl/curl.h>

static size_t write_response (void *contents, size_t size, size_t nmemb, void *userp) {
    const char *response = (char *)contents;
    size_t realsize = size * nmemb;
    lua_State *L = (lua_State *)userp;
    int len = luaL_len(L, -1);
    lua_pushinteger(L, len+1);
    lua_pushlstring(L, response, realsize);
    lua_settable(L, -3);

    return realsize;
}

static int get (lua_State *L) {
    const char *url = luaL_checkstring(L, 1);
    CURL *curl;

    lua_newtable(L); // result

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error loading cURL default initialization.\n");
	return 2;
    };

    curl = curl_easy_init();

    if (curl) {
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
//	curl_easy_setopt(curl, CURLOPT_URL, url);

	curl_easy_perform(curl);

	curl_easy_cleanup(curl);
    };

    curl_global_cleanup();

    return 1;
}

static const struct luaL_Reg curl_funcs[] = {
    {"get", get},
    {NULL, NULL}
};

int luaopen_lcurl (lua_State *L) {
    // create library
    luaL_newlib(L, curl_funcs);
    return 1;
}
