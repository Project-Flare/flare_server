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

static const char *s_tls_cert =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDazCCAlOgAwIBAgIUOeKuZSNfphkK21nCFsW15GJO6CswDQYJKoZIhvcNAQEL\n"
    "BQAwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM\n"
    "GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDAeFw0yNDAzMDMyMTM2MjdaFw0yNDA0\n"
    "MDIyMTM2MjdaMEUxCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw\n"
    "HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggEiMA0GCSqGSIb3DQEB\n"
    "AQUAA4IBDwAwggEKAoIBAQCv1FRLKGrnUT+iFZ8rY6o327Dql0CX2R1PFrpi8+xK\n"
    "jVM9CqNOLy++Ie1EObhKNBAkgBxq6Pn2aypQO0COzKyxAuiVtrYMa6qATzJjK3/h\n"
    "emSd6c2hoks35enf+Vxg4GPEydK6KGiUYP7fAvVZiDqZzh65nHqOH4iSxJEOfwsW\n"
    "mb2Tfc43wjPxt1/6fC9/y1XQ9tiPKUoA41h5hvfwYB9puabXBS5ZUdX0DgLp9F5y\n"
    "HyuvmKSWEz5yeQNhH1Y+o5hA/B8UCnxPNNmyRC7md16E7W93YUstoOyL9JTde9uM\n"
    "vHIikLj6PDziTYDF5ZqSugNxk9IDfjC1TENz5PgP7AULAgMBAAGjUzBRMB0GA1Ud\n"
    "DgQWBBQKHQ0uBDtAnJqydLL42t267HOTMDAfBgNVHSMEGDAWgBQKHQ0uBDtAnJqy\n"
    "dLL42t267HOTMDAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBA\n"
    "uOHBKEnqClzy7U5kwqPd0VgO0AYJEOsHb5CljGgqfUdHkrE9YZUUPRqKGvznZtJV\n"
    "rirs1cEVeknhQZZzkSW2k+O75nc4UJjfK9U+yAsTn4oiNG+cfjioMGfcGqZ5IpFo\n"
    "kprcKiZKVCA8e2ZzEcFjigezdQJqNChpTIKYWeVDc4A6OC7cga93SXflom+QQ4Oi\n"
    "yiD0qQRz7qkNq/4ZCNEC53rykLvixQRg/NIW2CVdYmJ+NotrjY1U0rgiZeZ3MCox\n"
    "9pPVvvhY1Kd6sIs3c2UKBOcMW0DiSHMquT8HfxurgNSg82Zgl5FgoqRr3DE3IsXh\n"
    "XiAmaxugY2jmlxRPJOmk\n"
    "-----END CERTIFICATE-----\n";


static const char *s_tls_key =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQCv1FRLKGrnUT+i\n"
    "FZ8rY6o327Dql0CX2R1PFrpi8+xKjVM9CqNOLy++Ie1EObhKNBAkgBxq6Pn2aypQ\n"
    "O0COzKyxAuiVtrYMa6qATzJjK3/hemSd6c2hoks35enf+Vxg4GPEydK6KGiUYP7f\n"
    "AvVZiDqZzh65nHqOH4iSxJEOfwsWmb2Tfc43wjPxt1/6fC9/y1XQ9tiPKUoA41h5\n"
    "hvfwYB9puabXBS5ZUdX0DgLp9F5yHyuvmKSWEz5yeQNhH1Y+o5hA/B8UCnxPNNmy\n"
    "RC7md16E7W93YUstoOyL9JTde9uMvHIikLj6PDziTYDF5ZqSugNxk9IDfjC1TENz\n"
    "5PgP7AULAgMBAAECggEABvR5fRuVLEniICbbdpuToRqsjdWmdXCIy0G/5P13DEqO\n"
    "JKz7YV1Wdck2bxYavGdWIst6fvQbkcFoGWeumbduNqDsfaI440GNq1OhT5uUjGEL\n"
    "V9URZY0GN/4n2SHY+JqyTOzFdshLErYffsd0LbLIy/VY8R3ybxVqO+al7OfePWo4\n"
    "nE77yixfE8iflVtuGje20g09AEKej6DF6yTcp0OyT77Ue/AGw8K5O5VZ78tgksSc\n"
    "q5LyDmrqDxGXiRfCmsMZESiQtPYcO3p0RzcUVLbPrhtdQAHOaw2FgA6RA+ZQABih\n"
    "cocXoRJs7vLWxZuB+eU+LIYaQuHQvlrsPZt/PYYGmQKBgQDcuFyUd0v7Yr2gE1rg\n"
    "uMk2PQySsjPbUbULvgrDkRcK9gLpPtaVau9qcZW37XSZ4UPn1lcGgLK4bwsu/lJA\n"
    "06RxqCP3pirvc/PVSgj+wpcqBle7l202KzZN/dXZzyzTpA0cOpkpviloVRMpcA6d\n"
    "U3szGlZtddXveTmVQbb6kUNorQKBgQDL7xXCtNNzoVcZvKHWoU2IpBJk68a1ATYc\n"
    "8bjb2Cg8sxDx7GaNwp8nTpmWSY+hdhvwDhgUQTF4WrHxOE7oVctSjLN/RGHaDk6i\n"
    "30I+i6agTsBLr9r09i1WikuGPjRPyyDhZSpFHvgRWK5WXpnnBVJcKi5nUed+WIOH\n"
    "ZlBndFRDlwKBgAC2x/Xi/OfvRdXCukSH1H7Ma8H9uZU4CnHjR32idoSejxvaDC+n\n"
    "jOa3P1i89+eXhvS1CMsIl4tpMIwSXom/JYUImjUu4Gyt31gNSUIptvt30cjVkJDg\n"
    "SuitrYdq2CZHHZO9zMkJyHCB9fHoXuO7ZWag6y6ndu2zYrBM6h6dYa5xAoGAAZCx\n"
    "MxBeOxn26CypdVNBnBXeSkYA/Wyn9KrqR02uaWDadXDiDJ58yDlzNlMUNiII7tu8\n"
    "1ZL2hTz09qv+9wuJhvWrfwOQWLMZaWibQo2h1sMj+LC91nl5OZvQHpSlpCMj+nbj\n"
    "TyqArulmVSVeuwYwbqKoPoTDaRXW1Jg1b8XVy7sCgYBSPufr+EnoJpd10Wef/C6R\n"
    "cSp7v2B9sNdeAbq9WOAbOPhs07aN9V+QkypxqkpBZBi7m/DoDJVIwzGmJKywFAvs\n"
    "jqMQ1Bw+tuFOOrhb3HC18CYInDc8L/IyTcucInbjjgU+BcXoQ87sWAjBRg6MoEwv\n"
    "Zlc525zF+fXT4zBMj2pX0Q==\n"
    "-----END PRIVATE KEY-----\n";


static void fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_ACCEPT && c->fn_data != NULL) {
    struct mg_tls_opts opts = {
        .cert = mg_str(s_tls_cert),
        .key = mg_str(s_tls_key)
    };
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

    struct mg_mgr mgr;  // event mgr

    mg_log_set(MG_LL_DEBUG);
    mg_mgr_init(&mgr);  // init event mgr

    printf("starting listener...");

    mg_http_listen(&mgr, s_listen_on, fn, (void *) 1);  // http listener bound to fn

    for (;;) mg_mgr_poll(&mgr, 1000); // inf loop

    mg_mgr_free(&mgr);
    return 0;
}