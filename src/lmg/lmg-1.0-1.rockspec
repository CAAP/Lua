package = "LMG"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "Mongoose web API wrapper"
}

dependencies = {
    "lua >= 5.2"
}

--[[
//MG_ENABLE_LWIP	0	Use LWIP low-level API instead of BSD sockets
//MG_ENABLE_SOCKET	1	Use BSD socket low-level API
//MG_ENABLE_MBEDTLS	0	Enable Mbed TLS library
//MG_ENABLE_OPENSSL	0	Enable OpenSSL library
//MG_ENABLE_FS	1	Enable API that use filesystem, like mg_http_send_file()
//MG_ENABLE_IPV6	0	Enable IPv6
//MG_ENABLE_LOG	1	Enable LOG() macro
//MG_ENABLE_MD5	0	Use native MD5 implementation
//MG_ENABLE_DIRECTORY_LISTING	0	Enable directory listing for HTTP server
//MG_ENABLE_HTTP_DEBUG_ENDPOINT	0	Enable /debug/info debug URI
//MG_ENABLE_SOCKETPAIR	0	Enable mg_socketpair() for multi-threading
//MG_IO_SIZE	512	Granularity of the send/recv IO buffer growth
//MG_MAX_RECV_BUF_SIZE	(3 * 1024 * 1024)	Maximum recv buffer size
//MG_MAX_HTTP_HEADERS	40	Maximum number of HTTP headers
--]]

build = {
    type = "cmake",
    variables = {
	CMAKE_C_FLAGS	   = "-O2 -fPIC -W -Wall -pedantic -DMG_ENABLE_OPENSSL",
    },
}

