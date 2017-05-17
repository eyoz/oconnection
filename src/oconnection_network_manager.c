#include "e_mod_main.h"
#include "NetworkManager.h"
#include "oconnection.h"
#include "oconnection_network_manager.h"
#include <uuid/uuid.h>
#include <arpa/inet.h>
#include <Eldbus.h>

#define OCONNECTION_CALL(SERVICE, PATH, IFACE, NAME, func, data, x ...) \
   do { \
        Eldbus_Object *_oconn_obj; \
        Eldbus_Proxy *_oconn_proxy; \
        \
        _oconn_obj = eldbus_object_get(_nm_dbus, SERVICE, PATH); \
        if (!_oconn_obj) break; \
        _oconn_proxy = eldbus_proxy_get(_oconn_obj, IFACE); \
        if (!_oconn_proxy) \
          { \
             eldbus_object_unref(_oconn_obj); \
             break; \
          } \
        eldbus_proxy_call(_oconn_proxy, NAME, func, \
                          data, -1, ##x); \
   } while (0)

#define OCONNECTION_GET(SERVICE, PATH, IFACE, NAME, func, data) \
   do { \
        Eldbus_Object *_oconn_obj; \
        Eldbus_Proxy *_oconn_proxy; \
        \
        _oconn_obj = eldbus_object_get(_nm_dbus, SERVICE, PATH); \
        if (!_oconn_obj) break; \
        _oconn_proxy = eldbus_proxy_get(_oconn_obj, IFACE); \
        if (!_oconn_proxy) \
          { \
             eldbus_object_unref(_oconn_obj); \
             break; \
          } \
        eldbus_proxy_property_get(_oconn_proxy, NAME, func, data); \
   } while (0)


#define OCONNECTION_PROPERTY_GET_ALL(SERVICE, PATH, IFACE, func, data) \
   do { \
        Eldbus_Object *_oconn_obj; \
        Eldbus_Proxy *_oconn_proxy; \
        _oconn_obj = eldbus_object_get(_nm_dbus, SERVICE, PATH); \
        if (!_oconn_obj) break; \
        _oconn_proxy = eldbus_proxy_get(_oconn_obj, IFACE); \
        if (!_oconn_proxy) \
          { \
             eldbus_object_unref(_oconn_obj); \
             break; \
          } \
        eldbus_proxy_property_get_all(_oconn_proxy, func, data); \
   } while (0)

#define OCONNECTION_CHECK(MSG) \
   do { \
        const char *errname, *errmsg; \
        if (eldbus_message_error_get(MSG, &errname, &errmsg)) \
          { \
             fprintf(stderr, "%s:%d %s %s\n", __FILE__,  __LINE__, errname, errmsg); \
             return; \
          } \
   } \
   while (0)

#if 0
#define ODBG_PROP ODBG
#else
#define ODBG_PROP(...)
#endif

static void _oconnection_nm_devices_get(void);
static void _oconnection_nm_devices_get_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_device_add(const char *path);
static void _oconnection_nm_device_iterate(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_device_update_property(void *data, const void *msg, Eldbus_Message_Iter *it);
static void _oconnection_nm_ip4_iterate(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_device_ip4_update_property(void *data, const void *msg, Eldbus_Message_Iter *it);
static void _oconnection_nm_wifi_connection_iterate(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_wifi_connection_update_property(void *data, const void *msg, Eldbus_Message_Iter *it);
static void _oconnection_nm_wifi_access_point_iterate(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_wifi_access_point_update_property(void *data, const void *msg, Eldbus_Message_Iter *it);
static void _oconnection_nm_util_ssid_get(Eldbus_Message_Iter *it, Eina_Stringshare **str);
static void _oconnection_nm_connected_access_point_property_update(void *data, const Eldbus_Message *msg);
static void _oconnection_nm_settings_get(void);
static void _oconnection_nm_settings_get_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_setting_add(const char *path);
static void _oconnection_nm_setting_add_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_setting_update_property(void *data, const void *msg, Eldbus_Message_Iter *it);
static void _oconnection_nm_setting_wifi_update_property(void *data, const void *msg, Eldbus_Message_Iter *it);
static void _oconnection_nm_scan_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_access_point_iterate(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_access_point_update_property(void *data, const void *msg, Eldbus_Message_Iter *it);
static Eina_Bool _oconnection_nm_scan_done(void *data);
static void _oconnection_nm_connect_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_activate_connection(OWireless_Network *ow, const char *conf_path);
static void _oconnection_nm_connect_apply_conf_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_wifi_state_changed(void *data, const Eldbus_Message *msg);
static void _oconnection_wired_state_changed(void *data, const Eldbus_Message *msg);
static void _oconnection_nm_signal_device_add(void *data, const Eldbus_Message *msg);
static void _oconnection_nm_signal_device_del(void *data, const Eldbus_Message *msg);
static Eina_Bool _oconnection_nm_setting_remove_from_spool(EINA_UNUSED const Eina_Hash *hash, const void *key, void *data, void *fdata);
static void _oconnection_nm_signal_setting_add(void *data, const Eldbus_Message *msg);
static void _oconnection_nm_signal_setting_del(void *data, const Eldbus_Message *msg);
static void _oconnection_nm_setting_remove_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_setting_connected_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_setting_save_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_nm_setting_save_done_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend);
static void _oconnection_recursive_iter_copy(Eldbus_Message_Iter *from, Eldbus_Message_Iter *to);

static Ecore_Timer *_scan_timer = NULL;
static Eina_Hash *_spool = NULL;
static Eina_List *_handlers = NULL;
static const char *_current_conf_path = NULL;
static OWireless_Network *_tmp_ow = NULL;

static Eldbus_Connection *_nm_dbus = NULL;

void
oconnection_nm_init(void)
{
   _spool = eina_hash_string_superfast_new((void (*)(void *))eina_stringshare_del);
   _nm_dbus = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM);
   ODBG("Oconnection Init\n");

   if (_nm_dbus)
     {
        _oconnection_nm_devices_get();
        _oconnection_nm_settings_get();
        _handlers =
           eina_list_append(_handlers,
                            eldbus_signal_handler_add(
                               _nm_dbus,
                               NM_DBUS_SERVICE, NM_DBUS_PATH_SETTINGS,
                               NM_DBUS_IFACE_SETTINGS,
                               "NewConnection",
                               _oconnection_nm_signal_setting_add, NULL));
        _handlers =
           eina_list_append(_handlers,
                            eldbus_signal_handler_add(
                               _nm_dbus,
                               NM_DBUS_SERVICE, NM_DBUS_PATH_SETTINGS,
                               NM_DBUS_IFACE_SETTINGS,
                               "ConnectionRemoved",
                               _oconnection_nm_signal_setting_del, NULL));
        _handlers =
           eina_list_append(_handlers,
                            eldbus_signal_handler_add(
                               _nm_dbus,
                               NM_DBUS_SERVICE, NM_DBUS_PATH,
                               NM_DBUS_INTERFACE,
                               "DeviceAdded",
                               _oconnection_nm_signal_device_add, NULL));
         _handlers =
           eina_list_append(_handlers,
                            eldbus_signal_handler_add(
                               _nm_dbus,
                               NM_DBUS_SERVICE, NM_DBUS_PATH,
                               NM_DBUS_INTERFACE,
                               "DeviceRemoved",
                               _oconnection_nm_signal_device_del, NULL));
     }
   else
     fprintf(stderr, "Cannot get DBUS system bus\n");
}

void
oconnection_nm_shutdown(void)
{
   Eldbus_Signal_Handler *h;

   if (_scan_timer) ecore_timer_del(_scan_timer);
   _scan_timer = NULL;
   eina_hash_free(_spool);
   EINA_LIST_FREE(_handlers, h)
      eldbus_signal_handler_del(h);
   eldbus_connection_unref(_nm_dbus);
}

void
oconnection_nm_scan(void)
{
   OIface_Status *os;
   Eina_List *l;

   EINA_LIST_FOREACH(oconnection_iface_wifi_get(), l, os)
     {
        OCONNECTION_CALL(NM_DBUS_SERVICE, os->dbus_path,
                         NM_DBUS_INTERFACE_DEVICE_WIRELESS,
                         "GetAccessPoints",
                         _oconnection_nm_scan_cb, NULL, "");
     }
}

void
oconnection_nm_connect_hidden(const char *ssid, const char *psk, Oconnection_Network_Flags key_type)
{
   _tmp_ow = calloc(1, sizeof(OWireless_Network));
   _tmp_ow->ssid = eina_stringshare_add(ssid);
   _tmp_ow->flags = key_type;
   _tmp_ow->dbus_path = eina_stringshare_add("/");
   oconnection_nm_connect(_tmp_ow, psk);
}

void
oconnection_nm_connect(OWireless_Network *ow, const char *psk)
{
   uuid_t uuid;
   const char *key, *val;
   char *buf;
   uint32_t t;
   int i;
   const char *conf_path;
   Eldbus_Message *msg;
   Eldbus_Message_Iter *it, *conf, *conn, *group, *param, *var;
   Eldbus_Object *obj;
   Eldbus_Proxy *proxy;

   obj = eldbus_object_get(_nm_dbus, NM_DBUS_SERVICE, NM_DBUS_PATH_SETTINGS);
   proxy = eldbus_proxy_get(obj, NM_DBUS_IFACE_SETTINGS);
   conf_path = eina_hash_find(_spool, ow->ssid);
   if (!conf_path)
     {
        ODBG("Create new settings\n");
        buf = malloc(37);
        uuid_generate_random(uuid);
        uuid_unparse_lower(uuid, buf);

        msg = eldbus_proxy_method_call_new(proxy, "AddConnectionUnsaved");
        it = eldbus_message_iter_get(msg);
        eldbus_message_iter_arguments_append(it, "a{sa{sv}}", &conf);

        /* connection */
        ODBG("Create new settings connection\n");
        key = "connection";
        eldbus_message_iter_arguments_append(conf, "{sa{sv}}", &conn);
        eldbus_message_iter_basic_append(conn, 's', key);
        eldbus_message_iter_arguments_append(conn, "a{sv}", &group);
          {
             /* connection / id */
             ODBG("Create new settings id\n");
             key = "id";
             val = ow->ssid;
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "s");
             eldbus_message_iter_basic_append(var, 's', val);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);

             /* connection / uuid */
             ODBG("Create new settings uuid\n");
             key = "uuid";
             val = buf;
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "s");
             eldbus_message_iter_basic_append(var, 's', val);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);

             /* connection / timestamp */
             ODBG("Create new settings timestamp\n");
             key = "timestamp";
             t = time(NULL);
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "u");
             eldbus_message_iter_basic_append(var, 'u', t);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);

             /* connection / type */
             ODBG("Create new settings type\n");
             key = "type";
             val = "802-11-wireless";
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "s");
             eldbus_message_iter_basic_append(var, 's', val);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);

             /* connection / autoconnect */
             ODBG("Create new settings autoconnect\n");
             key = "autoconnect";
             t = EINA_TRUE;
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "b");
             eldbus_message_iter_basic_append(var, 'b', t);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);

             /* connection / autoconnect / priority */
             ODBG("Create new settings priority\n");
             key = "autoconnect-priority";
             if (psk)
               t = 99;
             else
               t = 1;
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "u");
             eldbus_message_iter_basic_append(var, 'u', t);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);

             /* connection / autoconnect / retries */
             ODBG("Create new settings retries\n");
             key = "autoconnect-retries";
             t = -1; /* Default value (4 retries) */
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "u");
             eldbus_message_iter_basic_append(var, 'u', t);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);
          }
        eldbus_message_iter_container_close(conn, group);
        eldbus_message_iter_container_close(conf, conn);

        /* 802-11-wireless */
        ODBG("Create new settings 802-11-wireless\n");
        key = "802-11-wireless";
        eldbus_message_iter_arguments_append(conf, "{sa{sv}}", &conn);
        eldbus_message_iter_basic_append(conn, 's', key);
        eldbus_message_iter_arguments_append(conn, "a{sv}", &group);
          {
             /* 802-11-wireless / ssid */
             Eldbus_Message_Iter *bs;
             ODBG("Create new settings 802-11-wireless/ssid\n");
             key = "ssid";
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "ay");
             eldbus_message_iter_arguments_append(var, "ay", &bs);
             for (i = 0; i < strlen(ow->ssid); ++i)
               {
                  eldbus_message_iter_basic_append(bs, 'y', ow->ssid[i]);
               }
             eldbus_message_iter_container_close(var, bs);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);

             /* 802-11-wireless / mode */
             ODBG("Create new settings 802-11-wireless/mode\n");
             key = "ssid";
             key = "mode";
             val = "infrastructure";
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "s");
             eldbus_message_iter_basic_append(var, 's', val);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);

             if (psk)
               {
                  /* 802-11-wireless / security */
                  ODBG("Create new settings 802-11-wireless/security\n");
                  key = "security";
                  val = "802-11-wireless-security";
                  eldbus_message_iter_arguments_append(group, "{sv}", &param);
                  eldbus_message_iter_basic_append(param, 's', key);
                  var = eldbus_message_iter_container_new(param, 'v', "s");
                  eldbus_message_iter_basic_append(var, 's', val);
                  eldbus_message_iter_container_close(param, var);
                  eldbus_message_iter_container_close(group, param);
               }
          }
        eldbus_message_iter_container_close(conn, group);
        eldbus_message_iter_container_close(conf, conn);

        if (psk)
          {
             /* 802-11-wireless-security */
             ODBG("Create new settings 802-11-wireless-security\n");
             key = "802-11-wireless-security";
             eldbus_message_iter_arguments_append(conf, "{sa{sv}}", &conn);
             eldbus_message_iter_basic_append(conn, 's', key);
             eldbus_message_iter_arguments_append(conn, "a{sv}", &group);

             /* 802-11-wireless-security / key-mgmt */
             ODBG("Create new settings 802-11-wireless-security/key-mgmt\n");
             key = "key-mgmt";
             if (ow->flags & NW_FLAG_WPA)
               {
                  val = "wpa-psk";
               }
             else if (ow->flags & NW_FLAG_WEP)
               {
                  val = "none";
               }
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "s");
             eldbus_message_iter_basic_append(var, 's', val);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);

             if (ow->flags & NW_FLAG_WPA)
               {
                  /* 802-11-wireless-security / psk */
                  ODBG("Create new settings 802-11-wireless-security/psk\n");
                  key = "psk";
                  val = psk;
                  eldbus_message_iter_arguments_append(group, "{sv}", &param);
                  eldbus_message_iter_basic_append(param, 's', key);
                  var = eldbus_message_iter_container_new(param, 'v', "s");
                  eldbus_message_iter_basic_append(var, 's', val);
                  eldbus_message_iter_container_close(param, var);
                  eldbus_message_iter_container_close(group, param);
               }
             else if (ow->flags & NW_FLAG_WEP)
               {
                  char k[32];
                  int idx;

                  ODBG("Create new settings 802-11-wireless-security/psk\n");
                  for (idx = 0; idx < 4; ++idx)
                    {
                       snprintf(k, sizeof(k), "wep-key%d", idx);
                       key = k;
                       val = psk;
                       eldbus_message_iter_arguments_append(group, "{sv}", &param);
                       eldbus_message_iter_basic_append(param, 's', key);
                       var = eldbus_message_iter_container_new(param, 'v', "s");
                       eldbus_message_iter_basic_append(var, 's', val);
                       eldbus_message_iter_container_close(param, var);
                       eldbus_message_iter_container_close(group, param);
                    }
               }

             eldbus_message_iter_container_close(conn, group);
             eldbus_message_iter_container_close(conf, conn);
          }

        /* ipv4 */
        ODBG("Create new settings ipv4\n");
        key = "ipv4";
        eldbus_message_iter_arguments_append(conf, "{sa{sv}}", &conn);
        eldbus_message_iter_basic_append(conn, 's', key);
        eldbus_message_iter_arguments_append(conn, "a{sv}", &group);
          {
             /* ipv4 / method */
             ODBG("Create new settings ipv4/method\n");
             key = "method";
             val = "auto";
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "s");
             eldbus_message_iter_basic_append(var, 's', val);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);
          }
        eldbus_message_iter_container_close(conn, group);
        eldbus_message_iter_container_close(conf, conn);


        /* ipv6 */
        ODBG("Create new settings ipv6\n");
        key = "ipv6";
        eldbus_message_iter_arguments_append(conf, "{sa{sv}}", &conn);
        eldbus_message_iter_basic_append(conn, 's', key);
        eldbus_message_iter_arguments_append(conn, "a{sv}", &group);
          {
             /* ipv6 / method */
             ODBG("Create new settings ipv6/method\n");
             key = "method";
             val = "auto";
             eldbus_message_iter_arguments_append(group, "{sv}", &param);
             eldbus_message_iter_basic_append(param, 's', key);
             var = eldbus_message_iter_container_new(param, 'v', "s");
             eldbus_message_iter_basic_append(var, 's', val);
             eldbus_message_iter_container_close(param, var);
             eldbus_message_iter_container_close(group, param);
          }
        eldbus_message_iter_container_close(conn, group);
        eldbus_message_iter_container_close(conf, conn);
        eldbus_message_iter_container_close(it, conf);

        eldbus_proxy_send(proxy, msg, _oconnection_nm_connect_cb, ow, -1);
        free(buf);
     }
   else
     {
        ODBG("Use previous settings %s\n", conf_path);
        _oconnection_nm_activate_connection(ow, conf_path);
     }
}

Eina_Bool
oconnection_nm_is_in_spool_get(const char *key)
{
   return !!eina_hash_find(_spool, key);
}

static void
_oconnection_nm_devices_get(void)
{
   ODBG("request devices list\n");
   OCONNECTION_CALL(NM_DBUS_SERVICE, NM_DBUS_PATH, NM_DBUS_INTERFACE,
                    "GetDevices",
                    _oconnection_nm_devices_get_cb, NULL, "");


}

static void
_oconnection_nm_devices_get_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   char *iface;
   Eldbus_Message_Iter *it;

   OCONNECTION_CHECK(msg);
   if (!eldbus_message_arguments_get(msg, "ao", &it))
     {
        fprintf(stderr, "Invalid device\n");
        return;
     }

   while (eldbus_message_iter_get_and_next(it, 'o', &iface))
     {
        _oconnection_nm_device_add(iface);
     }
}

static void
_oconnection_nm_device_add(const char *path)
{
   OIface_Status *os;

   ODBG("device add %s\n", path);

   os = calloc(1, sizeof(OIface_Status));
   os->dbus_path = eina_stringshare_add(path);
   OCONNECTION_PROPERTY_GET_ALL(NM_DBUS_SERVICE, path, NM_DBUS_INTERFACE_DEVICE,
                                _oconnection_nm_device_iterate, os);
}

static void
_oconnection_nm_device_del(const char *path)
{
   OIface_Status *os = NULL;
   Eina_List *l;

   ODBG("device del %s\n", path);

   EINA_LIST_FOREACH(oconnection_iface_wired_get(), l, os)
     {
        if (!strcmp(os->dbus_path, path))
          {
             break;
          }
     }
   if (!l)
     {
        EINA_LIST_FOREACH(oconnection_iface_wifi_get(), l, os)
          {
             if (!strcmp(os->dbus_path, path))
               {
                  break;
               }
          }
     }
   if (os)
     oconnection_iface_del(os);
}

static void
_oconnection_nm_device_iterate(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   OIface_Status *os;
   Eldbus_Message_Iter *array;
   OCONNECTION_CHECK(msg);

   os = data;

   if (eldbus_message_arguments_get(msg, "a{sv}", &array))
     eldbus_message_iter_dict_iterate(array, "sv",
                                      _oconnection_nm_device_update_property,
                                      os);


   ODBG("device update...\n");
   if (os->supported)
     {
        if (os->is_wifi)
          {
             if (!os->in_list)
               {
                  os->in_list = EINA_TRUE;
                  _handlers =
                     eina_list_append(_handlers,
                                      eldbus_signal_handler_add(
                                         _nm_dbus,
                                         NM_DBUS_SERVICE,
                                         os->dbus_path,
                                         NM_DBUS_INTERFACE_DEVICE,
                                         "StateChanged",
                                         _oconnection_wifi_state_changed, os));
                  oconnection_iface_wifi_add(os);
               }

             if (os->connection_path)
               {
                  if ((os->connected) && (strcmp(os->connection_path, "/")))
                    OCONNECTION_PROPERTY_GET_ALL(NM_DBUS_SERVICE,
                                                 os->connection_path,
                                                 NM_DBUS_INTERFACE_ACTIVE_CONNECTION,
                                                 _oconnection_nm_wifi_connection_iterate,
                                                 os);
               }
          }
        else
          {
             if (!os->in_list)
               {
                  os->in_list = EINA_TRUE;
                  _handlers =
                     eina_list_append(_handlers,
                                      eldbus_signal_handler_add(
                                         _nm_dbus,
                                         NM_DBUS_SERVICE,
                                         os->dbus_path,
                                         NM_DBUS_INTERFACE_DEVICE,
                                         "StateChanged",
                                         _oconnection_wired_state_changed, os));
                  oconnection_iface_wired_add(os);
               }
          }
     }
   oconnection_iface_update(os);
}

static void
_oconnection_nm_device_update_property(void *data, const void *msg, Eldbus_Message_Iter *it)
{
   OIface_Status *os = data;
   const char *key = msg;
   ODBG_PROP("Update device %s property %s\n", os->name, key);
   if (!key) return;

   if (!strcmp(key, "DeviceType"))
     {
        unsigned int value;

        if (!eldbus_message_iter_arguments_get(it, "u", &value)) return;
        switch (value)
          {
           case NM_DEVICE_TYPE_ETHERNET:
              os->is_wifi = 0;
              os->supported = 1;
              break;
           case NM_DEVICE_TYPE_WIFI:
              os->is_wifi = 1;
              os->supported = 1;
              break;
           default:
              fprintf(stderr, "IFace type unsupported\n");
              os->supported = 0;
          }
     }
   else if (!strcmp(key, "State"))
     {
        unsigned int value;

        if (!eldbus_message_iter_arguments_get(it, "u", &value)) return;
        os->connected = !!(value == NM_DEVICE_STATE_ACTIVATED);
        os->enabled = 1;
     }
   else if (!strcmp(key, "Interface"))
     {
        char *value;

        if (!eldbus_message_iter_arguments_get(it, "s", &value)) return;
        eina_stringshare_replace(&os->name, value);
     }
   else if (!strcmp(key, "Ip4Config"))
     {
        char *value;

        if (!eldbus_message_iter_arguments_get(it, "o", &value)) return;
        if ((os->connected) && (strcmp(value, "/")))
          OCONNECTION_PROPERTY_GET_ALL(NM_DBUS_SERVICE,
                                       value,
                                       NM_DBUS_INTERFACE_IP4_CONFIG,
                                       _oconnection_nm_ip4_iterate, os);
     }
   else if (!strcmp(key, "ActiveConnection"))
     {
        char *value;

        if (!eldbus_message_iter_arguments_get(it, "o", &value))
          {
             eina_stringshare_del(os->connection_path);
             os->connection_path = NULL;
          }
        else
          eina_stringshare_replace(&os->connection_path, value);
     }
}


static void
_oconnection_nm_ip4_iterate(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   OIface_Status *os = data;
   Eldbus_Message_Iter *array;
   OCONNECTION_CHECK(msg);

   if (eldbus_message_arguments_get(msg, "a{sv}", &array))
     eldbus_message_iter_dict_iterate(array, "sv",
                                      _oconnection_nm_device_ip4_update_property,
                                      os);
}

static void
_oconnection_nm_device_ip4_update_property(void *data, const void *msg, Eldbus_Message_Iter *it)
{
   OIface_Status *os = data;
   const char *key = msg;
   struct in_addr tmp_addr;
   char buf[INET_ADDRSTRLEN+1];
   Eldbus_Message_Iter *itaa, *ita;
   size_t addr;

   ODBG_PROP("Update device ip4 %s property %s\n", os->name, key);
   if (!key) return;
   if (!strcmp(key, "Addresses"))
     {
        if (!eldbus_message_iter_arguments_get(it, "aau", &itaa)) return;
        if (!eldbus_message_iter_arguments_get(itaa, "au", &ita)) return;
        if (!eldbus_message_iter_get_and_next(ita, 'u', &addr)) return;
        memset (&buf, '\0', sizeof (buf));
        tmp_addr.s_addr = addr;

        inet_ntop (AF_INET, &tmp_addr, buf, INET_ADDRSTRLEN);
        eina_stringshare_replace(&os->addr, buf);
        ODBG_PROP("Ip4Address %d %s\n", addr, inet_ntoa(tmp_addr));
     }
}

static void
_oconnection_nm_wifi_connection_iterate(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   OIface_Status *os = data;
   Eldbus_Message_Iter *array;
   OCONNECTION_CHECK(msg);

   if (eldbus_message_arguments_get(msg, "a{sv}", &array))
     eldbus_message_iter_dict_iterate(array, "sv",
                                      _oconnection_nm_wifi_connection_update_property,
                                      os);
}

static void
_oconnection_nm_wifi_connection_update_property(void *data, const void *msg, Eldbus_Message_Iter *it)
{
   OIface_Status *os = data;
   const char *key = msg;
   ODBG_PROP("Wifi connection %s property %s\n", os->name, key);
   if (!key) return;

   if (!strcmp(key, "SpecificObject"))
     {
        char *value;

        if (!eldbus_message_iter_arguments_get(it, "o", &value)) return;
        if (strcmp(value, "/"))
          {

             OCONNECTION_PROPERTY_GET_ALL(NM_DBUS_SERVICE,
                                          value,
                                          NM_DBUS_INTERFACE_ACCESS_POINT,
                                          _oconnection_nm_wifi_access_point_iterate,
                                          os);
             if (os->data)
               {
                  ODBG("Remove old level update handler\n");
                  eldbus_signal_handler_del(os->data);
               }
             ODBG("Add level update handler %s\n", value);
             os->data = eldbus_signal_handler_add(
                _nm_dbus,
                NM_DBUS_SERVICE,
                value,
                NM_DBUS_INTERFACE_ACCESS_POINT,
                "PropertiesChanged",
                _oconnection_nm_connected_access_point_property_update,
                os);
          }
     }
}

static void
_oconnection_nm_wifi_access_point_iterate(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   OIface_Status *os = data;
   Eldbus_Message_Iter *array;
   OCONNECTION_CHECK(msg);

   ODBG_PROP("Wifi access point iterate\n");
   if (eldbus_message_arguments_get(msg, "a{sv}", &array))
     eldbus_message_iter_dict_iterate(array, "sv",
                                      _oconnection_nm_wifi_access_point_update_property,
                                      os);
}


static void
_oconnection_nm_wifi_access_point_update_property(void *data, const void *msg, Eldbus_Message_Iter *it)
{
   OIface_Status *os = data;
   const char *key = msg;

   ODBG_PROP("Update wifi access point %s property %s\n", os->name, key);
   if (!key) return;
   if (!strcmp(key, "Ssid"))
     {
        _oconnection_nm_util_ssid_get(it, &os->ssid);
     }
   else if (!strcmp(key, "Strength"))
     {
        unsigned char value;
        if (!eldbus_message_iter_arguments_get(it, "y", &value)) return;
        os->qual = value;
     }
   oconnection_iface_update(os);
}

static void
_oconnection_nm_util_ssid_get(Eldbus_Message_Iter *it, Eina_Stringshare **str)
{
   Eldbus_Message_Iter *ity;
   unsigned char value;
   char ssid[33]; /* ssid max len 32 */
   int i = 0;

   if (eldbus_message_iter_arguments_get(it, "ay", &ity))
     {
        while (eldbus_message_iter_get_and_next(ity, 'y', &value))
          {
             ssid[i] = value;
             ++i;
             if (i > 32)
               {
                  i = 32;
                  break;
               }
          }
        ssid[i] = '\0';
     }
   if (i)
     {
        eina_stringshare_replace(str, ssid);
     }
}


static void
_oconnection_nm_connected_access_point_property_update(void *data, const Eldbus_Message *msg)
{
   OIface_Status *os;
   os = data;
   Eldbus_Message_Iter *array;
   OCONNECTION_CHECK(msg);

   if (eldbus_message_arguments_get(msg, "a{sv}", &array))
     eldbus_message_iter_dict_iterate(array, "sv",
                                      _oconnection_nm_wifi_access_point_update_property,
                                      os);
}

static void
_oconnection_nm_settings_get(void)
{
   ODBG("Request settings list\n");
   OCONNECTION_CALL(NM_DBUS_SERVICE, NM_DBUS_PATH_SETTINGS,
                    NM_DBUS_IFACE_SETTINGS,
                    "ListConnections",
                    _oconnection_nm_settings_get_cb, NULL, "");
}

static void
_oconnection_nm_settings_get_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   Eldbus_Message_Iter *it;
   char *value;

   OCONNECTION_CHECK(msg);

   if (!eldbus_message_arguments_get(msg, "ao", &it))
     {
        fprintf(stderr, "Invalid settings list results\n");
        return;
     }

   while (eldbus_message_iter_get_and_next(it, 'o', &value))
     {
        _oconnection_nm_setting_add(value);
     }
}

static void
_oconnection_nm_setting_add(const char *path)
{
   ODBG("Add setting %s\n", path);
   OCONNECTION_CALL(NM_DBUS_SERVICE, path,
                    NM_DBUS_IFACE_SETTINGS_CONNECTION,
                    "GetSettings",
                    _oconnection_nm_setting_add_cb,
                    (void *)eina_stringshare_add(path),
                    "");
}

static void
_oconnection_nm_setting_add_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   Eldbus_Message_Iter *array;
   OCONNECTION_CHECK(msg);
   if (eldbus_message_arguments_get(msg, "a{sa{sv}}", &array))
     {
        eldbus_message_iter_dict_iterate(array, "sa{sv}",
                                         _oconnection_nm_setting_update_property,
                                         data);
     }
}

static void
_oconnection_nm_setting_update_property(void *data, const void *msg, Eldbus_Message_Iter *it)
{
   const char *key = msg;

   ODBG_PROP("Update setting %s property %s\n", data, key);

   if (!strcmp(key, "802-11-wireless"))
     {
        eldbus_message_iter_dict_iterate(it, "sv",
                                         _oconnection_nm_setting_wifi_update_property,
                                         data);
     }
}


static void
_oconnection_nm_setting_wifi_update_property(void *data, const void *msg, Eldbus_Message_Iter *it)
{
   const char *key = msg;
   ODBG_PROP("Update setting property %s\n", key);
   const char *ssid = NULL;

   if (!strcmp(key, "ssid"))
     {
        _oconnection_nm_util_ssid_get(it, &ssid);
        if (!eina_hash_find(_spool, ssid))
          {
             ODBG("Add %s setting for ssid %s to spool\n", (char *)data, ssid);
             eina_hash_add(_spool, ssid, data);
          }
        else
          eina_stringshare_del(data);
        eina_stringshare_del(ssid);
     }
}


static void
_oconnection_nm_scan_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   char *ssid_path;
   OWireless_Network *ow;
   Eina_List *l;
   Eldbus_Message_Iter *it;

   OCONNECTION_CHECK(msg);
   EINA_LIST_FOREACH(oconnection_scan_list_get(), l, ow)
     {
        ow->found = 0;
     }

   if (!eldbus_message_arguments_get(msg, "ao", &it))
     {
        fprintf(stderr, "Invalid scan results\n");
        return;
     }

   while (eldbus_message_iter_get_and_next(it, 'o', &ssid_path))
     {
        EINA_LIST_FOREACH(oconnection_scan_list_get(), l, ow)
          {
             if (!strcmp(ow->dbus_path, ssid_path))
               {
                  ow->found = 1;
                  break;
               }
          }
        if (!l)
          {
             ow = calloc(1, sizeof(OWireless_Network));
             ow->dbus_path = eina_stringshare_add(ssid_path);
             ow->found = 1;
             oconnection_scan_add(ow);
          }
        OCONNECTION_PROPERTY_GET_ALL(NM_DBUS_SERVICE,
                                     ssid_path,
                                     NM_DBUS_INTERFACE_ACCESS_POINT,
                                     _oconnection_nm_access_point_iterate, ow);
     }
}

static void
_oconnection_nm_access_point_iterate(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   OWireless_Network *ow = data;
   Eldbus_Message_Iter *array;
   OCONNECTION_CHECK(msg);

   if (eldbus_message_arguments_get(msg, "a{sv}", &array))
     eldbus_message_iter_dict_iterate(array, "sv",
                                      _oconnection_nm_access_point_update_property,
                                      ow);
}

static void
_oconnection_nm_access_point_update_property(void *data, const void *msg, Eldbus_Message_Iter *it)
{
   OWireless_Network *ow = data;
   const char *key = msg;
   unsigned int flags;

   ODBG_PROP("Update access point property %s\n", key);
   if (!strcmp(key, "Flags"))
     {
        if (!eldbus_message_iter_arguments_get(it, "u", &flags)) return;
        if (flags) ow->flags |= NW_FLAG_PRIVACY;
     }
   else if (!strcmp(key, "Frequency"))
     {
     }
   else if (!strcmp(key, "HwAddress"))
     {
     }
   else if (!strcmp(key, "MaxBitrate"))
     {
     }
   else if (!strcmp(key, "Mode"))
     {
     }
   else if (!strcmp(key, "RsnFlags"))
     {
        if (!eldbus_message_iter_arguments_get(it, "u", &flags)) return;
        if ((flags & NM_802_11_AP_SEC_PAIR_WEP40)
            || (flags & NM_802_11_AP_SEC_PAIR_WEP104)
            || (flags & NM_802_11_AP_SEC_GROUP_WEP40)
            || (flags & NM_802_11_AP_SEC_GROUP_WEP104))
          ow->flags |= NW_FLAG_WEP;
        else if ((flags & NM_802_11_AP_SEC_PAIR_TKIP)
                 || (flags & NM_802_11_AP_SEC_PAIR_CCMP)
                 || (flags & NM_802_11_AP_SEC_GROUP_TKIP)
                 || (flags & NM_802_11_AP_SEC_GROUP_CCMP)
                 || (flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
                 || (flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X))
          ow->flags |= NW_FLAG_WPA; // | NW_FLAG_WPA2;
	else if (ow->flags) ow->flags |= NW_FLAG_WEP;
     }
   else if (!strcmp(key, "Ssid"))
     {
        _oconnection_nm_util_ssid_get(it, &ow->ssid);
     }
   else if (!strcmp(key, "Strength"))
     {
        unsigned char value;

        if (!eldbus_message_iter_arguments_get(it, "y", &value)) return;
        ow->signal = value;
     }
   else if (!strcmp(key, "WpaFlags"))
     {
        unsigned int flags;

        if (!eldbus_message_iter_arguments_get(it, "u", &flags)) return;
        if ((flags & NM_802_11_AP_SEC_PAIR_WEP40)
            || (flags & NM_802_11_AP_SEC_PAIR_WEP104)
            || (flags & NM_802_11_AP_SEC_GROUP_WEP40)
            || (flags & NM_802_11_AP_SEC_GROUP_WEP104))
          ow->flags |= NW_FLAG_WEP;
        else if ((flags & NM_802_11_AP_SEC_PAIR_TKIP)
                 || (flags & NM_802_11_AP_SEC_PAIR_CCMP)
                 || (flags & NM_802_11_AP_SEC_GROUP_TKIP)
                 || (flags & NM_802_11_AP_SEC_GROUP_CCMP)
                 || (flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
                 || (flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X))
          ow->flags |= NW_FLAG_WPA; // | NW_FLAG_WPA2;
     }
   if (_scan_timer) ecore_timer_del(_scan_timer);
   _scan_timer = ecore_timer_add(0.4, _oconnection_nm_scan_done, NULL);
}

static Eina_Bool
_oconnection_nm_scan_done(void *data EINA_UNUSED)
{
   _scan_timer = NULL;
   oconnection_scan_hidden_set(EINA_TRUE, EINA_TRUE, EINA_TRUE);
   oconnection_scan_done();
   return ECORE_CALLBACK_CANCEL;
}

static void
_oconnection_nm_connect_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   char *conf_path;
   OWireless_Network *ow;
   Eldbus_Message_Iter *it;
   const char *errname, *errmsg;

   ow = data;
   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        oconnection_wifi_connect_associated(EINA_FALSE);
        fprintf(stderr, "Invalid setting %s %s\n", errname, errmsg);
     }

   it = eldbus_message_iter_get(msg);
   if (!eldbus_message_iter_arguments_get(it, "o", &conf_path)) return;
   _oconnection_nm_activate_connection(ow, conf_path);
}

static void
_oconnection_nm_activate_connection(OWireless_Network *ow, const char *conf_path)
{
   Eina_List *l;
   OIface_Status *os;

   eina_stringshare_replace(&_current_conf_path, conf_path);
   EINA_LIST_FOREACH(oconnection_iface_wifi_get(), l, os)
     {
        OCONNECTION_CALL(NM_DBUS_SERVICE, NM_DBUS_PATH, NM_DBUS_INTERFACE,
                         "ActivateConnection",
                         _oconnection_nm_connect_apply_conf_cb, NULL, "ooo",
                         conf_path, os->dbus_path, ow->dbus_path);
        ODBG("Activate connection %s %s %s\n", conf_path, os->dbus_path, ow->dbus_path);
     }
   if (_tmp_ow)
     {
        eina_stringshare_del(_tmp_ow->ssid);
        eina_stringshare_del(_tmp_ow->dbus_path);
        free(_tmp_ow);
        _tmp_ow = NULL;
     }
}

static void
_oconnection_nm_connect_apply_conf_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   Eldbus_Message_Iter *it;
   char *conf_path;

   OCONNECTION_CHECK(msg);
   it = eldbus_message_iter_get(msg);
   if (!eldbus_message_iter_arguments_get(it, "o", &conf_path)) return;
   ODBG("Activate Connection done %s\n", conf_path);
}

static void
_oconnection_wired_state_changed(void *data, const Eldbus_Message *msg)
{
   int state1, state2, state3;
   OIface_Status *os;
   Eldbus_Message_Iter *it;

   os = data;

   it = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_get(it, "uuu", &state1, &state2, &state3);
   ODBG("state changed %d %d %d | %s\n", state1, state2, state3, os->connection_path);

   if (state1 != NM_DEVICE_STATE_UNMANAGED)
     {
        OCONNECTION_PROPERTY_GET_ALL(NM_DBUS_SERVICE, os->dbus_path,
                                     NM_DBUS_INTERFACE_DEVICE,
                                     _oconnection_nm_device_iterate, os);
     }
}

static void
_oconnection_wifi_state_changed(void *data, const Eldbus_Message *msg)
{
   int state1, state2, state3;
   OIface_Status *os;
   Eldbus_Message_Iter *it;

   os = data;

   it = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_get(it, "uuu", &state1, &state2, &state3);
   ODBG("state changed %d %d %d | %s\n", state1, state2, state3, os->connection_path);
   if (state1 == NM_DEVICE_STATE_DISCONNECTED)
     {
        if (os->connection_path)
          {
             eina_stringshare_del(os->connection_path);
             os->connection_path = NULL;
          }
     }
   if (state1 == NM_DEVICE_STATE_DEACTIVATING)
     {
        if (os->connection_path)
          {
             ODBG("Deactivating %s\n", os->connection_path);
             eina_stringshare_del(os->connection_path);
             os->connection_path = NULL;
          }
     }
   if (state1 == NM_DEVICE_STATE_FAILED)
     {
        ODBG("State fail\n");
        if (state2 != NM_DEVICE_STATE_IP_CONFIG)
          oconnection_wifi_connect_associated(EINA_FALSE);
        else
          oconnection_wifi_connect_connected(EINA_FALSE);
        if (_current_conf_path)
          {
             ODBG("Remove settings %s\n", _current_conf_path);
             OCONNECTION_CALL(NM_DBUS_SERVICE, _current_conf_path,
                              NM_DBUS_IFACE_SETTINGS_CONNECTION,
                              "Delete",
                              _oconnection_nm_setting_remove_cb, NULL, "");
             eina_stringshare_del(_current_conf_path);
             _current_conf_path = NULL;
          }
        if (os->connection_path)
          {
             eina_stringshare_del(os->connection_path);
             os->connection_path = NULL;
          }
     }
   if (state1 == NM_DEVICE_STATE_IP_CONFIG)
     {
        ODBG("State ipconfig\n");
        oconnection_wifi_connect_associated(EINA_TRUE);
     }
   if (state1 == NM_DEVICE_STATE_ACTIVATED)
     {
        ODBG("State connected\n");
        os->connected = EINA_TRUE;
        oconnection_wifi_connect_connected(EINA_TRUE);
        if (os->connection_path)
          {
             OCONNECTION_GET(NM_DBUS_SERVICE, os->connection_path,
                             NM_DBUS_INTERFACE_ACTIVE_CONNECTION,
                             "Connection",
                             _oconnection_nm_setting_connected_cb, NULL);
          }
     }
   if (state1 != NM_DEVICE_STATE_UNMANAGED)
     {
        OCONNECTION_PROPERTY_GET_ALL(NM_DBUS_SERVICE, os->dbus_path,
                                     NM_DBUS_INTERFACE_DEVICE,
                                     _oconnection_nm_device_iterate, os);
     }
}

static Eina_Bool
_oconnection_nm_setting_remove_from_spool(EINA_UNUSED const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
   if (!strcmp(fdata, data))
     {
        ODBG("Setting %s remove from spool\n", (char *)key);
        eina_hash_del(_spool, key, NULL);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static void
_oconnection_nm_signal_setting_add(void *data, const Eldbus_Message *msg)
{
   Eldbus_Message_Iter *it;
   char *v = NULL;

   OCONNECTION_CHECK(msg);

   it = eldbus_message_iter_get(msg);
   eldbus_message_iter_basic_get(it, &v);
   ODBG("Signal new Setting %s\n", v);
   _oconnection_nm_setting_add(v);
}

static void
_oconnection_nm_signal_setting_del(void *data, const Eldbus_Message *msg)
{
   Eldbus_Message_Iter *it;
   char *v = NULL;

   OCONNECTION_CHECK(msg);

   it = eldbus_message_iter_get(msg);
   eldbus_message_iter_basic_get(it, &v);
   ODBG("Signal remove Setting %s\n", v);

   eina_hash_foreach(_spool, _oconnection_nm_setting_remove_from_spool, v);
}

static void
_oconnection_nm_signal_device_add(void *data, const Eldbus_Message *msg)
{
   Eldbus_Message_Iter *it;
   char *v = NULL;

   OCONNECTION_CHECK(msg);

   it = eldbus_message_iter_get(msg);
   eldbus_message_iter_basic_get(it, &v);
   ODBG("Signal device added %s\n", v);
   _oconnection_nm_device_add(v);
}

static void
_oconnection_nm_signal_device_del(void *data, const Eldbus_Message *msg)
{
   Eldbus_Message_Iter *it;
   char *v = NULL;

   OCONNECTION_CHECK(msg);

   it = eldbus_message_iter_get(msg);
   eldbus_message_iter_basic_get(it, &v);
   ODBG("Signal device removed %s\n", v);
   _oconnection_nm_device_del(v);
}

static void
_oconnection_nm_setting_remove_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   OCONNECTION_CHECK(msg);
}

static void
_oconnection_nm_setting_connected_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   Eldbus_Message_Iter *it, *variant;
   char *conf_path = NULL;

   OCONNECTION_CHECK(msg);
   it = eldbus_message_iter_get(msg);
   if (!eldbus_message_iter_arguments_get(it, "v", &variant)) return;
   if (!eldbus_message_iter_arguments_get(variant, "o", &conf_path)) return;
   ODBG("Request to save settings %s\n", conf_path);
   OCONNECTION_CALL(NM_DBUS_SERVICE,
                    conf_path,
                    NM_DBUS_IFACE_SETTINGS_CONNECTION,
                    "GetSettings",
                    _oconnection_nm_setting_save_cb,
                    (void *)eina_stringshare_add(conf_path), "");
}


static void
_oconnection_nm_setting_save_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   Eldbus_Object *obj;
   Eldbus_Proxy *proxy;
   Eldbus_Message *new_msg;
   Eldbus_Message_Iter *it;

   OCONNECTION_CHECK(msg);
   ODBG("Setting update %s\n", (char *)data);
   obj = eldbus_object_get(_nm_dbus, NM_DBUS_SERVICE, data);
   proxy = eldbus_proxy_get(obj, NM_DBUS_IFACE_SETTINGS_CONNECTION);
   new_msg = eldbus_proxy_method_call_new(proxy, "Update");
   it = eldbus_message_iter_get(new_msg);

   _oconnection_recursive_iter_copy(eldbus_message_iter_get(msg), it);

   eldbus_proxy_send(proxy, new_msg,
                     _oconnection_nm_setting_save_done_cb, NULL, -1);
   eina_stringshare_del(data);

}

static void
_oconnection_nm_setting_save_done_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pend)
{
   OCONNECTION_CHECK(msg);
}

#if 1
#undef ODBG
#define ODBG(...)
#else
static int _indent = 0;
#endif
static void
_oconnection_recursive_iter_copy(Eldbus_Message_Iter *from, Eldbus_Message_Iter *to)
{
   char *sig;
   char *to_sig;

   if (!from || !to) return;

   sig = eldbus_message_iter_signature_get(from);
   ODBG("%*sRecurse copy\n", ++_indent, "", sig);
   /* iterate over iterator to copy */
   for (;
        ((sig) && (*sig));
        sig = eldbus_message_iter_signature_get(from)
       )
     {
        ODBG("%*sSig %s\n", _indent, "", sig);
        /* simply copy string type entries */
        if ((*sig == 's')
            || (*sig == 'o'))
          {
             char *v = NULL;
             eldbus_message_iter_basic_get(from, &v);
             if (v)
               {
                  ODBG("%*sCreate string %s -> %s\n", _indent, "", sig, v);
                  eldbus_message_iter_basic_append(to, *sig, v);
               }
          }
        else if ((*sig == 'y')
                 || (*sig == 'b')
                 || (*sig == 'n')
                 || (*sig == 'q')
                 || (*sig == 'i')
                 || (*sig == 'u')
                 || (*sig == 'x')
                 || (*sig == 't')
                 || (*sig == 'd')
                 || (*sig == 'h'))
          {
             /*
              * According to DBus documentation all
              * fixed-length types are guaranteed to fit
              * 8 bytes
              */
             uint64_t v = 0;
             ODBG("%*sCreate fixed length %c\n", _indent, "", *sig);
             eldbus_message_iter_basic_get(from, &v);
             eldbus_message_iter_basic_append(to, *sig, v);
          }
        else
          {
             /* recursively copy container type entries */
             Eldbus_Message_Iter *to_it, *from_it;
             if ((*sig == '{')
                 || (*sig == '(')
                 || (*sig == 'a'))
               {
                  char signature;

                  if (*sig == '{') signature = 'e';
                  else if (*sig == '(') signature = 'r';
                  else signature = 'a';
                  if (eldbus_message_iter_get_and_next(from, signature, &from_it))
                    {
                       ODBG("%*sCreate new conteneur %c - %s %d\n", _indent, "", signature, sig, next);
                       eldbus_message_iter_arguments_append(to, sig, &to_it);
                       _oconnection_recursive_iter_copy(from_it, to_it);
                       eldbus_message_iter_container_close(to, to_it);
                       continue;
                    }
                  break;
               }
             else
               {
                  if ((*sig == '}')
                      || (*sig == ')'))
                    break;
                  if (eldbus_message_iter_arguments_get(from, sig, &from_it))
                    {
                       to_sig = eldbus_message_iter_signature_get(from_it);
                       ODBG("%*sCreate new variant %s - %s\n", _indent, "", sig, to_sig);
                       to_it = eldbus_message_iter_container_new(to, 'v', to_sig);
                       _oconnection_recursive_iter_copy(from_it, to_it);
                       eldbus_message_iter_container_close(to, to_it);
                       free(to_sig);
                    }
               }
          }
        ODBG("%*sSig end %s\n", _indent, "", sig);
        if (!eldbus_message_iter_next(from))
          break;
        free(sig);
     }
   ODBG("%*sEnd of turn %s\n", --_indent, "", sig);
   free(sig);
}

