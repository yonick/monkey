/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Monkey HTTP Server
 *  ==================
 *  Copyright 2001-2014 Monkey Software LLC <eduardo@monkey.io>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef MK_HTTP_H
#define MK_HTTP_H

#include "mk_utils.h"
#include "mk_memory.h"
#include "mk_list.h"
#include "mk_file.h"
#include "mk_rbtree.h"
#include "mk_scheduler.h"
#include "mk_http_parser.h"

#define MK_CRLF "\r\n"

/* Request buffer chunks = 4KB */
#define MK_REQUEST_CHUNK (int) 4096
#define MK_REQUEST_DEFAULT_PAGE  "<HTML><HEAD><STYLE type=\"text/css\"> body {font-size: 12px;} </STYLE></HEAD><BODY><H1>%s</H1>%s<BR><HR><ADDRESS>Powered by %s</ADDRESS></BODY></HTML>"

/* Hard coded restrictions */
#define MK_HTTP_DIRECTORY_BACKWARD ".."

#define MK_METHOD_GET_STR       "GET"
#define MK_METHOD_POST_STR      "POST"
#define MK_METHOD_HEAD_STR      "HEAD"
#define MK_METHOD_PUT_STR       "PUT"
#define MK_METHOD_DELETE_STR    "DELETE"
#define MK_METHOD_OPTIONS_STR   "OPTIONS"

/* Headers */
#define RH_ACCEPT "Accept:"
#define RH_ACCEPT_CHARSET "Accept-Charset:"
#define RH_ACCEPT_ENCODING "Accept-Encoding:"
#define RH_ACCEPT_LANGUAGE "Accept-Language:"
#define RH_CONNECTION "Connection:"
#define RH_COOKIE "Cookie:"
#define RH_CONTENT_LENGTH "Content-Length:"
#define RH_CONTENT_RANGE "Content-Range:"
#define RH_CONTENT_TYPE	"Content-Type:"
#define RH_IF_MODIFIED_SINCE "If-Modified-Since:"
#define RH_HOST	"Host:"
#define RH_LAST_MODIFIED "Last-Modified:"
#define RH_LAST_MODIFIED_SINCE "Last-Modified-Since:"
#define RH_REFERER "Referer:"
#define RH_RANGE "Range:"
#define RH_USER_AGENT "User-Agent:"

#define MK_REQUEST_STATUS_INCOMPLETE -1
#define MK_REQUEST_STATUS_COMPLETED 0

#define EXIT_NORMAL 0
#define EXIT_ERROR -1
#define EXIT_ABORT -2
#define EXIT_PCONNECTION 24

/* Available methods */
#define MK_HTTP_METHOD_AVAILABLE   \
    MK_HTTP_METHOD_GET_STR "," MK_HTTP_METHOD_POST_STR "," \
    MK_HTTP_METHOD_HEAD_STR "," MK_HTTP_METHOD_PUT_STR "," \
    MK_HTTP_METHOD_DELETE_STR "," MK_HTTP_METHOD_OPTIONS_STR  \
    MK_CRLF

#define MK_HTTP_PROTOCOL_UNKNOWN (-1)
#define MK_HTTP_PROTOCOL_09 (9)
#define MK_HTTP_PROTOCOL_10 (10)
#define MK_HTTP_PROTOCOL_11 (11)

#define MK_HTTP_PROTOCOL_09_STR "HTTP/0.9"
#define MK_HTTP_PROTOCOL_10_STR "HTTP/1.0"
#define MK_HTTP_PROTOCOL_11_STR "HTTP/1.1"

extern const mk_ptr_t mk_http_method_get_p;
extern const mk_ptr_t mk_http_method_post_p;
extern const mk_ptr_t mk_http_method_head_p;
extern const mk_ptr_t mk_http_method_put_p;
extern const mk_ptr_t mk_http_method_delete_p;
extern const mk_ptr_t mk_http_method_options_p;
extern const mk_ptr_t mk_http_method_null_p;

extern const mk_ptr_t mk_http_protocol_09_p;
extern const mk_ptr_t mk_http_protocol_10_p;
extern const mk_ptr_t mk_http_protocol_11_p;
extern const mk_ptr_t mk_http_protocol_null_p;

/*
 * A HTTP session represents an incoming session
 * from a client, a session can be used for pipelined or
 * keepalive requests.
 */

struct mk_http_session
{
    int socket;
    int pipelined;              /* Pipelined request */
    int counter_connections;    /* Count persistent connections */
    int status;                 /* Request status */

    unsigned int body_size;
    unsigned int body_length;

    int body_pos_end;

    /* red-black tree head */
    struct rb_node _rb_head;

    /* head for mk_http_request list nodes, each request is linked here */
    struct mk_list request_list;

    /* while a http_request don't complete a request, it's linked here */
    struct mk_list request_incomplete;

    /* creation time for this HTTP session */
    time_t init_time;

    /* request body buffer */
    char *body;

    /* Initial fixed size buffer for small requests */
    char body_fixed[MK_REQUEST_CHUNK];

    /*
     * FIXME: in previous versions of Monkey we used to parse the complete request
     * for pipelined requests and generate a linked lists of request. With the new
     * parser we are taking the approach to parse one request and process it before
     * parsing others, from that point of view we should not need a linked list
     * of requests.
     *
     * Still testing...
     */
    struct mk_http_request sr_fixed;

    /*
     * Parser context: we only held one parser per connection
     * which is re-used everytime we have a new request.
     */
    struct mk_http_parser parser;
};

static inline void mk_http_status_completed(struct mk_http_session *cs)
{
    mk_bug(cs->status == MK_REQUEST_STATUS_COMPLETED);

    cs->status = MK_REQUEST_STATUS_COMPLETED;
    mk_list_del(&cs->request_incomplete);
}

int mk_http_method_check(mk_ptr_t method);
mk_ptr_t mk_http_method_check_str(int method);
int mk_http_method_get(char *body);

int mk_http_protocol_check(char *protocol, int len);
mk_ptr_t mk_http_protocol_check_str(int protocol);

int mk_http_init(struct mk_http_session *cs, struct mk_http_request *sr);
int mk_http_keepalive_check(struct mk_http_session *cs);

int mk_http_pending_request(struct mk_http_session *cs);
int mk_http_send_file(struct mk_http_session *cs, struct mk_http_request *sr);
int mk_http_request_end(int socket);

int mk_http_error(int http_status, struct mk_http_session *cs,
                  struct mk_http_request *sr);

/* http session */
void mk_http_session_remove(int socket);
struct mk_http_session *mk_http_session_get(int socket);
struct mk_http_session *mk_http_session_create(int socket,
                                              struct sched_list_node *sched);

/* event handlers */
int mk_http_handler_read(int socket, struct mk_http_session *cs);
int mk_http_handler_write(int socket, struct mk_http_session *cs);

void mk_http_request_free(struct mk_http_request *sr);
void mk_http_request_free_list(struct mk_http_session *cs);

void mk_http_request_ka_next(struct mk_http_session *cs);
void mk_http_request_init(struct mk_http_session *session,
                          struct mk_http_request *request);
struct mk_http_header *mk_http_header_get(int name, struct mk_http_request *req,
                                          const char *key, unsigned int len);

#endif
