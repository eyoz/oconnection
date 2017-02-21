#include "e_mod_main.h"
#include "oconnection.h"
#include "oconnection_network_manager.h"

#define OCONNECTION_MIN_SCAN_TIME 0.5
#define OCONNECTION_MAX_SCAN_TIME 1800.0

static void _oconnection_event_iface_add_free(void *data, void *ev);
static Eina_Bool _oconnection_iface_update_timer(void *data);
static void _oconnection_event_iface_update_free(void *data, void *ev);
static Eina_Bool _oconnection_scan_timer(void *data);
static void _oconnection_wifi_scan(void);
static void _oconnection_event_scan_results_free(void *data, void *ev);


static Oconnection_Status *_ostatus = NULL;

void
oconnection_init(void)
{
    _ostatus = calloc(1, sizeof(Oconnection_Status));
    if (!_ostatus) return;

   OCONNECTION_EVENT_SCAN_RESULTS = ecore_event_type_new();
   OCONNECTION_EVENT_IFACE_UPDATE = ecore_event_type_new();
   OCONNECTION_EVENT_IFACE_WIRED_ADD = ecore_event_type_new();
   OCONNECTION_EVENT_IFACE_WIRED_DEL = ecore_event_type_new();
   OCONNECTION_EVENT_IFACE_WIFI_ADD = ecore_event_type_new();
   OCONNECTION_EVENT_IFACE_WIFI_DEL = ecore_event_type_new();


   oconnection_nm_init();
}

void
oconnection_shutdown(void)
{
   OWireless_Network *ow;
   OIface_Status *os;

   oconnection_nm_shutdown();
   if (_ostatus->scan_timer)
     ecore_timer_del(_ostatus->scan_timer);
   EINA_LIST_FREE(_ostatus->scan.networks, ow)
     {
        eina_stringshare_del(ow->dbus_path);
        eina_stringshare_del(ow->ssid);
        eina_stringshare_del(ow->bssid);
        free(ow);
     }
   EINA_LIST_FREE(_ostatus->wired_status, os)
     {
        eina_stringshare_del(os->dbus_path);
        eina_stringshare_del(os->name);
        eina_stringshare_del(os->addr);
        eina_stringshare_del(os->ssid);
        eina_stringshare_del(os->connection_path);
        free(os);
     }
   EINA_LIST_FREE(_ostatus->wifi_status, os)
     {
        eina_stringshare_del(os->dbus_path);
        eina_stringshare_del(os->name);
        eina_stringshare_del(os->addr);
        eina_stringshare_del(os->ssid);
        eina_stringshare_del(os->connection_path);
        free(os);
     }
   eina_list_free(_ostatus->ifaces_change);
   free(_ostatus);
}

void
oconnection_iface_wired_add(OIface_Status *os)
{
   Oconnection_Event_Iface_Wired_Add *ev;

   _ostatus->wired_status = eina_list_append(_ostatus->wired_status, os);
   ev = E_NEW(Oconnection_Event_Iface_Wired_Add, 1);
   ev->iface = os->name;
   ecore_event_add(OCONNECTION_EVENT_IFACE_WIRED_ADD, ev,
                   _oconnection_event_iface_add_free, NULL);
}

void
oconnection_iface_wifi_add(OIface_Status *os)
{
   Oconnection_Event_Iface_Wifi_Add *ev;

   _ostatus->wifi_status = eina_list_append(_ostatus->wifi_status, os);
   ev = E_NEW(Oconnection_Event_Iface_Wired_Add, 1);
   ev->iface = os->name;
   ecore_event_add(OCONNECTION_EVENT_IFACE_WIRED_ADD, ev,
                   _oconnection_event_iface_add_free, NULL);
   _oconnection_wifi_scan();
}

void
oconnection_iface_update(OIface_Status *os)
{
   Eina_List *l;
   OIface_Status *fos = NULL;

   EINA_LIST_FOREACH(_ostatus->ifaces_change, l, fos)
     {
        if (fos == os)
          break;
     }
   if (!fos)
     {
        _ostatus->ifaces_change = eina_list_append(_ostatus->ifaces_change, os);
        if (_ostatus->update_timer)
          ecore_timer_del(_ostatus->update_timer);
        ecore_timer_add(0.5, _oconnection_iface_update_timer, NULL);
     }
}

void
oconnection_iface_del(OIface_Status *os)
{
   Eina_List *l;
   OIface_Status *fos = NULL;

   EINA_LIST_FOREACH(_ostatus->wired_status, l, fos)
     {
        if (fos == os)
          {
             _ostatus->wired_status =
                eina_list_remove_list(_ostatus->wired_status, l);
             break;
          }
     }
   if (!l)
     {
        EINA_LIST_FOREACH(_ostatus->wifi_status, l, fos)
          {
             if (fos == os)
               {
                  _ostatus->wifi_status =
                     eina_list_remove_list(_ostatus->wifi_status, l);
                  break;
               }
          }
     }
}



void
oconnection_scan_add(OWireless_Network *ow)
{
   _ostatus->scan.networks = eina_list_append(_ostatus->scan.networks, ow);
}

Eina_List *
oconnection_scan_list_get(void)
{
   return _ostatus->scan.networks;
}

void
oconnection_scan_list_set(Eina_List *l)
{
   _ostatus->scan.networks = l;
}

void
oconnection_wireless_network_free(OWireless_Network *ow)
{
   eina_stringshare_del(ow->dbus_path);
   eina_stringshare_del(ow->ssid);
   eina_stringshare_del(ow->bssid);
   free(ow);
}

void
oconnection_scan_done(void)
{
   double t;
   Oconnection_Event_Scan_Results *ev;
   OWireless_Network *ow;
   Eina_List *l, *ll;

   EINA_LIST_FOREACH_SAFE(_ostatus->scan.networks, l, ll, ow)
     {
        if (!ow->found)
          {
             _ostatus->scan.networks =
                eina_list_remove_list(_ostatus->scan.networks, l);
             oconnection_wireless_network_free(ow);
          }
     }
   ev = E_NEW(Oconnection_Event_Scan_Results, 1);
   ev->networks = _ostatus->scan.networks;
   ecore_event_add(OCONNECTION_EVENT_SCAN_RESULTS, ev,
                   _oconnection_event_scan_results_free, NULL);

   t = ecore_loop_time_get() - _ostatus->scan_start;
   if (t > OCONNECTION_MAX_SCAN_TIME) t = OCONNECTION_MAX_SCAN_TIME;
   t = t / OCONNECTION_MAX_SCAN_TIME;
   t = 1.0 - t;
   t = t * t * t * t;
   t = OCONNECTION_MAX_SCAN_TIME * (1.0 - t);
   if (t < OCONNECTION_MIN_SCAN_TIME) t = OCONNECTION_MIN_SCAN_TIME;

   if (_ostatus->scan_timer) ecore_timer_del(_ostatus->scan_timer);
   _ostatus->scan_timer = ecore_timer_add(t, _oconnection_scan_timer, NULL);
   ODBG("new scan in %f\n", t);
}


static void
_oconnection_event_scan_results_free(void *data, void *ev)
{
   free(ev);
}


void
oconnection_wifi_connect(OWireless_Network *ow, const char *ssid, const char *psk, Oconnection_Network_Flags key_type, Oconnection_Connect_Cb associated_cb, Oconnection_Connect_Cb connect_cb, void *data)
{
   _ostatus->connect.want_connect = EINA_TRUE;
   _ostatus->connect.func.associated_cb = associated_cb;
   _ostatus->connect.func.connect_cb = connect_cb;
   _ostatus->connect.func.data = data;
   if (ow)
     {
        printf("try to connect to %s with %s\n", ow->ssid, psk);
        oconnection_nm_connect(ow, psk);
     }
   else
     {
        printf("try to connect to hidden %s with %s\n", ssid, psk);
        oconnection_nm_connect_hidden(ssid, psk, key_type);
     }
}

void
oconnection_wifi_connect_connected(Eina_Bool connected)
{
   if (_ostatus->connect.func.connect_cb)
     _ostatus->connect.func.connect_cb(_ostatus->connect.func.data,
                                       connected);
}

void
oconnection_wifi_connect_associated(Eina_Bool associated)
{
   if (_ostatus->connect.func.associated_cb)
     _ostatus->connect.func.associated_cb(_ostatus->connect.func.data,
                                          associated);
}

Eina_Bool
oconnection_is_in_spool_get(const char *key)
{
   return oconnection_nm_is_in_spool_get(key);
}

void
oconnection_wifi_connect_cancel_callback()
{
   if (_ostatus->connect.timer)
     {
        ecore_timer_del(_ostatus->connect.timer);
        _ostatus->connect.timer = NULL;
     }
   _ostatus->connect.func.associated_cb = NULL;
   _ostatus->connect.func.connect_cb = NULL;
   _ostatus->connect.func.data = NULL;
}



static void
_oconnection_event_iface_add_free(void *data, void *ev)
{
   free(ev);
}


static Eina_Bool
_oconnection_iface_update_timer(void *data)
{
   Oconnection_Event_Iface_Update *ev;

   _ostatus->update_timer = NULL;
   ev = E_NEW(Oconnection_Event_Iface_Update, 1);
   ev->ifaces = _ostatus->ifaces_change;

   ecore_event_add(OCONNECTION_EVENT_IFACE_UPDATE, ev,
                   _oconnection_event_iface_update_free, NULL);
   return ECORE_CALLBACK_CANCEL;
}

static void
_oconnection_event_iface_update_free(void *data, void *ev)
{
      _ostatus->ifaces_change = eina_list_free(_ostatus->ifaces_change);
         free(ev);
}


static Eina_Bool
_oconnection_scan_timer(void *data)
{
   ODBG("Scanning\n");
   _ostatus->scan_timer = NULL;
   oconnection_nm_scan();
   return ECORE_CALLBACK_CANCEL;
}

static void
_oconnection_wifi_scan(void)
{
   if (_ostatus->scan_timer)
     {
        ecore_timer_del(_ostatus->scan_timer);
        _ostatus->scan_timer = NULL;
     }
   _ostatus->scan_start = ecore_loop_time_get();
   _oconnection_scan_timer(NULL);
}

Eina_List *
oconnection_iface_wired_get(void)
{
   Eina_List *ret = NULL;
   if (_ostatus)
     ret = _ostatus->wired_status;
   return ret;
}

Eina_List *
oconnection_iface_wifi_get(void)
{
   Eina_List *ret = NULL;
   if (_ostatus)
     ret = _ostatus->wifi_status;
   return ret;
}

Eina_List *
oconnection_scan_get(void)
{
   _oconnection_wifi_scan();
   return _ostatus->scan.networks;
}

Eina_Bool
oconnection_scan_hidden_wpa(void)
{
   if (_ostatus)
     return _ostatus->scan.have_hidden_wpa;
   return EINA_FALSE;
}

Eina_Bool
oconnection_scan_hidden_wep(void)
{
   if (_ostatus)
     return _ostatus->scan.have_hidden_wep;
   return EINA_FALSE;
}

Eina_Bool
oconnection_scan_hidden_open(void)
{
   if (_ostatus)
     return _ostatus->scan.have_hidden_open;
   return EINA_FALSE;
}

void
oconnection_scan_hidden_set(Eina_Bool wpa, Eina_Bool wep, Eina_Bool open)
{
   if (_ostatus)
     {
        _ostatus->scan.have_hidden_wpa = wpa;
        _ostatus->scan.have_hidden_wep = wep;
        _ostatus->scan.have_hidden_open = open;
     }
}

