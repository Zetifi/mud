#pragma once

#include <stddef.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <net/if.h>

#define MUD_PATH_MAX    (32U)
#define MUD_PUBKEY_SIZE (32U)

struct mud;

enum mud_state {
    MUD_EMPTY = 0,
    MUD_DOWN,
    MUD_BACKUP,
    MUD_UP,
};

struct mud_stat {
    uint64_t val;
    uint64_t var;
    int setup;
};

struct mud_conf {
    unsigned long keepalive;
    unsigned long timetolerance;
    unsigned long kxtimeout;
    int tc;
};

struct mud_path {
    enum mud_state state;
    char interface_name[16];
    struct sockaddr_storage remote_address;
    struct mud_stat rtt;
    struct {
        uint64_t total;
        uint64_t bytes;
        uint64_t time;
        uint64_t rate;
        uint64_t loss;
    } tx, rx;
    struct {
        struct {
            uint64_t total;
            uint64_t bytes;
            uint64_t time;
            uint64_t acc;
            uint64_t acc_time;
        } tx, rx;
        uint64_t time;
        uint64_t sent;
        uint64_t set;
    } msg;
    struct {
        size_t min;
        size_t max;
        size_t probe;
        size_t last;
        size_t ok;
    } mtu;
    struct {
        uint64_t tx_max_rate;
        uint64_t rx_max_rate;
        uint64_t beat;
        unsigned char fixed_rate;
        unsigned char preferred; // True if this path should be used when all paths are lossy.
        unsigned char loss_limit; // Threshold for marking this path as lossy.
        uint64_t rtt_limit; // Threadhold for marking this path as laggy.
    } conf;
    uint64_t idle;
    unsigned char ok;
};

struct mud_bad {
    struct {
        struct sockaddr_storage addr;
        uint64_t time;
        uint64_t count;
    } decrypt, difftime, keyx;
};

struct mud *mud_create (struct sockaddr *);
void        mud_delete (struct mud *);

int mud_update    (struct mud *);
int mud_send_wait (struct mud *);

int    mud_get_fd  (struct mud *);
size_t mud_get_mtu (struct mud *);
int    mud_get_bad (struct mud *, struct mud_bad *);

int mud_set_key (struct mud *, unsigned char *, size_t);
int mud_get_key (struct mud *, unsigned char *, size_t *);

int mud_set_aes  (struct mud *);
int mud_set_conf (struct mud *, struct mud_conf *);

int mud_set_state(
    struct mud *mud,
    char interface_name[IFNAMSIZ],
    enum mud_state state,
    unsigned long tx_max_rate,
    unsigned long rx_max_rate,
    unsigned long beat,
    unsigned char fixed_rate,
    unsigned char preferred,
    unsigned char loss_limit,
    uint64_t rtt_limit);

int mud_peer (struct mud *, struct sockaddr *);

int mud_recv (struct mud *, void *, size_t);
int mud_send (struct mud *, const void *, size_t);

struct mud_path *mud_get_paths(struct mud *, unsigned *);
