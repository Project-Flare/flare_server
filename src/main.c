#include <unistd.h>
#include <err.h>

#include "mongoose.h"

int pledge(const char *promises, const char *execpromises);

static const char *s_listen_on = "http://0.0.0.0:8000";

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

int main(void) {
    if (pledge("stdio inet", NULL) == -1) {
        err(1,"pledge");
    }

    struct mg_mgr mgr;  // event mgr
    mg_mgr_init(&mgr);  // init event mgr

    printf("Starting WS listener");

    mg_http_listen(&mgr, s_listen_on, fn, NULL);  // http listener bound to fn
    for (;;) mg_mgr_poll(&mgr, 1000); // inf loop

    mg_mgr_free(&mgr);
    return 0;
}