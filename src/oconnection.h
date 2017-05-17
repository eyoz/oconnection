#ifndef OCONNECTION_H_
#define OCONNECTION_H_

typedef enum Oconnection_Network_Flags_
{
   NW_FLAG_NONE  = 0,
   NW_FLAG_WEP   = 1 << 1,
   NW_FLAG_WPA   = 1 << 2,
   NW_FLAG_PRIVACY = 1 << 3,
} Oconnection_Network_Flags;

EINTERN int OCONNECTION_EVENT_SCAN_RESULTS;
EINTERN int OCONNECTION_EVENT_IFACE_UPDATE;
EINTERN int OCONNECTION_EVENT_IFACE_WIRED_ADD;
EINTERN int OCONNECTION_EVENT_IFACE_WIRED_DEL;
EINTERN int OCONNECTION_EVENT_IFACE_WIFI_ADD;
EINTERN int OCONNECTION_EVENT_IFACE_WIFI_DEL;

typedef void (*Oconnection_Connect_Cb) (void *data, Eina_Bool result);
typedef void (*Iface_Cb) (const char *iface);

typedef struct OWireless_Network_
{
   const char *dbus_path;
   const char *ssid;
   const char *bssid;
   int signal;
   int freq;
   Oconnection_Network_Flags flags;
   Eina_Bool found:1;
} OWireless_Network;

typedef struct OIface_Status_
{
   const char *dbus_path;
   const char *name;
   const char *addr;
   const char *ssid;
   const char *connection_path;
   void *data;
   OWireless_Network *ow;
   int qual;
   Eina_Bool connected:1;
   Eina_Bool is_wifi:1;
   Eina_Bool enabled:1;
   Eina_Bool supported:1;
   Eina_Bool in_list:1;
} OIface_Status;

typedef struct Oconnection_Status_
{
   struct {
        Eina_List *networks;
        Eina_Bool have_hidden_wpa:1;
        Eina_Bool have_hidden_wep:1;
        Eina_Bool have_hidden_open:1;
   } scan;
   Ecore_Timer *scan_timer;
   Ecore_Timer *update_timer;
   double scan_start;
   Eina_List *wired_status;
   Eina_List *wifi_status;
   Eina_List *ifaces_change;
   Eina_Hash *spool;
   struct {
        Eina_Bool want_connect:1;
        struct {
             Oconnection_Connect_Cb associated_cb;
             Oconnection_Connect_Cb connect_cb;
             void *data;
        } func;
        Ecore_Timer *timer;
   } connect;
} Oconnection_Status;


typedef struct _Oconnection_Event_Scan_Results
{
   Eina_List *networks;
} Oconnection_Event_Scan_Results;

typedef struct _Oconnection_Event_Iface_Update
{
   Eina_List *ifaces;
} Oconnection_Event_Iface_Update;


struct _Oconnection_Event_Iface
{
   const char *iface;
};

typedef struct _Oconnection_Event_Iface Oconnection_Event_Iface;
typedef struct _Oconnection_Event_Iface Oconnection_Event_Iface_Wired_Add;
typedef struct _Oconnection_Event_Iface Oconnection_Event_Iface_Wired_Del;
typedef struct _Oconnection_Event_Iface Oconnection_Event_Iface_Wifi_Add;
typedef struct _Oconnection_Event_Iface Oconnection_Event_Iface_Wifi_Del;



void oconnection_init(void);
void oconnection_shutdown(void);
void oconnection_wifi_connect(OWireless_Network *ow, const char *ssid, const char *psk, Oconnection_Network_Flags key_type, Oconnection_Connect_Cb dhcp_cb, Oconnection_Connect_Cb connect_cb, void *data);
void oconnection_wifi_connect_cancel_callback(void);


/* not sure to keep this */
Eina_List *oconnection_scan_get(void);
void oconnection_iface_wired_add(OIface_Status *os);
void oconnection_iface_wifi_add(OIface_Status *os);
void oconnection_iface_update(OIface_Status *os);
void oconnection_iface_del(OIface_Status *os);

void oconnection_wifi_connect_connected(Eina_Bool connected);
void oconnection_wifi_connect_associated(Eina_Bool associated);

Eina_Bool oconnection_is_in_spool_get(const char *key);


void oconnection_scan_done(void);
void oconnection_scan_add(OWireless_Network *ow);
Eina_List *oconnection_scan_list_get(void);
void oconnection_scan_list_set(Eina_List *l);
void oconnection_wireless_network_free(OWireless_Network *ow);

Eina_List *oconnection_iface_wired_get(void);
Eina_List *oconnection_iface_wifi_get(void);
Eina_Bool oconnection_scan_hidden_wpa(void);
Eina_Bool oconnection_scan_hidden_wep(void);
Eina_Bool oconnection_scan_hidden_open(void);

void oconnection_scan_hidden_set(Eina_Bool wpa, Eina_Bool wep, Eina_Bool open);

#endif /* OCONNECTION_H_ */

