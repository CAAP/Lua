#include <lua.h>
#include <lauxlib.h>

#include <curl/curl.h>


/*   ******************************   */


/*   ******************************   */

static void new_easy(lua_State *L) {
    CURL *curl = curl_easy_init();
    if (curl) {
	CURL **pcurl = (CURL **)lua_newuserdata(L, sizeof(CURL *));
	char *errbuff = (char *)lua_newuserdata(L, CURL_ERROR_SIZE);
	lua_setuservalue(L, -2);
	*pcurl = curl;
	// set its metatable
	luaL_getmetatable(L, "caap.curl.easy");
	lua_setmetatable(L, -2);
	// store human readable error messages
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuff);
    }
}

/*   ******************************   */

static void set_options(lua_State *L) {
    lua_newtable(L); // upvalue
    // BEHAVIOR
    lua_pushinteger(L, CURLOPT_VERBOSE); lua_setfield(L, -2, "verbose");
    lua_pushinteger(L, CURLOPT_HEADER); lua_setfield(L, -2, "header");
    lua_pushinteger(L, CURLOPT_NOPROGRESS); lua_setfield(L, -2, "noprogress");
    lua_pushinteger(L, CURLOPT_NOSIGNAL); lua_setfield(L, -2, "nosignal");
    lua_pushinteger(L, CURLOPT_WILDCARDMATCH); lua_setfield(L, -2, "wildcard");
    // OTHER
    lua_pushinteger(L, CURLOPT_PRIVATE); lua_setfield(L, -2, "private");
    lua_pushinteger(L, CURLOPT_SHARE); lua_setfield(L, -2, "share");
    lua_pushinteger(L, CURLOPT_TRANSFERTEXT); lua_setfield(L, -2, "text");
    lua_pushinteger(L, CURLOPT_DIRLISTONLY); lua_setfield(L, -2, "dironly");
    lua_pushinteger(L, CURLOPT_NOBODY); lua_setfield(L, -2, "nobody");
    lua_pushinteger(L, CURLOPT_INFILESIZE); lua_setfield(L, -2, "sendfile:size");
    lua_pushinteger(L, CURLOPT_MAXFILESIZE); lua_setfield(L, -2, "getfile:size");
    lua_pushinteger(L, CURLOPT_UPLOAD); lua_setfield(L, -2, "upload");
    // SSL & SECURITY
    lua_pushinteger(L, CURLOPT_SSLCERT); lua_setfield(L, -2, "ssl:cert");
    lua_pushinteger(L, CURLOPT_SSLCERTTYPE); lua_setfield(L, -2, "ssl:cert:type");
    lua_pushinteger(L, CURLOPT_SSLKEY); lua_setfield(L, -2, "ssl:key");
    lua_pushinteger(L, CURLOPT_SSLKEYTYPE); lua_setfield(L, -2, "ssl:key:type");
    lua_pushinteger(L, CURLOPT_SSL_EC_CURVES); lua_setfield(L, -2, "ssl:xcurves");
    lua_pushinteger(L, CURLOPT_SSL_VERIFYHOST); lua_setfield(L, -2, "ssl:verify:host");
    lua_pushinteger(L, CURLOPT_SSL_VERIFYPEER); lua_setfield(L, -2, "ssl:verify:peer");
    lua_pushinteger(L, CURLOPT_SSL_VERIFYSTATUS); lua_setfield(L, -2, "ssl:verify:status");
    lua_pushinteger(L, CURLOPT_CAINFO); lua_setfield(L, -2, "ssl:ca:bundle"); // path to file
    lua_pushinteger(L, CURLOPT_CAPATH); lua_setfield(L, -2, "ssl:ca:dir"); // path to dir holding CA's
    // CONNECTION
    lua_pushinteger(L, CURLOPT_TIMEOUT_MS); lua_setfield(L, -2, "conn:request:timeout"); // timeout for request
    lua_pushinteger(L, CURLOPT_CONNECTTIMEOUT_MS); lua_setfield(L, -2, "conn:connect:timeout"); // timeout for connection
    lua_pushinteger(L, CURLOPT_ACCEPTTIMEOUT_MS); lua_setfield(L, -2, "conn:accept:timeout"); // timeout for server's connect back to be accepted
    lua_pushinteger(L, CURLOPT_MAXCONNECTS); lua_setfield(L, -2, "conn:maxconn");
    lua_pushinteger(L, CURLOPT_CONNECT_ONLY); lua_setfield(L, -2, "conn:only"); // only connect
    lua_pushinteger(L, CURLOPT_USE_SSL); lua_setfield(L, -2, "conn:ssl");
    // HTTP
    lua_pushinteger(L, CURLOPT_ACCEPT_ENCODING); lua_setfield(L, -2, "http:accept:encoding");
    lua_pushinteger(L, CURLOPT_TRANSFER_ENCODING); lua_setfield(L, -2, "http:transfer:encoding");
    lua_pushinteger(L, CURLOPT_FOLLOWLOCATION); lua_setfield(L, -2, "http:redirects"); // follow redirects
    lua_pushinteger(L, CURLOPT_MAXREDIRS); lua_setfield(L, -2, "http:redirects:max"); // max number of redirects to follow
    lua_pushinteger(L, CURLOPT_REFERER); lua_setfield(L, -2, "http:referer");
    lua_pushinteger(L, CURLOPT_USERAGENT); lua_setfield(L, -2, "http:useragent");
    lua_pushinteger(L, CURLOPT_HTTPHEADER); lua_setfield(L, -2, "http:header");
    lua_pushinteger(L, CURLOPT_PUT); lua_setfield(L, -2, "http:put");
    lua_pushinteger(L, CURLOPT_HTTPGET); lua_setfield(L, -2, "http:get");
    lua_pushinteger(L, CURLOPT_POST); lua_setfield(L, -2, "http:post");
    lua_pushinteger(L, CURLOPT_POSTFIELDS); lua_setfield(L, -2, "http:post:fields");
    lua_pushinteger(L, CURLOPT_POSTFIELDSIZE); lua_setfield(L, -2, "http:post:size"); // 2 Gb max
    // AUTHENTICATION
    lua_pushinteger(L, CURLOPT_USERPWD); lua_setfield(L, -2, "username:password"); // username & password
    lua_pushinteger(L, CURLOPT_USERNAME); lua_setfield(L, -2, "username");
    lua_pushinteger(L, CURLOPT_PASSWORD); lua_setfield(L, -2, "password");
    lua_pushinteger(L, CURLOPT_HTTPAUTH); lua_setfield(L, -2, "authentication");
    lua_pushinteger(L, CURLAUTH_BASIC); lua_setfield(L, -2, "auth:basic");
    lua_pushinteger(L, CURLAUTH_DIGEST); lua_setfield(L, -2, "auth:digest");
    lua_pushinteger(L, CURLAUTH_NEGOTIATE); lua_setfield(L, -2, "auth:negotiate");
    lua_pushinteger(L, CURLAUTH_ANY); lua_setfield(L, -2, "auth:any");
    lua_pushinteger(L, CURLAUTH_ANYSAFE); lua_setfield(L, -2, "auth:safe"); // any except basic
    lua_pushinteger(L, CURLAUTH_ONLY); lua_setfield(L, -2, "auth:only"); // meta symbol, OR this w a single specific auth
    lua_pushinteger(L, CURLOPT_TSLAUTH_USERNAME); lua_setfield(L, -2, "tsl:username");
    lua_pushinteger(L, CURLOPT_TSLAUTH_PASSWORD); lua_setfield(L, -2, "tsl:password");
    // NETWORK
    lua_pushinteger(L, CURLOPT_URL); lua_setfield(L, -2, "url");
    lua_pushinteger(L, CURLOPT_PATH_AS_IS); lua_setfield(L, -2, "asis");
    lua_pushinteger(L, CURLOPT_DEFAULT_PROTOCOL); lua_setfield(L, -2, "default");
    lua_pushinteger(L, CURLOPT_TCP_KEEPALIVE); lua_setfield(L, -2, "keepalivet");
    lua_pushinteger(L, CURLOPT_TCP_KEEPIDLE); lua_setfield(L, -2, "keepidle");
    lua_pushinteger(L, CURLOPT_TCP_KEEPINTVL); lua_setfield(L, -2, "keepintvl");
    lua_pushinteger(L, CURLOPT_PROTOCOLS); lua_setfield(L, -2, "protocols");
    lua_pushinteger(L, CURLPROTO_DICT); lua_setfield(L, -2, "proto:dict");
    lua_pushinteger(L, CURLPROTO_FILE); lua_setfield(L, -2, "proto:file");
    lua_pushinteger(L, CURLPROTO_FTP); lua_setfield(L, -2, "proto:ftp");
    lua_pushinteger(L, CURLPROTO_FTPS); lua_setfield(L, -2, "proto:ftps");
    lua_pushinteger(L, CURLPROTO_HTTP); lua_setfield(L, -2, "proto:http");
    lua_pushinteger(L, CURLPROTO_HTTPS); lua_setfield(L, -2, "proto:https");
    lua_pushinteger(L, CURLPROTO_IMAP); lua_setfield(L, -2, "proto:imap");
    lua_pushinteger(L, CURLPROTO_IMAPS); lua_setfield(L, -2, "proto:imaps");
    lua_pushinteger(L, CURLPROTO_POP3); lua_setfield(L, -2, "proto:pop3");
    lua_pushinteger(L, CURLPROTO_POP3S); lua_setfield(L, -2, "proto:pop3s");
    lua_pushinteger(L, CURLPROTO_SCP); lua_setfield(L, -2, "proto:scp");
    lua_pushinteger(L, CURLPROTO_SMTP); lua_setfield(L, -2, "proto:smtp");
    lua_pushinteger(L, CURLPROTO_SMTPS); lua_setfield(L, -2, "proto:smtps");
    // ERROR
    lua_pushinteger(L, CURLOPT_); lua_setfield(L, -2, "");
    lua_pushinteger(L, CURLOPT_); lua_setfield(L, -2, "");
    lua_pushinteger(L, CURLOPT_); lua_setfield(L, -2, "");
    // CALLBACK
    lua_pushinteger(L, CURLOPT_WRITEFUNCTION); lua_setfield(L, -2, "callback:write:fun");
    lua_pushinteger(L, CURLOPT_WRITEDATA); lua_setfield(L, -2, "callback:write:data");
    lua_pushinteger(L, CURLOPT_READFUNCTION); lua_setfield(L, -2, "callback:read:fun");
    lua_pushinteger(L, CURLOPT_READDATA); lua_setfield(L, -2, "callback:read:data");
    lua_pushinteger(L, CURLOPT_SOCKOPTFUNCTION); lua_setfield(L, -2, "callback:sock:fun");
    lua_pushinteger(L, CURLOPT_SOCKOPTDATA); lua_setfield(L, -2, "callback:sock:data");
    lua_pushinteger(L, CURLOPT_OPENSOCKETFUNCTION); lua_setfield(L, -2, "callback:opensock:fun");
    lua_pushinteger(L, CURLOPT_OPENSOCKETDATA); lua_setfield(L, -2, "callback:opensock:data");
    lua_pushinteger(L, CURLOPT_CLOSESOCKETFUNCTION); lua_setfield(L, -2, "callback:closesock:fun");
    lua_pushinteger(L, CURLOPT_CLOSESOCKETDATA); lua_setfield(L, -2, "callback:closesock:data");
    lua_pushinteger(L, CURLOPT_PROGRESSFUNCTION); lua_setfield(L, -2, "callback:progress:fun");
    lua_pushinteger(L, CURLOPT_PROGRESSDATA); lua_setfield(L, -2, "callback:progress:data");
    lua_pushinteger(L, CURLOPT_HEADERFUNCTION); lua_setfield(L, -2, "");
    lua_pushinteger(L, CURLOPT_HEADERDATA); lua_setfield(L, -2, "");
    lua_pushinteger(L, CURLOPT_DEBUGFUNCTION); lua_setfield(L, -2, "");
    lua_pushinteger(L, CURLOPT_DEBUGDATA); lua_setfield(L, -2, "");
}

static int easy_setopt(lua_State *L) {
    CURL *curl = checkeasy(L);
    curl_easy_setopt(curl, opt, );
    lua_pushboolean(L, 1);
    return 1;
}

static int easy_perform(lua_State *L) {
    CURL *curl = checkeasy(L);
    lua_pushboolean(L, CURLE_OK == curl_easy_perform(curl));
    return 1;
}

static int easy_reset(lua_State *L) {
    CURL *curl = checkeasy(L);
    curl_easy_reset(curl);
    lua_pushboolean(L, 1);
    return 1;
}

static int easy_asstr(lua_State *L) {
    lua_pushliteral(L, "CURL easy (active)");
    return 1;
}

static int easy_gc(lua_State *L) {
    CURL *curl = checkeasy(L);
    if (curl != NULL) {
	curl_easy_cleanup(curl);
	curl = NULL;
    }
    return 0;
}

/*   ******************************   */

static int clean_up_all(lua_State *L) {
    curl_global_cleanup();
    return 0;
}

/*   ******************************   */

static const struct luaL_Reg curl_funcs[] = {
    {"__gc",	   clean_up_all},
    {NULL,	   NULL}
};

/*   ******************************   */

int luaopen_lcurl (lua_State *L) {
/*
    luaL_newmetatable(L, "caap.mg.connection");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, conn_meths, 0);
*/
    // create library
    luaL_newlib(L, curl_funcs);

    // initialize CURL
    curl_global_init(CURL_GLOBAL_ALL);

    return 1;
}
