# Protocol

This document describes the protocol used by glorytun.

MUD aggregates a set of paths into a single interface with network bonding and fast failover support.

Time is stored in microseconds since the epoch. The MUD_ONE_MSEC, MUD_ONE_SEC, and MUD_ONE_MIN macros are shortcuts for useful times.

A path is represented using this structure.

```
struct mud_path {
    enum mud_state state;
    struct sockaddr_storage local_addr; // Local (for client or server or current process???) address for path.
    struct sockaddr_storage addr; // 
    struct sockaddr_storage r_addr;
    struct mud_stat rtt; // Rtt in glorytun units.
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
        uint64_t time; // Time that last control message was sent.
        uint64_t sent; // Number of control messages sent since a response was received.
        uint64_t set;
    } msg; // Statistics about control messages sent/received.
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
        unsigned char loss_limit;
    } conf;
    uint64_t idle; // Time of last send/receive on this path.
    unsigned char ok;
}
```

A control message is represented using this structure.
Values are stored in LSB(!).

```
struct mud_msg {
    unsigned char sent_time[MUD_TIME_SIZE]; // Time in us. Least significant bit is the msg flag.
    unsigned char state; // State field of sending path.
    unsigned char aes; // Boolean flag. Use AES if true, otherwise use chacha.
    unsigned char pkey[MUD_PUBKEY_SIZE]; // Public key.
    struct {
        unsigned char bytes[sizeof(uint64_t)];
        unsigned char total[sizeof(uint64_t)];
    } tx, rx, fw;
    unsigned char max_rate[sizeof(uint64_t)];
    unsigned char beat[MUD_TIME_SIZE];
    unsigned char mtu[2]; // Last valid MTU of path if path is not probing MTUs.
    unsigned char loss;
    unsigned char fixed_rate;
    unsigned char loss_limit;
    struct mud_addr addr; // Address field of sending path.
}
```

A MUD interface is represented using this structure.

```
struct mud {
    int fd; // File descriptor of the listening socket.
    int backup;
    uint64_t keepalive;
    uint64_t time_tolerance;
    struct sockaddr_storage addr; // Listening address of MUD instance.
    struct mud_path *paths;
    unsigned count;
    struct mud_keyx keyx;
    uint64_t last_recv_time;
    size_t mtu;
    int tc;
    struct {
        int set; // True if this instance is a client.
        struct sockaddr_storage addr; // Address of server.
    } peer;
    struct mud_bad bad;
    uint64_t rate;
    uint64_t window; // Current size of window in bytes. Updated every time the window is < the MTU. Limited to rate * 20ms.
    uint64_t window_time; // Last time window was updated.
    uint64_t base_time;
};

```


## Flow

```

p = packet
c_c = control message call - sent_time = 0
c_r = control message response - sent_time = c_c send time


Server     Client

# Glorytun server started. GT calls mud_create with bind address.
# MUD creates UDP socket listening to bind address.
# Glorytun gets the socket.
# Glorytun sets MUD private key using keyfile.
# MUD uses private key as current, next, and last keyx.
# Glorytun creates tun interface.
# Glorytun creates a unix socket for control messages named after tun interface.
# Glorytun enters a loop of calling mud_update and processing packets from the different interface using select.

# Glorytun client is started. The client repeats the process above.
# After creating the tun device, the client registers the server as a peer.
# N devices registered on client.

       <-- c_c

# Server receives control messages and creates/updates the path used.

c_r    --> 

# Client determines rtt + other stats.

       <--  c_r

# Server determines rtt + other stats.
```

## TODO

- Determine exactly what to use in this protocol to control what interfaces to use.


## Changes

- Added two new states for paths. LOSSY and LAGGY
  - An path is LAGGY if it's rtt is greater than the rtt limit.
  - An path is LOSSY if it's % lost is greater than the loss limit. Being LOSSY overwrites being LAGGY.
  - An path is OK if it's neither LOSSY or LAGGY.
- If there is one or more OK path, then choose one at random.
- If there are no OK paths, then choose the LAGGY path with the lowest rtt.
- If there are no LAGGY paths, then choose the preferred LOSSY path. This is the worse case scenario.