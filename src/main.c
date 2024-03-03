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
    } else if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        mg_ws_upgrade(c, hm, NULL);
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

    if (unveil(s_base_dir, "rw") == -1) {
        err(1, "unveil");
    }
    
    if (pledge("stdio inet rpath", NULL) == -1) {
        err(1,"pledge");
    }

    
    struct mg_str certm = mg_file_read(&mg_fs_posix, s_cert_path);
    struct mg_str keym = mg_file_read(&mg_fs_posix, s_key_path);
    opts.cert = certm;
    opts.key = keym;

    struct mg_mgr mgr;  // event mgr

    mg_log_set(MG_LL_DEBUG);
    mg_mgr_init(&mgr);  // init event mgr

    printf("starting listener...");

    mg_http_listen(&mgr, s_listen_on, fn, (void *) 1);  // http listener bound to fn

    for (;;) mg_mgr_poll(&mgr, 1000); // inf loop

    mg_mgr_free(&mgr);
    return 0;
}