/*
 * Kobo Color BLE Peripheral - Uses MediaTek BT middleware via dlopen
 *
 * Powers on BT, registers a GATT server, adds a simple service,
 * enables BLE advertising so Apple Watch can discover and connect.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/time.h>
#include <linux/input.h>

/* Type definitions matching MediaTek BT MW */
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned int   UINT32;
typedef char           CHAR;
typedef int            INT32;
typedef void           VOID;

#define BT_GATT_MAX_UUID_LEN  37
#define BT_GATT_MAX_ATTR_LEN  600
#define MAX_BDADDR_LEN        18

/* Callback event types */
typedef enum {
    BT_GATTS_REGISTER_SERVER = 0,
    BT_GATTS_CONNECT,
    BT_GATTS_DISCONNECT,
    BT_GATTS_GET_RSSI_DONE,
    BT_GATTS_EVENT_MAX
} BT_GATTS_EVENT_T;

typedef enum {
    BT_GATTS_START_SRVC = 0,
    BT_GATTS_STOP_SRVC,
    BT_GATTS_DEL_SRVC,
    BT_GATTS_SRVC_OP_MAX
} BT_GATTS_SRVC_OP_TYPE_T;

/* Callback result structures */
typedef struct {
    CHAR uuid[BT_GATT_MAX_UUID_LEN];
    UINT8 inst_id;
} BT_GATT_ID_T;

typedef struct {
    BT_GATT_ID_T id;
    UINT8 is_primary;
} BT_GATTS_SRVC_ID_T;

typedef struct {
    INT32 server_if;
    CHAR  app_uuid[BT_GATT_MAX_UUID_LEN];
} BT_GATTS_REG_SERVER_RST_T;

typedef struct {
    INT32 conn_id;
    INT32 server_if;
    CHAR  btaddr[MAX_BDADDR_LEN];
} BT_GATTS_CONNECT_RST_T;

typedef struct {
    INT32 server_if;
    BT_GATTS_SRVC_ID_T srvc_id;
    INT32 srvc_handle;
} BT_GATTS_ADD_SRVC_RST_T;

typedef struct {
    INT32 server_if;
    INT32 srvc_handle;
    INT32 incl_srvc_handle;
} BT_GATTS_ADD_INCL_RST_T;

typedef struct {
    INT32 server_if;
    CHAR  uuid[BT_GATT_MAX_UUID_LEN];
    INT32 srvc_handle;
    INT32 char_handle;
} BT_GATTS_ADD_CHAR_RST_T;

typedef struct {
    INT32 server_if;
    CHAR  uuid[BT_GATT_MAX_UUID_LEN];
    INT32 srvc_handle;
    INT32 descr_handle;
} BT_GATTS_ADD_DESCR_RST_T;

typedef struct {
    INT32 server_if;
    INT32 srvc_handle;
} BT_GATTS_SRVC_RST_T;

typedef struct {
    INT32 conn_id;
    INT32 trans_id;
    CHAR  btaddr[MAX_BDADDR_LEN];
    INT32 attr_handle;
    INT32 offset;
    UINT8 is_long;
} BT_GATTS_REQ_READ_RST_T;

typedef struct {
    INT32 conn_id;
    INT32 trans_id;
    CHAR  btaddr[MAX_BDADDR_LEN];
    INT32 attr_handle;
    INT32 offset;
    INT32 length;
    UINT8 need_rsp;
    UINT8 is_prep;
    UINT8 value[BT_GATT_MAX_ATTR_LEN];
} BT_GATTS_REQ_WRITE_RST_T;

/* Callback function pointer types */
typedef VOID (*BtAppGATTSEventCbk)(BT_GATTS_EVENT_T event, void* pv_tag);
typedef VOID (*BtAppGATTSRegServerCbk)(BT_GATTS_REG_SERVER_RST_T *rst, void* pv_tag);
typedef VOID (*BtAppGATTSAddSrvcCbk)(BT_GATTS_ADD_SRVC_RST_T *rst, void* pv_tag);
typedef VOID (*BtAppGATTSAddInclCbk)(BT_GATTS_ADD_INCL_RST_T *rst, void* pv_tag);
typedef VOID (*BtAppGATTSAddCharCbk)(BT_GATTS_ADD_CHAR_RST_T *rst, void* pv_tag);
typedef VOID (*BtAppGATTSAddDescCbk)(BT_GATTS_ADD_DESCR_RST_T *rst, void* pv_tag);
typedef VOID (*BtAppGATTSOpSrvcCbk)(BT_GATTS_SRVC_OP_TYPE_T op, BT_GATTS_SRVC_RST_T *rst, void* pv_tag);
typedef VOID (*BtAppGATTSReqReadCbk)(BT_GATTS_REQ_READ_RST_T *rst, void* pv_tag);
typedef VOID (*BtAppGATTSReqWriteCbk)(BT_GATTS_REQ_WRITE_RST_T *rst, void* pv_tag);
typedef VOID (*BtAppGATTSIndSentCbk)(INT32 conn_id, INT32 status, void* pv_tag);

typedef struct {
    BtAppGATTSEventCbk     bt_gatts_event_cb;
    BtAppGATTSRegServerCbk bt_gatts_reg_server_cb;
    BtAppGATTSAddSrvcCbk   bt_gatts_add_srvc_cb;
    BtAppGATTSAddInclCbk   bt_gatts_add_incl_cb;
    BtAppGATTSAddCharCbk   bt_gatts_add_char_cb;
    BtAppGATTSAddDescCbk   bt_gatts_add_desc_cb;
    BtAppGATTSOpSrvcCbk    bt_gatts_op_srvc_cb;
    BtAppGATTSReqReadCbk   bt_gatts_req_read_cb;
    BtAppGATTSReqWriteCbk  bt_gatts_req_write_cb;
    BtAppGATTSIndSentCbk   bt_gatts_ind_sent_cb;
} MTKRPCAPI_BT_APP_GATTS_CB_FUNC_T;

/* GATTC callback types (needed for advertising) */
typedef enum {
    BT_GATTC_REGISTER_APP = 0,
    BT_GATTC_SCAN_RESULT,
    BT_GATTC_CONNECT,
    BT_GATTC_DISCONNECT,
    BT_GATTC_EVENT_MAX
} BT_GATTC_EVENT_T;

typedef struct {
    INT32 client_if;
    CHAR  app_uuid[BT_GATT_MAX_UUID_LEN];
} BT_GATTC_REG_CLIENT_RST_T;

typedef VOID (*BtAppGATTCEventCbk)(BT_GATTC_EVENT_T event, void* pv_tag);
typedef VOID (*BtAppGATTCRegClientCbk)(BT_GATTC_REG_CLIENT_RST_T *rst, void* pv_tag);
typedef VOID (*BtAppGATTCScanCbk)(void *rst, void* pv_tag);
typedef VOID (*BtAppGATTCGetGattDbCbk)(void *rst, void* pv_tag);
typedef VOID (*BtAppGATTCGetRegNotiCbk)(void *rst, void* pv_tag);
typedef VOID (*BtAppGATTCNotifyCbk)(void *rst, void* pv_tag);
typedef VOID (*BtAppGATTCReadCharCbk)(void *rst, void* pv_tag);
typedef VOID (*BtAppGATTCWriteCharCbk)(void *rst, void* pv_tag);
typedef VOID (*BtAppGATTCScanFilterCbk)(void *rst, void* pv_tag);
typedef VOID (*BtAppGATTCAdvCbk)(void *rst, void* pv_tag);

typedef struct {
    BtAppGATTCEventCbk       bt_gattc_event_cb;
    BtAppGATTCRegClientCbk   bt_gattc_reg_client_cb;
    BtAppGATTCScanCbk        bt_gattc_scan_cb;
    BtAppGATTCGetGattDbCbk   bt_gattc_get_gatt_db_cb;
    BtAppGATTCGetRegNotiCbk  bt_gattc_get_reg_noti_cb;
    BtAppGATTCNotifyCbk      bt_gattc_notify_cb;
    BtAppGATTCReadCharCbk    bt_gattc_read_char_cb;
    BtAppGATTCWriteCharCbk   bt_gattc_write_char_cb;
    BtAppGATTCScanFilterCbk  bt_gattc_scan_filt_cb;
    BtAppGATTCAdvCbk         bt_gattc_adv_cb;
} MTKRPCAPI_BT_APP_GATTC_CB_FUNC_T;

/* Function pointer types for dlsym */
typedef INT32 (*fn_service_init)(void);
typedef INT32 (*fn_rpc_init)(void);
typedef INT32 (*fn_rpc_start)(void);
typedef INT32 (*fn_gap_on_off)(INT32 on);
typedef INT32 (*fn_gatts_base_init)(MTKRPCAPI_BT_APP_GATTS_CB_FUNC_T *func, void* pv_tag);
typedef INT32 (*fn_gatts_register_server)(CHAR *app_uuid);
typedef INT32 (*fn_gatts_unregister_server)(INT32 server_if);
typedef INT32 (*fn_gatts_add_service)(INT32 server_if, CHAR *service_uuid, UINT8 is_primary, INT32 number);
typedef INT32 (*fn_gatts_add_char)(INT32 server_if, INT32 service_handle, CHAR *uuid, INT32 properties, INT32 permissions);
typedef INT32 (*fn_gatts_add_desc)(INT32 server_if, INT32 service_handle, CHAR *uuid, INT32 permissions);
typedef INT32 (*fn_gatts_start_service)(INT32 server_if, INT32 service_handle, INT32 transport);
typedef INT32 (*fn_gatts_stop_service)(INT32 server_if, INT32 service_handle);
typedef INT32 (*fn_gatts_send_response)(INT32 conn_id, INT32 trans_id, INT32 status, INT32 handle, CHAR *p_value, INT32 value_len, INT32 auth_req);
typedef INT32 (*fn_gatts_send_indication)(INT32 server_if, INT32 attr_handle, INT32 conn_id, INT32 fg_confirm, CHAR *p_value, INT32 value_len);
typedef INT32 (*fn_gattc_base_init)(MTKRPCAPI_BT_APP_GATTC_CB_FUNC_T *func, void* pv_tag);
typedef INT32 (*fn_gattc_register_app)(CHAR *app_uuid);
typedef INT32 (*fn_gattc_multi_adv_enable)(INT32 client_if, INT32 min_interval, INT32 max_interval, INT32 adv_type, INT32 chnl_map, INT32 tx_power, INT32 timeout_s);
typedef INT32 (*fn_gattc_multi_adv_setdata)(INT32 client_if, UINT8 set_scan_rsp, UINT8 include_name, UINT8 include_txpower, INT32 appearance, INT32 manufacturer_len, CHAR* manufacturer_data, INT32 service_data_len, CHAR* service_data, INT32 service_uuid_len, CHAR* service_uuid);
typedef INT32 (*fn_gattc_multi_adv_disable)(INT32 client_if);

/* Globals */
static volatile int g_running = 1;
static volatile INT32 g_server_if = -1;
static volatile INT32 g_client_if = -1;
static volatile INT32 g_srvc_handle = -1;
static volatile INT32 g_char_handle = -1;
static volatile INT32 g_conn_id = -1;

/* Page turn commands */
#define CMD_NEXT_PAGE  0x01
#define CMD_PREV_PAGE  0x02

/* Kobo physical page buttons on /dev/input/event0 (gpio-keys) */
#define KEY_F23  193   /* page back */
#define KEY_F24  194   /* page forward */
#define INPUT_DEV "/dev/input/event0"

static UINT8 g_last_cmd = 0;

/* Function pointers stored globally so callbacks can use them */
static fn_gatts_send_response g_send_response = NULL;

static void emit_input(int fd, unsigned short type, unsigned short code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

static void inject_page_turn(int forward) {
    int fd = open(INPUT_DEV, O_WRONLY);
    if (fd < 0) {
        printf("[PAGE] Failed to open %s\n", INPUT_DEV);
        return;
    }
    unsigned short key = forward ? KEY_F24 : KEY_F23;
    emit_input(fd, EV_KEY, key, 1);       /* key down */
    emit_input(fd, EV_SYN, SYN_REPORT, 0);
    usleep(50000);                          /* 50ms hold */
    emit_input(fd, EV_KEY, key, 0);        /* key up */
    emit_input(fd, EV_SYN, SYN_REPORT, 0);
    close(fd);
    printf("[PAGE] Injected %s page turn\n", forward ? "NEXT" : "PREV");
}

/* Our custom service UUID - "page turn" service for Apple Watch */
#define SERVICE_UUID    "12345678-1234-5678-1234-56789abcdef0"
#define CHAR_PAGE_UUID  "12345678-1234-5678-1234-56789abcdef1"
/* Client Characteristic Configuration Descriptor */
#define CCCD_UUID       "00002902-0000-1000-8000-00805f9b34fb"

/* BLE GATT properties */
#define GATT_PROP_READ       0x02
#define GATT_PROP_WRITE      0x08
#define GATT_PROP_NOTIFY     0x10
#define GATT_PROP_INDICATE   0x20

/* BLE GATT permissions */
#define GATT_PERM_READ       0x01
#define GATT_PERM_WRITE      0x10

/* Callbacks */
static void gatts_event_cb(BT_GATTS_EVENT_T event, void* pv_tag) {
    printf("[GATTS] Event: %d\n", event);
    if (event == BT_GATTS_CONNECT) {
        printf("[GATTS] *** Device connected! ***\n");
    } else if (event == BT_GATTS_DISCONNECT) {
        printf("[GATTS] *** Device disconnected ***\n");
        g_conn_id = -1;
    }
}

static void gatts_reg_server_cb(BT_GATTS_REG_SERVER_RST_T *rst, void* pv_tag) {
    printf("[GATTS] Server registered: server_if=%d uuid=%s\n", rst->server_if, rst->app_uuid);
    g_server_if = rst->server_if;
}

static void gatts_add_srvc_cb(BT_GATTS_ADD_SRVC_RST_T *rst, void* pv_tag) {
    printf("[GATTS] Service added: server_if=%d srvc_handle=%d\n", rst->server_if, rst->srvc_handle);
    g_srvc_handle = rst->srvc_handle;
}

static void gatts_add_incl_cb(BT_GATTS_ADD_INCL_RST_T *rst, void* pv_tag) {
    printf("[GATTS] Included service added\n");
}

static void gatts_add_char_cb(BT_GATTS_ADD_CHAR_RST_T *rst, void* pv_tag) {
    printf("[GATTS] Char added: uuid=%s char_handle=%d\n", rst->uuid, rst->char_handle);
    g_char_handle = rst->char_handle;
}

static void gatts_add_desc_cb(BT_GATTS_ADD_DESCR_RST_T *rst, void* pv_tag) {
    printf("[GATTS] Descriptor added: uuid=%s descr_handle=%d\n", rst->uuid, rst->descr_handle);
}

static void gatts_op_srvc_cb(BT_GATTS_SRVC_OP_TYPE_T op, BT_GATTS_SRVC_RST_T *rst, void* pv_tag) {
    const char *ops[] = {"START", "STOP", "DELETE"};
    printf("[GATTS] Service op: %s server_if=%d srvc_handle=%d\n",
           op < 3 ? ops[op] : "?", rst->server_if, rst->srvc_handle);
}

static void gatts_req_read_cb(BT_GATTS_REQ_READ_RST_T *rst, void* pv_tag) {
    printf("[GATTS] Read request: conn_id=%d trans_id=%d handle=%d from %s\n",
           rst->conn_id, rst->trans_id, rst->attr_handle, rst->btaddr);
    g_conn_id = rst->conn_id;

    if (g_send_response) {
        CHAR value[1] = { (CHAR)g_last_cmd };
        g_send_response(rst->conn_id, rst->trans_id, 0, rst->attr_handle,
                        value, 1, 0);
    }
}

static void gatts_req_write_cb(BT_GATTS_REQ_WRITE_RST_T *rst, void* pv_tag) {
    printf("[GATTS] Write request: conn_id=%d trans_id=%d handle=%d len=%d from %s\n",
           rst->conn_id, rst->trans_id, rst->attr_handle, rst->length, rst->btaddr);
    printf("[GATTS] Write data: ");
    for (int i = 0; i < rst->length && i < 32; i++)
        printf("%02x ", rst->value[i]);
    printf("\n");
    g_conn_id = rst->conn_id;

    /* Handle page turn commands */
    if (rst->length >= 1) {
        UINT8 cmd = rst->value[0];
        g_last_cmd = cmd;
        if (cmd == CMD_NEXT_PAGE) {
            inject_page_turn(1);
        } else if (cmd == CMD_PREV_PAGE) {
            inject_page_turn(0);
        }
    }

    /* Send write response if needed */
    if (rst->need_rsp && g_send_response) {
        g_send_response(rst->conn_id, rst->trans_id, 0, rst->attr_handle,
                        NULL, 0, 0);
    }
}

static void gatts_ind_sent_cb(INT32 conn_id, INT32 status, void* pv_tag) {
    printf("[GATTS] Indication sent: conn_id=%d status=%d\n", conn_id, status);
}

/* GATTC Callbacks */
static void gattc_event_cb(BT_GATTC_EVENT_T event, void* pv_tag) {
    printf("[GATTC] Event: %d\n", event);
    if (event == BT_GATTC_REGISTER_APP) {
        printf("[GATTC] App registered\n");
    }
}

static void gattc_reg_client_cb(BT_GATTC_REG_CLIENT_RST_T *rst, void* pv_tag) {
    printf("[GATTC] Client registered: client_if=%d uuid=%s\n", rst->client_if, rst->app_uuid);
    g_client_if = rst->client_if;
}

static void gattc_adv_cb(void *rst, void* pv_tag) {
    printf("[GATTC] Advertising callback\n");
}

static void sighandler(int sig) {
    printf("\nCaught signal %d, shutting down...\n", sig);
    g_running = 0;
}

#define LOAD_SYM(handle, type, name) \
    type name = (type)dlsym(handle, #name); \
    if (!name) { fprintf(stderr, "dlsym(%s): %s\n", #name, dlerror()); return 1; }

int main(int argc, char *argv[]) {
    INT32 ret;

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    printf("=== Kobo BLE Peripheral ===\n");

    /* Load the client library */
    void *lib = dlopen("libmtk_bt_service_client.so", RTLD_NOW);
    if (!lib) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        fprintf(stderr, "Try: LD_LIBRARY_PATH=/usr/lib %s\n", argv[0]);
        return 1;
    }
    printf("Loaded libmtk_bt_service_client.so\n");

    /* Load RPC init functions */
    LOAD_SYM(lib, fn_service_init, a_mtk_bt_service_init);
    LOAD_SYM(lib, fn_rpc_init, c_rpc_init_mtk_bt_service_client);
    LOAD_SYM(lib, fn_rpc_start, c_rpc_start_mtk_bt_service_client);

    /* Load all function pointers */
    LOAD_SYM(lib, fn_gap_on_off, a_mtkapi_bt_gap_on_off);
    LOAD_SYM(lib, fn_gattc_base_init, a_mtkapi_bt_gattc_base_init);
    LOAD_SYM(lib, fn_gatts_base_init, a_mtkapi_bt_gatts_base_init);
    LOAD_SYM(lib, fn_gatts_register_server, a_mtkapi_bt_gatts_register_server);
    LOAD_SYM(lib, fn_gatts_unregister_server, a_mtkapi_bt_gatts_unregister_server);
    LOAD_SYM(lib, fn_gatts_add_service, a_mtkapi_bt_gatts_add_service);
    LOAD_SYM(lib, fn_gatts_add_char, a_mtkapi_bt_gatts_add_char);
    LOAD_SYM(lib, fn_gatts_add_desc, a_mtkapi_bt_gatts_add_desc);
    LOAD_SYM(lib, fn_gatts_start_service, a_mtkapi_bt_gatts_start_service);
    LOAD_SYM(lib, fn_gatts_stop_service, a_mtkapi_bt_gatts_stop_service);
    LOAD_SYM(lib, fn_gatts_send_response, a_mtkapi_bt_gatts_send_response);
    LOAD_SYM(lib, fn_gatts_send_indication, a_mtkapi_bt_gatts_send_indication);
    LOAD_SYM(lib, fn_gattc_register_app, a_mtkapi_bt_gattc_register_app);
    LOAD_SYM(lib, fn_gattc_multi_adv_enable, a_mtkapi_bt_gattc_multi_adv_enable);
    LOAD_SYM(lib, fn_gattc_multi_adv_setdata, a_mtkapi_bt_gattc_multi_adv_setdata);
    LOAD_SYM(lib, fn_gattc_multi_adv_disable, a_mtkapi_bt_gattc_multi_adv_disable);

    printf("All symbols loaded\n");

    /* Store send_response globally so callbacks can use it */
    g_send_response = a_mtkapi_bt_gatts_send_response;

    /* Step 0: Initialize RPC connection to btservice */
    setbuf(stdout, NULL); /* Force unbuffered output */

    printf("Calling a_mtk_bt_service_init...\n");
    ret = a_mtk_bt_service_init();
    printf("a_mtk_bt_service_init: ret=%d\n", ret);
    sleep(3);

    /* Initialize RPC client explicitly (belt and suspenders) */
    printf("Initializing RPC client...\n");
    ret = c_rpc_init_mtk_bt_service_client();
    printf("c_rpc_init: ret=%d\n", ret);
    printf("Starting RPC client...\n");
    ret = c_rpc_start_mtk_bt_service_client();
    printf("c_rpc_start: ret=%d\n", ret);
    sleep(2);

    /* BT must be on via Kobo UI / mtkbtd before running this */

    /* Step 1: Initialize GATTC callbacks */
    MTKRPCAPI_BT_APP_GATTC_CB_FUNC_T gattc_cbs;
    memset(&gattc_cbs, 0, sizeof(gattc_cbs));
    gattc_cbs.bt_gattc_event_cb = gattc_event_cb;
    gattc_cbs.bt_gattc_reg_client_cb = gattc_reg_client_cb;
    gattc_cbs.bt_gattc_adv_cb = gattc_adv_cb;

    printf("Initializing GATTC callbacks...\n");
    ret = a_mtkapi_bt_gattc_base_init(&gattc_cbs, NULL);
    printf("gattc_base_init: ret=%d\n", ret);
    sleep(1);

    /* Step 2: Initialize GATTS callbacks */
    MTKRPCAPI_BT_APP_GATTS_CB_FUNC_T cbs = {
        .bt_gatts_event_cb      = gatts_event_cb,
        .bt_gatts_reg_server_cb = gatts_reg_server_cb,
        .bt_gatts_add_srvc_cb   = gatts_add_srvc_cb,
        .bt_gatts_add_incl_cb   = gatts_add_incl_cb,
        .bt_gatts_add_char_cb   = gatts_add_char_cb,
        .bt_gatts_add_desc_cb   = gatts_add_desc_cb,
        .bt_gatts_op_srvc_cb    = gatts_op_srvc_cb,
        .bt_gatts_req_read_cb   = gatts_req_read_cb,
        .bt_gatts_req_write_cb  = gatts_req_write_cb,
        .bt_gatts_ind_sent_cb   = gatts_ind_sent_cb,
    };

    printf("Initializing GATTS callbacks...\n");
    ret = a_mtkapi_bt_gatts_base_init(&cbs, NULL);
    printf("gatts_base_init: ret=%d\n", ret);
    sleep(1);

    /* Step 3: Register GATTC app for advertising */
    printf("Registering GATTC app...\n");
    ret = a_mtkapi_bt_gattc_register_app("12345678-1234-5678-1234-56789abcdef2");
    printf("gattc_register_app: ret=%d\n", ret);
    sleep(3);

    if (g_client_if < 0) {
        printf("WARNING: client_if not received from callback, using 0\n");
        g_client_if = 0;
    }
    printf("client_if = %d\n", g_client_if);

    /* Step 4: Register GATT server */
    printf("Registering GATT server...\n");
    ret = a_mtkapi_bt_gatts_register_server(SERVICE_UUID);
    printf("gatts_register_server: ret=%d\n", ret);
    sleep(3); /* Wait for callback */

    if (g_server_if < 0) {
        fprintf(stderr, "GATTS register failed (ret=%d). mtkbtd may be holding the interface.\n", ret);
        fprintf(stderr, "Continuing with advertising only...\n");
    }
    printf("server_if = %d\n", g_server_if);

    /* Step 5: Add a primary service (4 handles: service + char decl + char value + cccd) */
    if (g_server_if >= 0) {
        printf("Adding service...\n");
        ret = a_mtkapi_bt_gatts_add_service(g_server_if, SERVICE_UUID, 1, 4);
        printf("gatts_add_service: ret=%d\n", ret);
        sleep(1);
        printf("srvc_handle = %d\n", g_srvc_handle);

        /* Step 6: Add characteristic (read + write + notify) */
        if (g_srvc_handle >= 0) {
            printf("Adding characteristic...\n");
            ret = a_mtkapi_bt_gatts_add_char(g_server_if, g_srvc_handle, CHAR_PAGE_UUID,
                                              GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_NOTIFY,
                                              GATT_PERM_READ | GATT_PERM_WRITE);
            printf("gatts_add_char: ret=%d\n", ret);
            sleep(1);
            printf("char_handle = %d\n", g_char_handle);

            /* Add CCCD descriptor for notifications */
            printf("Adding CCCD descriptor...\n");
            ret = a_mtkapi_bt_gatts_add_desc(g_server_if, g_srvc_handle, CCCD_UUID,
                                              GATT_PERM_READ | GATT_PERM_WRITE);
            printf("gatts_add_desc: ret=%d\n", ret);
            sleep(1);

            /* Start the service (transport=2 for LE) */
            printf("Starting service...\n");
            ret = a_mtkapi_bt_gatts_start_service(g_server_if, g_srvc_handle, 2);
            printf("gatts_start_service: ret=%d\n", ret);
            sleep(1);
        }
    }

    /* Step 9: Enable BLE advertising
     * ADV_IND (type 0) = connectable undirected advertising
     * interval 160 = 100ms (units of 0.625ms)
     * channel map 7 = all channels (37, 38, 39)
     * Try multiple client_if values if the callback one fails */
    {
        INT32 adv_client = g_client_if;
        int adv_ok = 0;
        for (int try = 0; try < 8 && !adv_ok; try++) {
            printf("Enabling BLE advertising (client_if=%d)...\n", adv_client);
            ret = a_mtkapi_bt_gattc_multi_adv_enable(adv_client,
                                                      160,  /* min interval (100ms) */
                                                      320,  /* max interval (200ms) */
                                                      0,    /* ADV_IND */
                                                      7,    /* all channels */
                                                      1,    /* tx power */
                                                      0);   /* no timeout */
            printf("multi_adv_enable: ret=%d\n", ret);
            if (ret == 0) {
                adv_ok = 1;
                g_client_if = adv_client;
            } else {
                adv_client++;
                sleep(1);
            }
        }
        sleep(1);

        /* Step 10: Set advertising data */
        printf("Setting advertising data...\n");
        ret = a_mtkapi_bt_gattc_multi_adv_setdata(g_client_if,
                                                   0,    /* not scan response */
                                                   1,    /* include name */
                                                   0,    /* don't include tx power */
                                                   0,    /* appearance */
                                                   0, NULL, /* no manufacturer data */
                                                   0, NULL, /* no service data */
                                                   strlen(SERVICE_UUID), SERVICE_UUID);
        printf("multi_adv_setdata: ret=%d\n", ret);
    }

    printf("\n=== BLE Peripheral running ===\n");
    printf("Service UUID: %s\n", SERVICE_UUID);
    printf("Char UUID:    %s\n", CHAR_PAGE_UUID);
    printf("Press Ctrl+C to stop\n\n");

    /* Main loop - just wait for events */
    while (g_running) {
        sleep(1);
    }

    /* Cleanup */
    printf("Stopping advertising...\n");
    a_mtkapi_bt_gattc_multi_adv_disable(g_client_if);

    printf("Stopping service...\n");
    a_mtkapi_bt_gatts_stop_service(g_server_if, g_srvc_handle);

    printf("Unregistering server...\n");
    a_mtkapi_bt_gatts_unregister_server(g_server_if);

    dlclose(lib);
    printf("Done.\n");
    return 0;
}
