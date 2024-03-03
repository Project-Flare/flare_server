#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>

#include "mongoose.h"

int pledge(const char *promises, const char *execpromises);
int unveil(const char *path, const char *permissions);

static const char *s_listen_on = "https://0.0.0.0:8443";

static const char *s_base_dir = "./flare_data";
static const char *s_cert_path = "./flare_data/cert.pem";
static const char *s_key_path = "./flare_data/key.pem";

struct mg_tls_opts opts;

static void fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_ACCEPT && c->fn_data != NULL) {
    mg_tls_init(c, &opts);
  }
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    if (mg_http_match_uri(hm, "/api/stats")) {
      // Print some statistics about currently established connections
      mg_printf(c, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
      mg_http_printf_chunk(c, "ID PROTO TYPE      LOCAL           REMOTE\n");
      for (struct mg_connection *t = c->mgr->conns; t != NULL; t = t->next) {
        mg_http_printf_chunk(c, "%-3lu %4s %s %M %M\n", t->id,
                             t->is_udp ? "UDP" : "TCP",
                             t->is_listening  ? "LISTENING"
                             : t->is_accepted ? "ACCEPTED "
                                              : "CONNECTED",
                             mg_print_ip, &t->loc, mg_print_ip, &t->rem);
      }
      mg_http_printf_chunk(c, "");  // Don't forget the last empty chunk
    } else if (mg_http_match_uri(hm, "/api/f2/*")) {
      mg_http_reply(c, 200, "", "{\"result\": \"%.*s\"}\n", (int) hm->uri.len,
                    hm->uri.ptr);
    }
  }
}

// static void fn(struct mg_connection *c, int ev, void *ev_data) {
//     if (ev == MG_EV_ACCEPT && mg_url_is_ssl(s_listen_on)) {
//         struct mg_str cert_data = mg_file_read(&mg_fs_posix, s_cert_path);
//         struct mg_str privkey_data = mg_file_read(&mg_fs_posix, s_key_path);
//         struct mg_tls_opts ops = { .cert = cert_data, .key = privkey_data };

//         mg_tls_init(c, &ops);

//         // free((void *) cert_data.ptr);
//         // free((void *) privkey_data.ptr);
//     } else if (ev == MG_EV_TLS_HS) {
//         MG_INFO(("TLS handshake done! Sending EHLO again"));
//         mg_printf(c, "EHLO myname\r\n");
//     } else if (ev == MG_EV_HTTP_MSG) {
//         mg_http_reply(c, 200, "Content-Type: text/plain\r\n", "%s\n", "hello from flare");
//     }

//     // if (ev == MG_EV_OPEN) {
//     //     printf("open conn.");
//     // } else if (ev == MG_EV_READ) {
//     // }
    
//     // if (ev == MG_EV_HTTP_MSG) {
//     //     struct mg_http_message *hm = (struct mg_http_message *) ev_data;
//     //     if (mg_http_match_uri(hm, "/ws")) {
//     //         mg_ws_upgrade(c, hm, NULL);
//     //     } else if (mg_http_match_uri(hm, "/test")) {
//     //         struct mg_http_serve_opts opts = {
//     //             .mime_types = "html=text/html",
//     //         };
//     //         mg_http_serve_file(c, ev_data, "./flare_data/test.html", &opts);
//     //     } else {
//     //         mg_http_reply(c, 200, "Content-Type: text/plain\r\n", "%s\n", "hello from flare");
//     //     }
//     // } else if (ev == MG_EV_WS_MSG) {
//     //     // got websocket frame, rec data in wm->data. echo.
//     //     struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
//     //     mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
//     // }
// }

char* concat(const char *str_1, const char *str_2)
{
    char *r = malloc(strlen(str_1) + strlen(str_2) + 1); // nt

    strcpy(r, str_1);
    strcat(r, str_2);
    return r;
}

int main(int argc, char** argv) {
    // attempt to create the dirs if not found
    char *create_data_dir = concat("mkdir -p ", s_base_dir);
    system(create_data_dir);
    free((void*) create_data_dir);

    if (unveil(s_base_dir, "rw") == -1) {
        err(1, "unveil");
    }
    
    if (pledge("stdio inet rpath", NULL) == -1) {
        err(1,"pledge");
    }

    printf("attempting to read tls key and cert\n");
    struct mg_str cert = mg_file_read(&mg_fs_posix, s_cert_path);
    struct mg_str key = mg_file_read(&mg_fs_posix, s_key_path);
    opts.key = key;
    opts.cert = cert;

    struct mg_mgr mgr;  // event mgr

    mg_log_set(MG_LL_DEBUG);
    mg_mgr_init(&mgr);  // init event mgr

    printf("starting listener...");

    mg_http_listen(&mgr, s_listen_on, fn, (void *) 1);  // http listener bound to fn

    for (;;) mg_mgr_poll(&mgr, 1000); // inf loop

    mg_mgr_free(&mgr);
    return 0;
}