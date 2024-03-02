#include "mongoose.h"

static const char *s_listen_on = "ws://localhost:8000";
static const char *s_web_root = ".";

void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        // upgrade all of http to ws
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        mg_http_reply(c, 200, "", "flare_server\n", "%s");
        mg_ws_upgrade(c, hm, NULL);
    } else if (ev == MG_EV_WS_MSG) {
        // got websocket frame, rec data in wm->data. echo.
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
        
        
    }
}

int main(void) {
    struct mg_mgr mgr;  // event mgr
    mg_mgr_init(&mgr);  // init event mgr
    printf("Starting WS listener");
    mg_http_listen(&mgr, s_listen_on, fn, NULL);  // http listener bound to fn
    for (;;) mg_mgr_poll(&mgr, 1000); // inf loop
    mg_mgr_free(&mgr);
    return 0;
}