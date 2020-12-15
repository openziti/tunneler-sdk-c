/*
Copyright NetFoundry, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/**
 * @file ziti_tunneler.h
 * @brief Defines the macros, functions, typedefs and constants required to implement a Ziti
 * tunneler application.
 */

#ifndef ZITI_TUNNELER_SDK_ZITI_TUNNEL_H
#define ZITI_TUNNELER_SDK_ZITI_TUNNEL_H

#include <stdbool.h>
#include "uv.h"
#include "uv_mbed/queue.h"
#include "ziti/netif_driver.h"
#include "lwip/ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tunneler_ctx_s *tunneler_context;
typedef struct tunneler_io_ctx_s *tunneler_io_context;
const char * get_intercepted_address(const struct tunneler_io_ctx_s * tnlr_io);
typedef struct hosted_io_ctx_s *hosted_io_context;

typedef enum {
    CLIENT_CFG_V1,    // ziti-tunnel-client.v1
    SERVER_CFG_V1,    // ziti-tunnel-server.v1
    INTERCEPT_CFG_V1, // intercept.v1
    HOST_CFG_V1       // host.v1
} cfg_type_e;

typedef struct protocol_s {
    char *protocol;
    STAILQ_ENTRY(protocol_s) entries;
} protocol_t;

typedef struct address_s {
    char       str[UV_MAXHOSTNAMESIZE]; // hostname || ip || ip/prefix
    bool       is_hostname;
    ip_addr_t  ip;
    uint8_t    prefix_len;
    STAILQ_ENTRY(address_s) entries;
} address_t;

typedef struct port_range_s {
    int low;
    int high;
    char str[16]; // [123456-123456]
    STAILQ_ENTRY(port_range_s) entries;
} port_range_t;

/** data needed to intercept packets and dial the associated ziti service */
typedef struct intercept_ctx_s {
    const char *  service_name;
    const void *  ziti_ctx;
    STAILQ_HEAD(protocol, protocol_s)     protocols;
    STAILQ_HEAD(port_range, port_range_s) port_ranges;
    STAILQ_HEAD(address, address_s)       addresses;
    cfg_type_e    cfg_type;
    const void *  cfg;
    STAILQ_ENTRY(intercept_ctx_s) entries;
} intercept_ctx_t;

extern void intercept_ctx_add_protocol(intercept_ctx_t *ctx, const char *protocol);
/** parse address string as hostname|ip|cidr and add result to list of intercepted addresses */
extern address_t *intercept_ctx_add_address(tunneler_context tnlr_ctx, intercept_ctx_t *i_ctx, const char *address);
extern port_range_t *intercept_ctx_add_port_range(intercept_ctx_t *i_ctx, uint16_t low, uint16_t high);

struct io_ctx_s {
    tunneler_io_context   tnlr_io;
    void *                ziti_io; // context specific to ziti SDK being used by the app.
    const void *          ziti_ctx;
};

struct io_ctx_list_entry_s {
    struct io_ctx_s *io;
    SLIST_ENTRY(io_ctx_list_entry_s) entries;
};
SLIST_HEAD(io_ctx_list_s, io_ctx_list_entry_s);

typedef struct hosted_service_ctx_s {
    char *       service_name;
    const void * ziti_ctx;
    uv_loop_t *  loop;
    cfg_type_e   cfg_type;
    const void * cfg;
} *hosted_service_context;

/**
 * called when a client connection is intercepted.
 * implementations are expected to dial the service and return
 * context that will be passed to ziti_read/ziti_write */
typedef void * (*ziti_sdk_dial_cb)(const intercept_ctx_t *intercept_ctx, struct io_ctx_s *io);
typedef int (*ziti_sdk_close_cb)(void *ziti_io_ctx);
typedef ssize_t (*ziti_sdk_write_cb)(const void *ziti_io_ctx, void *write_ctx, const void *data, size_t len);
typedef void (*ziti_sdk_host_cb)(void *ziti_ctx, uv_loop_t *loop, const char *service_name, cfg_type_e cfg_type, const void *cfg);

typedef struct tunneler_sdk_options_s {
    netif_driver   netif_driver;
    ziti_sdk_dial_cb    ziti_dial;
    ziti_sdk_close_cb   ziti_close;
    ziti_sdk_write_cb   ziti_write;
    ziti_sdk_host_cb    ziti_host;
} tunneler_sdk_options;

typedef struct dns_manager_s dns_manager;
struct dns_manager_s {
    int (*apply)(dns_manager *dns, const char *host, const char *ip);
    void *data;
};

extern address_t *parse_address(const char *hn_or_ip_or_cidr, dns_manager *dns);

extern tunneler_context ziti_tunneler_init(tunneler_sdk_options *opts, uv_loop_t *loop);

/** called by tunneler application when it is done with a tunneler_context.
 * calls `stop_intercepting` for each intercepted service. */
extern void ziti_tunneler_shutdown(tunneler_context tnlr_ctx);

extern void ziti_tunneler_set_dns(tunneler_context tnlr_ctx, dns_manager *dns);

extern int ziti_tunneler_intercept(tunneler_context tnlr_ctx, intercept_ctx_t *i_ctx);

extern int ziti_tunneler_host(tunneler_context tnlr_ctx, const void *ziti_ctx, const char *service_name, cfg_type_e cfg_type, void *cfg);

extern void ziti_tunneler_stop_intercepting(tunneler_context tnlr_ctx, void *ziti_ctx, const char *service_name);

extern void ziti_tunneler_dial_completed(struct io_ctx_s *io_context, bool ok);

extern ssize_t ziti_tunneler_write(tunneler_io_context *tnlr_io_ctx, const void *data, size_t len);

struct write_ctx_s;
extern void ziti_tunneler_ack(struct write_ctx_s *write_ctx);

extern int ziti_tunneler_close(tunneler_io_context *tnlr_io_ctx_p);

extern int ziti_tunneler_close_write(tunneler_io_context *tnlr_io_ctx_p);

extern const char* ziti_tunneler_version();

extern void ziti_tunneler_init_dns(uint32_t mask, int bits);

#ifdef __cplusplus
}
#endif

#endif /* ZITI_TUNNELER_SDK_ZITI_TUNNEL_H */