#include <sys/stat.h>
#include <unistd.h>
#include <err.h>

#include "mongoose.h"

int pledge(const char *promises, const char *execpromises);
int unveil(const char *path, const char *permissions);

static const char *s_listen_on = "ws://0.0.0.0:8000";
static const char *s_data_dir = "./flare_server_data";

static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        if (mg_http_match_uri(hm, "/ws")) {
            mg_ws_upgrade(c, hm, NULL);
        } else {
            mg_http_reply(c, 200, "Content-Type: text/plain\r\n", "%s\n", "hello from flare");
        }
    } else if (ev == MG_EV_WS_MSG) {
        // got websocket frame, rec data in wm->data. echo.
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
    }
}

int main(int argc, char** argv) {
    if (pledge("stdio inet", NULL) == -1) {
        err(1,"pledge");
    }

    if (mkdir(s_data_dir, 600) == -1) {
        err(1, "mkdir");
    }

    if (unveil(s_data_dir, "rw") == -1) {
        err(1, "unveil");
    }

    struct mg_mgr mgr;  // event mgr
    mg_mgr_init(&mgr);  // init event mgr

    printf("Starting WS listener");

    mg_http_listen(&mgr, s_listen_on, fn, NULL);  // http listener bound to fn
    for (;;) mg_mgr_poll(&mgr, 1000); // inf loop

    mg_mgr_free(&mgr);
    return 0;
}