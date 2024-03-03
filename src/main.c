#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>

#include "mongoose.h"

int pledge(const char *promises, const char *execpromises);
int unveil(const char *path, const char *permissions);

static const char *s_listen_on = "ws://0.0.0.0:8000";

static const char *s_base_dir = "./flare_data";
static const char *s_cert_path = "./flare_data/cert.pem";
static const char *s_key_path = "./flare_data/key.pem";

struct mg_tls_opts tls_opts;
bool tls_initialized = false;

static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_CONNECT && tls_initialized) {
        mg_tls_init(c, &tls_opts);
    } else if (ev == MG_EV_ACCEPT) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        if (mg_http_match_uri(hm, "/ws")) {
            mg_ws_upgrade(c, hm, NULL);
        } else if (mg_http_match_uri(hm, "/test")) {
            struct mg_http_serve_opts opts = {
                .mime_types = "html=text/html",
            };
            mg_http_serve_file(c, ev_data, "./flare_data/test.html", &opts);
        } else {
            mg_http_reply(c, 200, "Content-Type: text/plain\r\n", "%s\n", "hello from flare");
        }
    } else if (ev == MG_EV_WS_MSG) {
        // got websocket frame, rec data in wm->data. echo.
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
    }
}

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
    
    printf("attempting to read tls files...\n");
    struct mg_str cert_data = mg_file_read(&mg_fs_posix, s_cert_path);
    struct mg_str privkey_data = mg_file_read(&mg_fs_posix, s_key_path);
    tls_opts.cert = cert_data;
    tls_opts.key = privkey_data;
    tls_initialized = cert_data.len > 0 && privkey_data.len > 0;

    printf("pk: %s\n", privkey_data.ptr);
    printf("crt: %s\n", cert_data.ptr);

    if (unveil(s_base_dir, "rw") == -1) {
        err(1, "unveil");
    }
    
    if (pledge("stdio inet rpath", NULL) == -1) {
        err(1,"pledge");
    }

    struct mg_mgr mgr;  // event mgr
    mg_mgr_init(&mgr);  // init event mgr

    printf("starting listener...");

    mg_http_listen(&mgr, s_listen_on, fn, NULL);  // http listener bound to fn

    for (;;) mg_mgr_poll(&mgr, 1000); // inf loop

    mg_mgr_free(&mgr);
    free((void *) cert_data.ptr);
    free((void *) privkey_data.ptr);
    return 0;
}