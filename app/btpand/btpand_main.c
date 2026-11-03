#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <pthread.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netutils/netlib.h>
#include <netutils/dhcpc.h>
#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_device.h"
#include "bt_pan.h"

#define TAG "btpand"
#define RECONNECT_SEC 5
#define SDP_SETTLE_SEC 8

static bt_instance_t *g_ins = NULL;
static bt_address_t g_peer = {{0}};
static volatile int g_pan_connected = 0;
static volatile int g_bonded = 0;
static volatile int g_running = 1;

/* ===== DHCP over BNEP（参考 22 号文第四节）===== */
static int configure_dhcp(void)
{
    void *handle;
    struct dhcpc_state result;
    uint8_t mac[6];
    int ret;

    netlib_getmacaddr("bt-pan", mac);
    syslog(LOG_INFO, "[%s] DHCP: bt-pan MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
           TAG, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    netlib_ifup("bt-pan");
    syslog(LOG_INFO, "[%s] DHCP: ifup BEFORE dhcpc\n", TAG);
    handle = dhcpc_open("bt-pan", mac, 6);
    if (!handle) {
        syslog(LOG_ERR, "[%s] DHCP: dhcpc_open FAILED\n", TAG);
        return -1;
    }

    ret = dhcpc_request(handle, &result);
    dhcpc_close(handle);
    if (ret < 0) {
        syslog(LOG_ERR, "[%s] DHCP: request FAILED ret=%d errno=%d\n", TAG, ret, errno);
        return -1;
    }

    netlib_set_ipv4addr("bt-pan", &result.ipaddr);
    netlib_set_ipv4netmask("bt-pan", &result.netmask);
    netlib_set_dripv4addr("bt-pan", &result.default_router);
    netlib_set_ipv4dnsaddr(&result.dnsaddr);
    netlib_ifup("bt-pan");

    syslog(LOG_INFO, "[%s] DHCP OK: ip=%s gw=%s dns=%s\n", TAG,
           inet_ntoa(result.ipaddr), inet_ntoa(result.default_router),
           inet_ntoa(result.dnsaddr));
    return 0;
}

static void *pan_thread(void *arg)
{
    (void)arg;
    bt_address_t zero = {{0}};
    int settle = 0;
    int dhcp_done = 0;

    while (g_running) {
        if (!g_bonded || memcmp(&g_peer, &zero, sizeof(zero)) == 0) {
            sleep(1);
            continue;
        }
        if (g_pan_connected) {
            settle = 0;
            if (!dhcp_done) {
                usleep(500000); /* 等 PAN 稳定 */
                syslog(LOG_INFO, "[%s] PAN connected, starting DHCP...\n", TAG);
                if (configure_dhcp() == 0) {
                    dhcp_done = 1;
                    syslog(LOG_INFO, "[%s] ===== NETWORK READY =====\n", TAG);
                }
            }
            sleep(RECONNECT_SEC);
            continue;
        }
        dhcp_done = 0;
        if (settle < SDP_SETTLE_SEC) {
            settle++;
            sleep(1);
            continue;
        }
        syslog(LOG_INFO, "[%s] auto-PAN connecting...\n", TAG);
        bt_status_t st = bt_pan_connect(g_ins, &g_peer,
                                         PAN_ROLE_NAP, PAN_ROLE_PANU);
        syslog(LOG_INFO, "[%s] bt_pan_connect ret=%d\n", TAG, st);
        sleep(RECONNECT_SEC);
    }
    return NULL;
}

static void pan_state_cb(void *cookie, profile_connection_state_t state,
                          bt_address_t *addr, uint8_t local_role, uint8_t remote_role)
{
    syslog(LOG_INFO, "[%s] PAN state=%d lr=%d rr=%d\n", TAG, state, local_role, remote_role);
    g_pan_connected = (state == PROFILE_STATE_CONNECTED);
    if (g_pan_connected) {
        syslog(LOG_INFO, "[%s] ===== PAN CONNECTED =====\n", TAG);
    }
}

static void conn_state_cb(void *cookie, bt_address_t *addr,
                           bt_transport_t transport, connection_state_t state)
{
    syslog(LOG_INFO, "[%s] conn state=%d transport=%d\n", TAG, state, transport);
    if (state == CONNECTION_STATE_CONNECTED) {
        memcpy(&g_peer, addr, sizeof(bt_address_t));
        g_bonded = 1;
        syslog(LOG_INFO, "[%s] peer remembered, will auto-PAN\n", TAG);
    }
}

static void bond_state_cb(void *cookie, bt_address_t *addr,
                           bt_transport_t transport, bond_state_t state,
                           bool is_ctkd)
{
    syslog(LOG_INFO, "[%s] bond state=%d\n", TAG, state);
    if (state == BOND_STATE_BONDING) {
        bt_device_set_pairing_confirmation(g_ins, addr, transport, true);
    }
    if (state == BOND_STATE_BONDED) {
        memcpy(&g_peer, addr, sizeof(bt_address_t));
        g_bonded = 1;
    }
}

static adapter_callbacks_t g_adapter_cbs;
static pan_callbacks_t g_pan_cbs;

static void sig_handler(int sig) { (void)sig; g_running = 0; }

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    syslog(LOG_INFO, "[%s] starting\n", TAG);
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    g_ins = bluetooth_create_instance();
    if (!g_ins) {
        syslog(LOG_ERR, "[%s] create_instance FAILED\n", TAG);
        return 1;
    }

    memset(&g_adapter_cbs, 0, sizeof(g_adapter_cbs));
    g_adapter_cbs.on_connection_state_changed = conn_state_cb;
    g_adapter_cbs.on_bond_state_changed = bond_state_cb;

    memset(&g_pan_cbs, 0, sizeof(g_pan_cbs));
    g_pan_cbs.size = sizeof(g_pan_cbs);
    g_pan_cbs.connection_state_cb = pan_state_cb;

    bt_adapter_register_callback(g_ins, &g_adapter_cbs);
    bt_pan_register_callbacks(g_ins, &g_pan_cbs);

    bt_status_t st = bt_adapter_enable(g_ins);
    syslog(LOG_INFO, "[%s] enable ret=%d\n", TAG, st);
    sleep(3);

    st = bt_adapter_set_scan_mode(g_ins,
        BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE, true);
    syslog(LOG_INFO, "[%s] scanmode ret=%d\n", TAG, st);

    pthread_t tid;
    pthread_create(&tid, NULL, pan_thread, NULL);

    while (g_running) sleep(1);
    syslog(LOG_INFO, "[%s] stopped\n", TAG);
    return 0;
}
