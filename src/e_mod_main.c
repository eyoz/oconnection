#include "e_mod_main.h"
#include "oconnection.h"

#define OAPPRENDRE_MIN_SPACE 20

typedef enum Oconnection_Step_
{
   OCONNECTION_STEP_INFO,
   OCONNECTION_STEP_INFO_ALL,
   OCONNECTION_STEP_SSID,
   OCONNECTION_STEP_KEY,
   OCONNECTION_STEP_KEY2,
   OCONNECTION_STEP_CONNECT,
   OCONNECTION_STEP_IPCONFIG,
   OCONNECTION_STEP_STATUS
} Oconnection_Step;

typedef enum Oconnection_Type_
{
   OCONNECTION_TYPE_NONE,
   OCONNECTION_TYPE_WIRED,
   OCONNECTION_TYPE_WIFI
} Oconnection_Type;

// for enlightenment
typedef struct _Instance Instance;
struct _Instance
{
   /* An instance of our item (module) with its elements */
   /* pointer to this gadget's container */
   E_Gadcon_Client *gcc;
   E_Gadcon_Popup *popup;

   /* evas_object used to display */
   Evas_Object *o_oconnection;
   Evas_Object *wifi_icon;
   Evas_Object *wired_icon;
   Evas_Object *no_connection_icon;
   Oconnection_Type connection_type;
   struct
     {
        Eina_List *oconnection_items;
        Eina_List *ifaces_wired;
        Eina_List *ifaces_wifi;
        unsigned int nth_item;
        Evas *evas;
        Evas_Object *list;
        Evas_Object *label;
        Evas_Object *box;
        Evas_Object *button_box;
        Evas_Object *iface_box;
        Evas_Object *scan_box;
        Eina_List *items;
        Eina_List *items_more;
        Eina_List *items_space;
        Eina_List *items_info;
        Eina_List *items_entry;
        Eina_List *items_button;
        struct
          {
             Ecore_X_Window win;
             Ecore_X_Window last_focused;
             Ecore_Event_Handler *mouse_up;
             Ecore_Event_Handler *key_down;
             E_Border *border;
          } input;
        Evas_Coord w;
        Evas_Coord h;
        Evas_Coord last_item_h;
        Evas_Coord last_network_h;
        Evas_Coord items_w;
        Evas_Coord items_h;
        Evas_Coord items_more_w;
        Evas_Coord items_more_h;
        Evas_Coord label_w;
        Evas_Coord label_h;
        Evas_Coord security_label_w;
        Evas_Coord security_label_h;
        Evas_Coord pct_label_w;
        Evas_Coord pct_label_h;
        Evas_Coord space_w;
        Evas_Coord space_h;
        Evas_Coord info_w;
        Evas_Coord info_h;
        Evas_Coord entry_w;
        Evas_Coord entry_h;
        Evas_Coord button_w;
        Evas_Coord button_h;
        char *ssid;
        char *key;
        OWireless_Network *ow;
        Oconnection_Network_Flags key_type;
        Eina_Bool connected:1;
        Eina_Bool associated:1;
        Oconnection_Step step;
        Ecore_Timer *status_timer;
        Ecore_Timer *popup_timer;
     } ui;
   Config_Item *conf_item;
};

typedef struct _Oconnection_Item
{
   Instance *inst;
   const char *ssid;
   const char *bssid;
   OWireless_Network *ow;
   Oconnection_Network_Flags flags;
   Evas_Object *o_lbl;
   Evas_Object *o_lvl;
   Evas_Object *o_sec;
   Evas_Object *o_fav;
} Oconnection_Item;

typedef struct _Oconnection_Wired_Status
{
   const char *name;
   const char *addr;
   Eina_Bool connected:1;
   Evas_Object *obj;
} Oconnection_Wired_Status;

typedef struct _Oconnection_Wifi_Status
{
   const char *name;
   const char *addr;
   Eina_Bool connected:1;
   const char *ssid;
   Evas_Object *obj;
} Oconnection_Wifi_Status;

/* Local Function Prototypes */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char *_gc_label(const E_Gadcon_Client_Class *client_class);
static const char *_gc_id_new(const E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);

static void _oconnection_conf_new(void);
static void _oconnection_conf_free(void);
Eina_Bool _oconnection_conf_timer(void *data);
static Config_Item *_oconnection_conf_item_get(const char *id);
static void _oconnection_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event);
// callback for popup menu items
static void _oconnection_cb_mouse_down_popup_item(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _oconnection_cb_mouse_down_popup_item_ask_ssid_wpa(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _oconnection_cb_mouse_down_popup_item_ask_ssid_wep(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _oconnection_cb_mouse_down_popup_item_ask_ssid_only(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _oconnection_popup_new(Instance *inst);
static void _oconnection_popup_show(Instance *inst);
static void _oconnection_popup_populate(Instance *inst);
static void _oconnection_popup_del(Instance *inst);
static Eina_Bool _oconnection_popup_del_idler_cb(void *data);
static void _oconnection_popup_input_create(Instance *inst, Ecore_X_Window sibling);
static void _oconnection_popup_input_destroy(Instance *inst);
static void _oconnection_popup_position(Instance *inst, Evas_Coord ww, Evas_Coord hh);
static void _oconnection_popup_size_calc(Instance *inst, Evas_Coord *ww, Evas_Coord *hh);

static int _oconnection_scan_compare(const void *data1, const void *data2);
static void _oconnection_popup_populate_info(Instance *inst);
static void _oconnection_popup_populate_info_all(Instance *inst);
static void _oconnection_popup_populate_info_network(Instance *inst);
static void _oconnection_popup_populate_info_scan(Instance *inst, int max);
static void _oconnection_popup_populate_ask_ssid(Instance *inst);
static void _oconnection_popup_populate_ask_key(Instance *inst);
static void _oconnection_popup_populate_reask_key(Instance *inst);
static void _oconnection_popup_populate_dhcp(Instance *inst);
static void _oconnection_popup_populate_connect(Instance *inst);
static void _oconnection_popup_populate_status(Instance *inst);
static Eina_Bool _oconnection_popup_status_end(void *data);

static void _oconnection_popup_button_callback_ask_key(void *data, void *arg2);

Eina_Bool _oconnection_popup_input_mouse_down_cb(void *data, int type, void *event);
Eina_Bool _oconnection_popup_input_key_down_cb(void *data, int type, void *event);

static void _oconnection_end_populate_cb(void *data, Evas* evas, Evas_Object *obj, void *event);
static Evas_Object* _oconnection_popup_info_label_new (Instance *inst, Evas *evas, const char *label );
Eina_Bool print_connection_popup(void *data);
static void _oconnection_popup_item_new (Instance *inst, Evas *evas, Oconnection_Item *oi);
static void _oconnection_popup_item_hidden_ssid_new (Instance *inst, Evas *evas, const char *label, Oconnection_Network_Flags key_type);

static void _oconnection_popup_network_fill(Instance *inst);
static void _oconnection_popup_network_clean(Instance *inst);

static void _oconnection_popup_item_security_set(Oconnection_Item *oi, Eina_Bool sec);
static void _oconnection_popup_item_level_set(Oconnection_Item *oi, const char *lvl);
static void _oconnection_popup_info_label_wired_network_update(Instance *inst, Oconnection_Wired_Status *ows);
static void _oconnection_popup_info_label_wifi_network_update(Instance *inst, Oconnection_Wifi_Status *ows);

static Eina_Bool _e_mod_oconnection_scan_results_cb(void *data, int type, void *event);
static Eina_Bool _e_mod_oconnection_iface_update_cb(void *data, int type, void *event);

/* Local Variables */
static int uuid = 0;
static Ecore_Event_Handler *oconnection_scan_handler;
static Ecore_Event_Handler *oconnection_iface_handler;
static Eina_List *instances = NULL;
static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;
Config *oconnection_conf = NULL;

static const E_Gadcon_Client_Class _gc_class =
{
  GADCON_CLIENT_CLASS_VERSION, "oconnection",
  {_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon,
    _gc_id_new, NULL, NULL},
  E_GADCON_CLIENT_STYLE_PLAIN
};

/* We set the version and the name, check e_mod_main.h for more details */
EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, "Oconnection"};

/*
 * Module Functions
 */

/* Function called when the module is initialized */
EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[PATH_MAX];

   /* Location of message catalogs for localization */
   snprintf(buf, sizeof(buf), "%s/locale", e_module_dir_get(m));
   bindtextdomain(PACKAGE, buf);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   /* Location of theme to load for this module */
   snprintf(buf, sizeof(buf), "%s/e-module-oconnection.edj", m->dir);


   /* Display this Modules config info in the main Config Panel */

   /* starts with a category, create it if not already exists */
   e_configure_registry_category_add("advanced", 80, "Advanced",
                                     NULL, "preferences-advanced");
   /* add right-side item */
   e_configure_registry_item_add("advanced/oconnection", 110, D_("Oconnection"),
                                 NULL, buf, e_int_config_oconnection_module);

   /* Define EET Data Storage for the config file */
   conf_item_edd = E_CONFIG_DD_NEW("Config_Item", Config_Item);
#undef T
#undef D
#define T Config_Item
#define D conf_item_edd
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, switch2, INT);

   conf_edd = E_CONFIG_DD_NEW("Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, switch1, UCHAR); /* our var from header */
   E_CONFIG_LIST(D, T, conf_items, conf_item_edd); /* the list */

   /* Tell E to find any existing module data. First run ? */
   oconnection_conf = e_config_domain_load("module.oconnection", conf_edd);
   if (oconnection_conf)
     {
        /* Check config version */
        if ((oconnection_conf->version >> 16) < MOD_CONFIG_FILE_EPOCH)
          {
             /* config too old */
             _oconnection_conf_free();
             ecore_timer_add(1.0, _oconnection_conf_timer,
                             D_("Oconnection Module Configuration data needed "
                                "upgrading. Your old configuration<br> has been"
                                " wiped and a new set of defaults initialized. "
                                "This<br>will happen regularly during "
                                "development, so don't report a<br>bug. "
                                "This simply means the module needs "
                                "new configuration<br>data by default for "
                                "usable functionality that your old<br>"
                                "configuration simply lacks. This new set of "
                                "defaults will fix<br>that by adding it in. "
                                "You can re-configure things now to your<br>"
                                "liking. Sorry for the inconvenience.<br>"));
          }

    /* Ardvarks */
    else if (oconnection_conf->version > MOD_CONFIG_FILE_VERSION)
    {
      /* config too new...wtf ? */
      _oconnection_conf_free();
      ecore_timer_add(1.0, _oconnection_conf_timer,
                      D_("Your Oconnection Module configuration is NEWER "
                         "than the module version. This is "
                         "very<br>strange. This should not happen unless"
                         " you downgraded<br>the module or "
                         "copied the configuration from a place where"
                         "<br>a newer version of the module "
                         "was running. This is bad and<br>as a "
                         "precaution your configuration has been now "
                         "restored to<br>defaults. Sorry for the "
                         "inconvenience.<br>"));
    }
  }

  /* if we don't have a config yet, or it got erased above,
   * then create a default one */
  if (!oconnection_conf) _oconnection_conf_new();

  /* create a link from the modules config to the module
   * this is not written */
  oconnection_conf->module = m;

  /* Tell any gadget containers (shelves, etc) that we provide a module
   * for the user to enjoy */
  e_gadcon_provider_register(&_gc_class);

  oconnection_init();
  oconnection_scan_handler =
     ecore_event_handler_add(OCONNECTION_EVENT_SCAN_RESULTS,
                             _e_mod_oconnection_scan_results_cb, NULL);
  oconnection_iface_handler =
     ecore_event_handler_add(OCONNECTION_EVENT_IFACE_UPDATE,
                             _e_mod_oconnection_iface_update_cb, NULL);

  /* Give E the module */
  return m;
}

/*
 * Function to unload the module
 */
EAPI int
e_modapi_shutdown(E_Module *m)
{
   ecore_event_handler_del(oconnection_scan_handler);
   ecore_event_handler_del(oconnection_iface_handler);
   oconnection_shutdown();
   /* Unregister the config dialog from the main panel */
   e_configure_registry_item_del("advanced/oconnection");

   /* Remove the config panel category if we can. E will tell us.
      category stays if other items using it */
   e_configure_registry_category_del("advanced");

   /* Kill the config dialog */
   if (oconnection_conf->cfd) e_object_del(E_OBJECT(oconnection_conf->cfd));
   oconnection_conf->cfd = NULL;

   /* Tell E the module is now unloaded. Gets removed from shelves, etc. */
   oconnection_conf->module = NULL;
   e_gadcon_provider_unregister(&_gc_class);

   /* Cleanup our item list */
   while (oconnection_conf->conf_items)
     {
        Config_Item *ci = NULL;

        /* Grab an item from the list */
        ci = oconnection_conf->conf_items->data;

        /* remove it */
        oconnection_conf->conf_items =
           eina_list_remove_list(oconnection_conf->conf_items,
                                 oconnection_conf->conf_items);

        /* cleanup stringshares */
        if (ci->id) eina_stringshare_del(ci->id);

        /* keep the planet green */
        E_FREE(ci);
     }

   /* Cleanup the main config structure */
   E_FREE(oconnection_conf);

   /* Clean EET */
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

/*
 * Function to Save the modules config
 */
EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.oconnection", conf_edd, oconnection_conf);
   return 1;
}

/* Local Functions */


/* Called when Gadget Controller (gadcon) says to appear in scene */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Instance *inst = NULL;
   char buf[PATH_MAX];

   /* theme file */
   snprintf(buf, sizeof(buf), "%s/e-module-oconnection.edj",
            oconnection_conf->module->dir);

   /* New visual instance, any config ? */
   inst = E_NEW(Instance, 1);
   inst->conf_item = _oconnection_conf_item_get(id);

   /* create on-screen object */
   inst->o_oconnection = edje_object_add(gc->evas);
   /* we have a theme ? */
   if (!e_theme_edje_object_set(inst->o_oconnection,
                                "base/theme/modules/oconnection",
                                "modules/oconnection/status_icon_swallow"))
     edje_object_file_set(inst->o_oconnection, buf,
                          "modules/oconnection/status_icon_swallow");

   // create and set the wifi icon
   inst->wifi_icon = edje_object_add(gc->evas);
   edje_object_file_set(inst->wifi_icon, buf, "modules/oconnection/wifi");
   edje_object_part_swallow(inst->o_oconnection,"wifi_icon",inst->wifi_icon);

   // create and set the wired network icon
   inst->wired_icon = edje_object_add(gc->evas);
   edje_object_file_set(inst->wired_icon, buf, "modules/oconnection/wired");
   edje_object_part_swallow(inst->o_oconnection,"wired_icon",inst->wired_icon);

   // create and set the no_connection icon
   inst->no_connection_icon = edje_object_add(gc->evas);
   edje_object_file_set(inst->no_connection_icon, buf,
                        "modules/oconnection/no_connection");
   edje_object_part_swallow(inst->o_oconnection, "no_connection_icon",
                            inst->no_connection_icon);
   edje_object_signal_emit(inst->o_oconnection,
                           "no_connection_make_transition", "");

   /* Start loading our module on screen via container */
   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_oconnection);
   inst->gcc->data = inst;

   /* hook a mouse down. we want/have a popup menu, right ? */
   evas_object_event_callback_add(inst->o_oconnection, EVAS_CALLBACK_MOUSE_DOWN,
                                  _oconnection_cb_mouse_down, inst);

   /* add to list of running instances so we can cleanup later */
   instances = eina_list_append(instances, inst);

   return inst->gcc;
}

/* Called when Gadget_Container says stop */
static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst = NULL;

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);

   /* delete the visual */
   if (inst->o_oconnection)
     {
        /* remove mouse down callback hook */
        evas_object_event_callback_del(inst->o_oconnection,
                                       EVAS_CALLBACK_MOUSE_DOWN,
                                       _oconnection_cb_mouse_down);
        evas_object_del(inst->o_oconnection);
        evas_object_del(inst->wifi_icon);
        evas_object_del(inst->wired_icon);
        evas_object_del(inst->no_connection_icon);
     }
   E_FREE(inst);
}

/* For when container says we are changing position */
static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

/* Gadget/Module label, name for our module */
static const char *
_gc_label(const E_Gadcon_Client_Class *client_class)
{
   return D_("Oconnection");
}

/* so E can keep a unique instance per-container */
static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class)
{
   Config_Item *ci = NULL;

   ci = _oconnection_conf_item_get(NULL);
   return ci->id;
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas)
{
   Evas_Object *o = NULL;
   char buf[PATH_MAX];

   /* theme */
   snprintf(buf, sizeof(buf), "%s/e-module-oconnection.edj",
            oconnection_conf->module->dir);

   /* create icon object */
   o = edje_object_add(evas);

   /* load icon from theme */
   edje_object_file_set(o, buf, "icon");

   return o;
}

/* new module needs a new config :), or config too old and we need one anyway */
static void
_oconnection_conf_new(void)
{
   oconnection_conf = E_NEW(Config, 1);
   oconnection_conf->version = (MOD_CONFIG_FILE_EPOCH << 16);

#define IFMODCFG(v) if ((oconnection_conf->version & 0xffff) < v) {
#define IFMODCFGEND }

   /* setup defaults */
   IFMODCFG(0x008d);
   oconnection_conf->switch1 = 1;
   _oconnection_conf_item_get(NULL);
   IFMODCFGEND;

   /* update the version */
   oconnection_conf->version = MOD_CONFIG_FILE_VERSION;

   /* setup limits on the config properties here (if needed) */

   /* save the config to disk */
   e_config_save_queue();
}

/* This is called when we need to cleanup the actual configuration,
 * for example when our configuration is too old */
static void
_oconnection_conf_free(void)
{
   /* cleanup any stringshares here */
   while (oconnection_conf->conf_items)
     {
        Config_Item *ci = NULL;

        ci = oconnection_conf->conf_items->data;
        oconnection_conf->conf_items =
           eina_list_remove_list(oconnection_conf->conf_items,
                                 oconnection_conf->conf_items);
        /* EPA */
        if (ci->id) eina_stringshare_del(ci->id);
        E_FREE(ci);
     }

   E_FREE(oconnection_conf);
}

/* timer for the config oops dialog (old configuration needs update) */
Eina_Bool
_oconnection_conf_timer(void *data)
{
   e_util_dialog_show(D_("Oconnection Configuration need to be updated %s"),
                      data);
   return 0;
}

/* function to search for any Config_Item struct for this Item
 * create if needed */
static Config_Item *
_oconnection_conf_item_get(const char *id)
{
   Eina_List *l = NULL;
   Config_Item *ci = NULL;
   char buf[128];

   if (!id)
     {
        /* nothing passed, return a new id */
        snprintf(buf, sizeof(buf), "%s.%d", _gc_class.name, ++uuid);
        id = buf;
     }
   else
     {
        uuid++;
        for (l = oconnection_conf->conf_items; l; l = l->next)
          {
             if (!(ci = l->data)) continue;
             if ((ci->id) && (!strcmp(ci->id, id))) return ci;
          }
     }
   ci = E_NEW(Config_Item, 1);
   ci->id = eina_stringshare_add(id);
   ci->switch2 = 0;
   oconnection_conf->conf_items =
      eina_list_append(oconnection_conf->conf_items, ci);
   return ci;
}

/*
 * Function called when the module icon is clicked
 */

static void
_oconnection_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Instance *inst = NULL;
   inst = data;
   if (!inst) return;
   inst->ui.step = OCONNECTION_STEP_INFO;
   if (!inst->popup)
     {
        _oconnection_popup_network_fill(inst);
        _oconnection_popup_new(inst);
        _oconnection_popup_populate(inst);
        _oconnection_popup_show(inst);
     }

   else
     _oconnection_popup_del(inst);
}

/* function to destroy the input layer used to catch the keyboard events */
static void
_oconnection_popup_input_destroy(Instance *inst)
{
   ODBG("delete input window\n");
   ecore_x_window_delete_request_send(inst->ui.input.win);
   ecore_x_window_free(inst->ui.input.win);
   inst->ui.input.win = 0;
   if (inst->ui.input.border)
     {
        inst->ui.input.border->internal_ecore_evas = NULL;
        inst->ui.input.border = NULL;
     }
   if(inst->ui.input.mouse_up != NULL)
     {
        ecore_event_handler_del(inst->ui.input.mouse_up);
        inst->ui.input.mouse_up = NULL;
     }

   if(inst->ui.input.key_down != NULL)
     {
        ecore_event_handler_del(inst->ui.input.key_down);
        inst->ui.input.key_down = NULL;
     }
   ecore_x_window_focus(inst->ui.input.last_focused);
}

/* function to create the input layer used to catch the keyboard events */
static void
_oconnection_popup_input_create(Instance *inst, Ecore_X_Window sibling)
{
   Ecore_X_Window_Configure_Mask mask;
   Ecore_X_Window w;
   E_Manager *man;

   ODBG("create input window\n");
   man = e_manager_current_get();
   w = ecore_x_window_input_new(man->root, 0, 0, man->w, man->h);
   mask = (ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE |
           ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING);
   ecore_x_window_configure(w, mask, 0, 0, man->w, man->h, 0, sibling,
                            ECORE_X_WINDOW_STACK_BELOW);
   ecore_x_window_show(w);
   inst->ui.input.last_focused = ecore_x_window_focus_get();
   ecore_x_window_focus(w);

   inst->ui.input.mouse_up =
      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                              _oconnection_popup_input_mouse_down_cb, inst);

   inst->ui.input.key_down =
      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                              _oconnection_popup_input_key_down_cb, inst);

   inst->ui.input.win = w;
}

static Eina_Bool
_next_step(void *data)
{
   Instance *inst = data;
   inst->ui.popup_timer = NULL;
   _oconnection_popup_new(inst);
   _oconnection_popup_populate(inst);
   _oconnection_popup_show(inst);
   return ECORE_CALLBACK_CANCEL;
}

/* function freeing all the data structures of the popup, then deleting it */
static Eina_Bool
_oconnection_popup_del_idler_cb(void *data)
{
   Instance *inst;
   inst = data;
   if (!inst) return ECORE_CALLBACK_CANCEL;
   if (inst->popup) e_object_del(E_OBJECT(inst->popup));
   _oconnection_popup_input_destroy(inst);
   if(inst->ui.items != NULL)
     eina_list_free(inst->ui.items);
   if(inst->ui.items_more != NULL)
     eina_list_free(inst->ui.items_more);
   if(inst->ui.items_info != NULL)
     eina_list_free(inst->ui.items_info);
   if(inst->ui.items_space != NULL)
     eina_list_free(inst->ui.items_space);
   if(inst->ui.items_entry != NULL)
     eina_list_free(inst->ui.items_entry);
   if(inst->ui.items_button != NULL)
     eina_list_free(inst->ui.items_button);
   inst->ui.items = NULL;
   inst->ui.items_more = NULL;
   inst->ui.items_space = NULL;
   inst->ui.items_entry = NULL;
   inst->ui.items_button = NULL;
   inst->ui.items_info = NULL;
   inst->ui.list = NULL;
   inst->ui.items_w = 0;
   if (inst->ui.items_h)
     inst->ui.last_item_h = inst->ui.items_h;
   inst->ui.items_h = 0;
   inst->ui.items_more_h = 0;
   inst->ui.items_more_w = 0;
   inst->ui.label_w = 0 ;
   inst->ui.label_h = 0 ;
   inst->ui.info_w = 0 ;
   inst->ui.info_h = 0 ;
   inst->ui.pct_label_w = 0 ;
   inst->ui.pct_label_h = 0 ;
   inst->ui.entry_w = 0;
   inst->ui.entry_h = 0;
   inst->ui.button_box = NULL;
   inst->popup = NULL;
   if (inst->ui.step != OCONNECTION_STEP_INFO_ALL)
     _oconnection_popup_network_clean(inst);

   if (inst->ui.popup_timer)
     ecore_timer_del(inst->ui.popup_timer);
   if (inst->ui.step != OCONNECTION_STEP_INFO)
     // Hum really bad hack
     inst->ui.popup_timer = ecore_timer_add(0.5, _next_step, inst);
   else
     inst->ui.popup_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_oconnection_popup_del(Instance *inst) {
     ecore_idler_add(_oconnection_popup_del_idler_cb, inst);
}

/* function creating the popup, it is then filled with the populate function */
static void
_oconnection_popup_new(Instance *inst) {
     Evas_Object *o, *ow;
     Evas *evas;

     inst->popup = e_gadcon_popup_new(inst->gcc);
     evas = inst->popup->win->evas;

     ow = e_widget_add(evas);
     inst->ui.list = ow;

     o = e_box_add(evas);
     inst->ui.box = o;
     evas_object_show(o);
     e_widget_sub_object_add(ow, o);
     e_widget_resize_object_set(ow, o);

     e_box_orientation_set(o, EINA_FALSE);
}

/* function to show the popup */
static void
_oconnection_popup_show(Instance *inst)
{
   Eina_List *l, *lbd;
   Eina_Bool catch_keys;
   E_Border *bd;
   Evas_Coord ww, hh;
   Ecore_X_Window sibling;

   evas_object_show(inst->ui.box);

   _oconnection_popup_size_calc(inst, &ww, &hh);
   e_popup_show(inst->popup->win);

   if ((inst->ui.step == OCONNECTION_STEP_SSID)
       || (((inst->ui.step == OCONNECTION_STEP_KEY)
            || (inst->ui.step == OCONNECTION_STEP_KEY2))
           && (inst->ui.key_type)))
     catch_keys = EINA_TRUE;
   else
     catch_keys = EINA_FALSE;

   if (catch_keys)
     {
        catch_keys = EINA_FALSE;
        lbd = e_border_client_list();
        EINA_LIST_FOREACH(lbd, l, bd)
          {
             ODBG("search for catch %s %d\n",
                  bd->client.icccm.title, bd->client.vkbd.vkbd);
             /* explicit hint that its a virtual keyboard */
             if ((bd->client.vkbd.vkbd)
                 ||
                 /* legacy */
                 ( /* trap the matchbox qwerty and multitap kbd's */
                    (((bd->client.icccm.title)
                      && (!strcmp(bd->client.icccm.title, "Keyboard")))
                     ||
                     ((bd->client.icccm.name)
                      && ((!strcmp(bd->client.icccm.name, "multitap-pad")))))
                    && (bd->client.netwm.state.skip_taskbar)
                    && (bd->client.netwm.state.skip_pager)))
               {
                  sibling = bd->win;
                  catch_keys = EINA_TRUE;
                  break;
               }
          }
     }
   ODBG("catch key %d\n", catch_keys);

   if (catch_keys)
     {
        E_Zone *zone = inst->gcc->gadcon->zone;
        E_Border *bd;
        ecore_x_e_virtual_keyboard_state_set(inst->popup->win->evas_win,
                                             ECORE_X_VIRTUAL_KEYBOARD_STATE_ON);
        ecore_x_icccm_size_pos_hints_set(inst->popup->win->evas_win, EINA_TRUE,
                                         ECORE_X_GRAVITY_NW,
                                         inst->popup->w, inst->popup->h, //min
                                         inst->popup->w, inst->popup->h, //max
                                         inst->popup->w, inst->popup->h, //base
                                         1, 1, //step
                                         0.0, 0.0);
        bd = e_border_new(zone->container,
                          inst->popup->win->evas_win,
                          0, 1);
        bd->internal_ecore_evas = inst->popup->win->ecore_evas;
        bd->internal_no_remember = 1;
        bd->ignore_first_unmap = 1;
        bd->shaded = 0;
        bd->shading = 0;
        e_border_show(bd);
        inst->ui.input.border = bd;
     }
   else
     {
        _oconnection_popup_position(inst, ww, hh);
        sibling = inst->popup->win->evas_win;
     }
   _oconnection_popup_input_create(inst, sibling);
}



/* callback for the mouse up event on the input layer */
Eina_Bool
_oconnection_popup_input_mouse_down_cb(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   Instance *inst = data;
   Ecore_X_Window target;
   int x, y;

   if (ev->window != inst->ui.input.win)
     return ECORE_CALLBACK_PASS_ON;
   ODBG("mouse down input popup %d %d\n", ev->x, ev->y);
   if (inst->ui.input.border)
     {
        x = ev->x - inst->ui.input.border->x;
        y = ev->y - inst->ui.input.border->y;
        ODBG("I have input border %d %d %d %d\n", x, y,
             inst->ui.input.border->w, inst->ui.input.border->h);

        if ((x > 0) && (x < inst->ui.input.border->w)
            && (y > 0) && (y < inst->ui.input.border->h))
          {
             target = inst->ui.input.border->client.win;

             //        ecore_x_mouse_in_send(target, x, y);
             ecore_x_mouse_move_send(target, x, y);
             ecore_x_mouse_down_send(target, x, y, 1);
             ecore_x_mouse_up_send(target, x, y, 1);
             //        ecore_x_mouse_out_send(target, x, y);
             return ECORE_CALLBACK_DONE;
          }
     }
   inst->ui.step = OCONNECTION_STEP_INFO;
   _oconnection_popup_del(inst);

   return ECORE_CALLBACK_DONE;
}

Eina_Bool
_oconnection_popup_input_key_down_cb(void *data, int type, void *event)
{
   Ecore_Event_Key *ev = event;
   Instance *inst = data;
   const char *keysym;

   keysym = ev->key;
#ifdef DEBUG
   printf("keyname: %s\nkey: %s\nstring: %s\ncompose: %s\nmodifiers: %d\n",
          ev->keyname, ev->key, ev->string, ev->compose, ev->modifiers);
#endif /* DEBUG */
   if (!strcmp(keysym, "Escape"))
     {
        inst->ui.step = OCONNECTION_STEP_INFO;
        _oconnection_popup_del(inst);
     }
   else
     {
        if (!strcmp(keysym, "v") && (ev->modifiers == ECORE_CTRL))
          ecore_x_selection_clipboard_request(
             inst->ui.input.win, ECORE_X_SELECTION_TARGET_UTF8_STRING);
        else if (!strcmp(keysym, "Return"))
          {
             if (inst->ui.items_button)
               {
                  Evas_Coord x, y, w, h;
                  evas_object_geometry_get(inst->ui.items_button->data,
                                           &x, &y, &w, &h);
                  evas_event_feed_mouse_move(inst->popup->win->evas,
                                             x + w + (w / 2), y + (h / 2),
                                             ev->timestamp, NULL);
                  evas_event_feed_mouse_down(inst->popup->win->evas,
                                             1, EVAS_BUTTON_NONE,
                                             ev->timestamp,  NULL);
                  evas_event_feed_mouse_up(inst->popup->win->evas,
                                           1, EVAS_BUTTON_NONE,
                                           ev->timestamp,  NULL);
               }
          }
        else if (inst->ui.input.border)
          return 1;

        else
          {
             evas_event_feed_key_down(inst->popup->win->evas, ev->keyname,
                                      ev->key, ev->string,
                                      ev->compose, ev->timestamp, NULL);
             evas_event_feed_key_up(inst->popup->win->evas, ev->keyname,
                                    ev->key, ev->string, ev->compose,
                                    ev->timestamp, NULL);
          }
     }
   return 1;
}


/* function to create a new entry item (text field) */
static Evas_Object *
_oconnection_popup_item_entry_new(Instance *inst, Evas *evas, char** textfield)
{
  Evas_Object *entry;
  Evas_Coord w, h;

  entry = e_widget_entry_add(evas, textfield, NULL, NULL,NULL );

  e_widget_size_min_get(entry, NULL, &h);
  w = 350;
  if (h < 30) h = 36;
  if (inst->ui.entry_w < w) inst->ui.entry_w = w;
  if (inst->ui.entry_h < h) inst->ui.entry_h = h;
  e_widget_size_min_set(entry, inst->ui.entry_w, inst->ui.entry_h);

  e_box_pack_end(inst->ui.box, entry);
  e_box_pack_options_set(entry,
                         1, 1, /* fill */
                         1, 1, /* expand */
                         0.5, 0.5, /* align */
                         inst->ui.entry_w, inst->ui.entry_h, /* min */
                         -1, -1 /* max */
                        );

  inst->ui.items_entry = eina_list_append(inst->ui.items_entry, entry);
  e_widget_sub_object_add(inst->ui.list, entry);
  e_widget_focus_set(entry, 1);
  evas_object_show(entry);

  return entry;
}

/* callback for the connection / get SSID buttons
 *
 *  its behaviour is determined by the ssid of the network.
 *  if it is not a public SSID, then the button should copy what's in
 *  the text field into the ssid field of the network_properties structure
 */
static void
_oconnection_popup_button_callback_dhcp_cb(void *data, Eina_Bool status)
{
   Instance *inst;
   inst = data;
   if (!inst) return;
   if (inst->ui.step == OCONNECTION_STEP_CONNECT)
     {
        if (status)
          {
             ODBG("Step Ipconfig\n");
             inst->ui.step = OCONNECTION_STEP_IPCONFIG;
             inst->ui.associated = EINA_TRUE;
          }
        else
          {
             ODBG("Step Key2\n");
             inst->ui.step = OCONNECTION_STEP_KEY2;
          }
        _oconnection_popup_del(inst);
     }
}

static void
_oconnection_popup_button_callback_connect_cb(void *data, Eina_Bool status)
{
   Instance *inst;
   inst = data;
   if (!inst) return;
   if (inst->ui.step == OCONNECTION_STEP_IPCONFIG)
     {
        inst->ui.step = OCONNECTION_STEP_STATUS;
        inst->ui.connected = !!status;
        _oconnection_popup_del(inst);
     }
}

static void
_oconnection_popup_button_callback_connect(void* data, void* arg2)
{
   Instance *inst;

   inst = data;
   if (!inst) return;
   inst->ui.associated = EINA_FALSE;
   inst->ui.step = OCONNECTION_STEP_CONNECT;
   ODBG("connexion a %s avec %s \n", inst->ui.ssid, inst->ui.key);
   oconnection_wifi_connect(
      inst->ui.ow,
      inst->ui.ssid,
      inst->ui.key,
      inst->ui.key_type,
      _oconnection_popup_button_callback_dhcp_cb,
      _oconnection_popup_button_callback_connect_cb,
      inst);
   _oconnection_popup_del(inst);
}

static void
_oconnection_popup_button_callback_cancel(void *data, void *arg2)
{
   Instance *inst;
   inst = data;
   if (!inst) return;
   inst->ui.step = OCONNECTION_STEP_INFO;
   _oconnection_popup_del(inst);
}

static void
_oconnection_popup_button_callback_ask_key(void *data, void *arg2)
{
   // in case we are getting the ssid (hidden network)
   Instance *inst;
   inst = data;
   if (!data) return;
   inst->ui.step = OCONNECTION_STEP_KEY;
   _oconnection_popup_del(inst);
}

/* function to create a new button */
static Evas_Object *
_oconnection_popup_item_button_new(Instance *inst, Evas *evas, char* label, void *data, void (*func_cb)(void *data, void *data2))
{
  Evas_Object *button;
  Evas_Coord w, h;

  if (!inst->ui.button_box)
    {
       inst->ui.button_box = e_box_add(evas);
       e_box_orientation_set(inst->ui.button_box, 1);
       e_box_pack_end(inst->ui.box, inst->ui.button_box);
       evas_object_show(inst->ui.button_box);
    }

  button = e_widget_button_add(evas, label, NULL, func_cb, data, NULL);

  e_widget_size_min_get(button, &w, &h);
  if (inst->ui.button_w < w) inst->ui.button_w = w;
  if (inst->ui.button_h < h) inst->ui.button_h = h;
  e_widget_size_min_set(button, inst->ui.button_w, inst->ui.button_h);

  e_box_pack_end(inst->ui.button_box, button);
  e_box_pack_options_set(button,
                         1, 1, /* fill */
                         1, 1, /* expand */
                         0.5, 0.5, /* align */
                         inst->ui.button_w, inst->ui.button_h, /* min */
                         -1, inst->ui.button_h /* max */
                        );

  inst->ui.items_button = eina_list_append(inst->ui.items_button, button);
  e_widget_sub_object_add(inst->ui.list, button);
  evas_object_show(button);
  e_box_pack_options_set(inst->ui.button_box,
                         1, 1,
                         1, 1,
                         0.5, 0.5,
                         1, inst->ui.button_h + 10,
                         -1, inst->ui.button_h + 10);
  return button;
}

/* function to add an item (text) to the _connection_ popup (the one with the entry)  */
static Evas_Object *
_oconnection_popup_info_label_new(Instance *inst, Evas *evas, const char *label)
{
  Evas_Object *lbl;
  Evas_Coord w, h;

  char buf[PATH_MAX];
  snprintf(buf, sizeof(buf), "%s/e-module-oconnection.edj",
           oconnection_conf->module->dir);
  lbl = edje_object_add(evas);
  edje_object_file_set(lbl, buf, "modules/oconnection/unclickable_item");
  if (label)
    edje_object_part_text_set(lbl, "label", label);

  evas_object_show(lbl);

  edje_object_size_min_calc(lbl, &w, &h);
  if (inst->ui.info_w < w) inst->ui.info_w = w;
  if (inst->ui.info_h < h) inst->ui.info_h = h;
  evas_object_resize(lbl,w,h);


  e_box_pack_end(inst->ui.box, lbl);
  e_box_pack_options_set(lbl,
                         1, 1, /* fill */
                         1, 0, /* expand */
                         0.5, 0.5, /* align */
                         w, h, /* min */
                         -1, -1 /* max */
                        );

  inst->ui.items_info = eina_list_append(inst->ui.items_info, lbl);
  e_widget_sub_object_add(inst->ui.list, lbl);
  return lbl;
}

/* function to add the "plus de résultats" button to the popup */
static Evas_Object *
_oconnection_popup_item_expand_results_new (Instance *inst, Evas *evas, const char *label) {
  Evas_Object *wid;
  Evas_Object *box;
  Evas_Object *lbl;
  Evas_Object *edj;
  Evas_Coord ww, wh, w, h;
  char buf[PATH_MAX];

  snprintf(buf, sizeof(buf), "%s/e-module-oconnection.edj",
           oconnection_conf->module->dir);


  wid = e_widget_add(evas);

  box = e_box_add(evas);
  e_box_orientation_set(box, 1);
  evas_object_show(box);
  e_widget_sub_object_add(wid, box);
  e_widget_resize_object_set(wid, box);

  lbl = edje_object_add(evas);

  edje_object_file_set(lbl, buf, "modules/oconnection/label");
  edje_object_part_text_set(lbl, "label", label);

  evas_object_show(lbl);

  edje_object_size_min_calc(lbl, &w, &h);
  evas_object_resize(lbl, w, h);

  e_box_pack_end(box, lbl);
  e_box_pack_options_set(lbl,
                         1, 1, /* fill */
                         1, 0, /* expand */
                         1.0, 0.5, /* align */
                         w, h, /* min */
                         -1, h /* max */
                        );
  e_widget_sub_object_add(wid, lbl);

  edj = edje_object_add(evas);


  evas_object_event_callback_add(edj, EVAS_CALLBACK_MOUSE_DOWN,
                                 _oconnection_end_populate_cb, inst);

  edje_object_file_set(edj, buf, "modules/oconnection/item");
  edje_object_part_swallow(edj, "e.swallow.content", wid);
  evas_object_show(edj);

  edje_object_size_min_calc(edj, &ww, &wh);
  w += ww;
  h += wh;
  if (inst->ui.items_more_w < w) inst->ui.items_more_w = w;
  if (inst->ui.items_more_h < h) inst->ui.items_more_h = h;
  evas_object_resize(edj, inst->ui.items_more_w, inst->ui.items_more_h);

  e_box_pack_end(inst->ui.box, edj);
  e_box_pack_options_set(edj,
                         1, 1, /* fill */
                         1, 0, /* expand */
                         0.5, 0.5, /* align */
                         inst->ui.items_more_w, inst->ui.items_more_h, /* min */
                         -1, inst->ui.items_more_h /* max */
                        );

  inst->ui.items_more = eina_list_append(inst->ui.items_more, wid);

  e_widget_sub_object_add(inst->ui.list, wid);
  return wid;
}

/* function to add a spacer to the popup */
static Evas_Object *
_oconnection_popup_spacer_new(Instance *inst, Evas *evas) {
  Evas_Object *edj;
  Evas_Coord w, h;
  char buf[PATH_MAX];

  snprintf(buf, sizeof(buf), "%s/e-module-oconnection.edj",
           oconnection_conf->module->dir);

  edj = edje_object_add(evas);
  edje_object_file_set(edj, buf, "modules/oconnection/item/space");
  evas_object_show(edj);

  edje_object_size_min_calc(edj, &w, &h);
  if (inst->ui.space_w < w) inst->ui.space_w = w;
  if (inst->ui.space_h < h) inst->ui.space_h = h;
  evas_object_resize(edj, inst->ui.space_w, inst->ui.space_h);

  e_box_pack_end(inst->ui.box, edj);
  e_box_pack_options_set(edj,
                         1, 1, /* fill */
                         1, 0, /* expand */
                         0.5, 0.5, /* align */
                         inst->ui.space_w, inst->ui.space_h, /* min */
                         -1, inst->ui.space_h /* max */
                        );
  inst->ui.items_space = eina_list_append(inst->ui.items_space, edj);
  e_widget_sub_object_add(inst->ui.list, edj);

  return edj;
}

static void
_oconnection_popup_item_hidden_ssid_new(Instance *inst, Evas *evas, const char *label, Oconnection_Network_Flags key_type)
{
   Evas_Object *wid;
   Evas_Object *box;
   Evas_Object *lbl;
   Evas_Object *edj;
   Evas_Coord ww, wh, w, h;
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/e-module-oconnection.edj",
            oconnection_conf->module->dir);
   wid = e_widget_add(evas);

   box = e_box_add(evas);
   e_box_orientation_set(box, 1);
   evas_object_show(box);
   e_widget_sub_object_add(wid, box);
   e_widget_resize_object_set(wid, box);

   lbl = edje_object_add(evas);
   edje_object_file_set(lbl, buf, "modules/oconnection/label");
   edje_object_part_text_set(lbl, "label", label);
   evas_object_show(lbl);
   edje_object_size_min_calc(lbl, &w, &h);
   evas_object_resize(lbl, w, h);
   e_box_pack_end(box, lbl);
   e_box_pack_options_set(lbl,
                          1, 1, /* fill */
                          1, 0, /* expand */
                          1.0, 0.5, /* align */
                          w, h, /* min */
                          -1, h /* max */
                         );
   e_widget_sub_object_add(wid, lbl);

   edj = edje_object_add(evas);
   edje_object_file_set(edj, buf, "modules/oconnection/item");
   edje_object_part_swallow(edj, "e.swallow.content", wid);
   evas_object_show(edj);
   edje_object_size_min_calc(edj, &ww, &wh);
   w += ww;
   h += wh;
   if (inst->ui.items_more_w < w) inst->ui.items_more_w = w;
   if (inst->ui.items_more_h < h) inst->ui.items_more_h = h;
   evas_object_resize(edj, inst->ui.items_more_w, inst->ui.items_more_h);
   e_box_pack_end(inst->ui.box, edj);
   e_box_pack_options_set(edj,
                          1, 1, /* fill */
                          1, 0, /* expand */
                          0.5, 0.5, /* align */
                          inst->ui.items_more_w,
                          inst->ui.items_more_h, /* min */
                          -1, -1 /* max */
                         );
   if (key_type & NW_FLAG_WPA)
     evas_object_event_callback_add(
        edj, EVAS_CALLBACK_MOUSE_DOWN,
        _oconnection_cb_mouse_down_popup_item_ask_ssid_wpa, inst);
   else if (key_type & NW_FLAG_WEP)
     evas_object_event_callback_add(
        edj, EVAS_CALLBACK_MOUSE_DOWN,
        _oconnection_cb_mouse_down_popup_item_ask_ssid_wep, inst);
   else
     evas_object_event_callback_add(
        edj, EVAS_CALLBACK_MOUSE_DOWN,
        _oconnection_cb_mouse_down_popup_item_ask_ssid_only, inst);

   inst->ui.items_more = eina_list_append(inst->ui.items_more, wid);

   e_widget_sub_object_add(inst->ui.list, wid);
}

/* function to add an item to the popup */
static void
_oconnection_popup_item_new(Instance *inst, Evas *evas, Oconnection_Item *oi)
{
  Evas_Object *wid;
  Evas_Object *box;
  Evas_Object *lbl;
  Evas_Object *lbl3;
  Evas_Object *fav;
  Evas_Object *edj;
  Evas_Coord ww, wh, boxw, boxh;
  Evas_Object *edj_padlock_pic;
  Evas_Coord pw,ph;
  char buf[PATH_MAX];

  snprintf(buf, sizeof(buf), "%s/e-module-oconnection.edj",
           oconnection_conf->module->dir);

  // creating the widget
  wid = e_widget_add(evas);

  // creating the box to place the items
  // on a line
  box = e_box_add(evas);
  e_box_orientation_set(box, 1);
  evas_object_show(box);
  e_widget_sub_object_add(wid, box);
  e_widget_resize_object_set(wid, box);

  lbl = edje_object_add(evas);
  edje_object_file_set(lbl, buf, "modules/oconnection/label");
  edje_object_part_text_set(lbl, "label", oi->ssid);
  evas_object_show(lbl);
  edje_object_size_min_calc(lbl, &ww, &wh);
  evas_object_resize(lbl, ww, wh);
  e_box_pack_end(box, lbl);
  e_box_pack_options_set(lbl,
                         1, 1, /* fill */
                         1, 1, /* expand */
                         0.5, 0.5, /* align */
                         ww, wh, /* min */
                         -1, wh /* max */
                        );
  e_widget_sub_object_add(wid, lbl);

  lbl3 = edje_object_add(evas);
  edje_object_file_set(lbl3, buf, "modules/oconnection/label");
  edje_object_part_text_set(lbl3, "label", "<right>100 %%</right>");
  edje_object_size_min_calc(lbl3, &ww, &wh);
  evas_object_resize(lbl3, ww, wh);
  e_box_pack_end(box, lbl3);
  e_box_pack_options_set(lbl3,
                         0, 1, /* fill */
                         0, 1, /* expand */
                         1.0, 0.5, /* align */
                         ww, wh, /* min */
                         ww, wh /* max */
                        );
  e_widget_sub_object_add(wid, lbl3);

  fav = edje_object_add(evas);
  edje_object_file_set(fav, buf, "modules/oconnection/label");
  edje_object_part_text_set(fav, "label", "<center> ♥ </center>");
  edje_object_size_min_calc(fav, &ww, &wh);
  evas_object_resize(fav, ww, wh);
  e_box_pack_end(box, fav);
  e_box_pack_options_set(fav,
                         0, 1, /* fill */
                         0, 1, /* expand */
                         1.0, 0.5, /* align */
                         ww, wh, /* min */
                         ww, wh /* max */
                        );
  e_widget_sub_object_add(wid, fav);

  // print a picture of a padlock
  edj_padlock_pic = edje_object_add(evas);
  edje_object_file_set(edj_padlock_pic, buf, "modules/oconnection/padlock");
  e_box_pack_end(box, edj_padlock_pic);
  e_widget_sub_object_add(wid, edj_padlock_pic);

  edje_object_size_min_calc(edj_padlock_pic,&pw,&ph);
  e_widget_size_min_set(edj_padlock_pic, pw, ph);

  e_box_pack_options_set(edj_padlock_pic,
                         0, 1, /* fill */
                         0, 1, /* expand */
                         1.0, 0.5, /* align */
                         pw, ph, /* min */
                         //-1, inst->ui.space_h /* max */
                         pw, ph /* max */
                        );
  e_widget_sub_object_add(wid, edj_padlock_pic);

  edj = edje_object_add(evas);
  evas_object_event_callback_add(edj, EVAS_CALLBACK_MOUSE_DOWN,
                                 _oconnection_cb_mouse_down_popup_item, oi);

  // swallow the widget in an edje object
  edje_object_file_set(edj, buf, "modules/oconnection/item");
  edje_object_part_swallow(edj, "e.swallow.content", box);
  evas_object_show(edj);

  e_box_size_min_get(box, &boxw, &boxh);
  evas_object_resize(box, boxw, boxh);
  edje_object_size_min_calc(edj, &ww, &wh);
  boxw += ww;
  boxh += wh;
  if (inst->ui.items_w < boxw) inst->ui.items_w = boxw;
  if (inst->ui.items_h < boxh) inst->ui.items_h = boxh;
  evas_object_resize(edj, inst->ui.items_w, inst->ui.items_h);


  e_box_pack_end(inst->ui.box, edj);
  e_box_pack_options_set(edj,
                         1, 1, /* fill */
                         1, 0, /* expand */
                         0.5, 0.5, /* align */
                         inst->ui.items_w, inst->ui.items_h, /* min */
                         -1, inst->ui.items_h /* max */
                        );
  e_widget_sub_object_add(inst->ui.list, wid);
  inst->ui.items = eina_list_append(inst->ui.items, wid);

  oi->o_lbl = lbl;
  oi->o_fav = fav;
  oi->o_sec = edj_padlock_pic;
  oi->o_lvl = lbl3;

}

static void
_oconnection_popup_item_level_set(Oconnection_Item *oi, const char *lvl)
{
   if (oi && oi->o_lvl)
     {
        edje_object_part_text_set(oi->o_lvl, "label", lvl);
        evas_object_show(oi->o_lvl);
     }
}

static void
_oconnection_popup_item_security_set(Oconnection_Item *oi, Eina_Bool sec)
{
   if (oi && oi->o_sec)
     {
        if (sec)
          evas_object_show(oi->o_sec);
        else
          evas_object_hide(oi->o_sec);
     }
}


/* callback for the "Plus de résultats" button on the popup
 * It just recreates the popup with the right argument to populate
 * so that it shows the complete list of networks
 **/
static void _oconnection_end_populate_cb(void *data, Evas* evas, Evas_Object *obj, void *event)
{
  Instance *inst;
  inst = data ;
  if (!inst) return;
  inst->ui.step = OCONNECTION_STEP_INFO_ALL;
  _oconnection_popup_del(inst);
}

/* mouse down callback */
static void
_oconnection_cb_mouse_down_popup_item(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Oconnection_Item *oi;
   oi = data;
   if (!oi || !oi->inst) return;

   oi->inst->ui.ssid = strdup(oi->ssid);
   oi->inst->ui.ow = oi->ow;
   if (oconnection_is_in_spool_get(oi->ssid) && (oi->flags & (NW_FLAG_WPA | NW_FLAG_WEP)))
     {
        ODBG("don't ask again the key\n");
        oi->inst->ui.step = OCONNECTION_STEP_CONNECT;
        oi->inst->ui.key_type = oi->flags;
        oconnection_wifi_connect(
           oi->ow, oi->inst->ui.ssid, NULL, oi->inst->ui.key_type,
           _oconnection_popup_button_callback_dhcp_cb,
           _oconnection_popup_button_callback_connect_cb,
           oi->inst);
     }
   else
     {
        ODBG("we need the key\n");
        oi->inst->ui.step = OCONNECTION_STEP_KEY;
        oi->inst->ui.key_type = oi->flags;
     }
   _oconnection_popup_del(oi->inst);
}

static void
_oconnection_cb_mouse_down_popup_item_ask_ssid_wpa(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Instance *inst;
   inst = data;
   if (!data) return;
   inst->ui.step = OCONNECTION_STEP_SSID;
   inst->ui.key_type = NW_FLAG_WPA;
   _oconnection_popup_del(inst);
}

static void
_oconnection_cb_mouse_down_popup_item_ask_ssid_wep(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Instance *inst;
   inst = data;
   if (!data) return;
   inst->ui.step = OCONNECTION_STEP_SSID;
   inst->ui.key_type = NW_FLAG_WEP;
   _oconnection_popup_del(inst);
}

static void
_oconnection_cb_mouse_down_popup_item_ask_ssid_only(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Instance *inst;
   inst = data;
   if (!data) return;
   inst->ui.step = OCONNECTION_STEP_SSID;
   inst->ui.key_type = NW_FLAG_NONE;
   _oconnection_popup_del(inst);
}

static int
_oconnection_scan_compare(const void *data1, const void *data2)
{
   const Oconnection_Item *oi1, *oi2;
   oi1 = data1;
   oi2 = data2;

   if (oi1->ow->signal < oi2->ow->signal)
     return 1;
   else if (oi1->ow->signal > oi2->ow->signal)
     return -1;
   else
     return 0;
}

static void
_oconnection_popup_network_fill(Instance *inst)
{
   Eina_List *l;
   OWireless_Network *ow;
   Oconnection_Item *oi;
   Oconnection_Wired_Status *ods;
   Oconnection_Wifi_Status *ows;
   OIface_Status *os;

   EINA_LIST_FOREACH(oconnection_iface_wired_get(), l, os)
     {
        ods = calloc(1, sizeof(Oconnection_Wired_Status));
        ods->name = eina_stringshare_add(os->name);
        ods->addr = eina_stringshare_add(os->addr);
        ods->connected = os->connected;
        inst->ui.ifaces_wired = eina_list_append(inst->ui.ifaces_wired, ods);
     }
   EINA_LIST_FOREACH(oconnection_iface_wifi_get(), l, os)
     {
        ows = calloc(1, sizeof(Oconnection_Wifi_Status));
        ows->name = eina_stringshare_add(os->name);
        ows->addr = eina_stringshare_add(os->addr);
        ows->ssid = eina_stringshare_add(os->ssid);
        ows->connected = os->connected;
        inst->ui.ifaces_wifi = eina_list_append(inst->ui.ifaces_wifi, ows);
     }

   if (inst->ui.ifaces_wifi)
     {
        EINA_LIST_FOREACH(oconnection_scan_get(), l, ow)
          {
             oi = calloc(1, sizeof(Oconnection_Item));
             if (oi)
               {
                  oi->ssid = eina_stringshare_add(ow->ssid);
                  oi->bssid = eina_stringshare_add(ow->bssid);
                  oi->flags = ow->flags;
                  oi->inst = inst;
                  oi->ow = ow;
                  inst->ui.oconnection_items =
                     eina_list_sorted_insert(inst->ui.oconnection_items,
                                             _oconnection_scan_compare, oi);
               }
          }
     }
}

static void
_oconnection_popup_network_clean(Instance *inst)
{
   Oconnection_Item *oi;
   Oconnection_Wired_Status *ods;
   Oconnection_Wifi_Status *ows;

   EINA_LIST_FREE(inst->ui.ifaces_wired, ods)
     {
        eina_stringshare_del(ods->name);
        eina_stringshare_del(ods->addr);
        free(ods);
     }
   EINA_LIST_FREE(inst->ui.ifaces_wifi, ows)
     {
        eina_stringshare_del(ows->name);
        eina_stringshare_del(ows->ssid);
        eina_stringshare_del(ows->addr);
        free(ows);
     }
   EINA_LIST_FREE(inst->ui.oconnection_items, oi)
     {
        eina_stringshare_del(oi->ssid);
        eina_stringshare_del(oi->bssid);
        free(oi);
     }
   if (inst->ui.status_timer)
     {
        ecore_timer_del(inst->ui.status_timer);
        inst->ui.status_timer = NULL;
     }
   if (inst->ui.step == OCONNECTION_STEP_INFO)
     {
        if (inst->ui.ssid)
          {
             free(inst->ui.ssid);
             inst->ui.ssid = NULL;
             oconnection_wifi_connect_cancel_callback();
          }
        if (inst->ui.key)
          {
             free(inst->ui.key);
             inst->ui.key = NULL;
          }
     }
}

/* function to fill the popup with items */
static void
_oconnection_popup_populate(Instance *inst)
{
   switch (inst->ui.step)
     {
      case OCONNECTION_STEP_INFO:
         _oconnection_popup_populate_info(inst);
         break;
      case OCONNECTION_STEP_INFO_ALL:
         _oconnection_popup_populate_info_all(inst);
         break;
      case OCONNECTION_STEP_SSID:
         _oconnection_popup_populate_ask_ssid(inst);
         break;
      case OCONNECTION_STEP_KEY:
         _oconnection_popup_populate_ask_key(inst);
         break;
      case OCONNECTION_STEP_KEY2:
         _oconnection_popup_populate_reask_key(inst);
         break;
      case OCONNECTION_STEP_CONNECT:
         _oconnection_popup_populate_connect(inst);
         break;
      case OCONNECTION_STEP_IPCONFIG:
         _oconnection_popup_populate_dhcp(inst);
         break;
      case OCONNECTION_STEP_STATUS:
         _oconnection_popup_populate_status(inst);
         break;
      default:
         printf("Error oconnection is lost in a bad way\n");
     }
}


static void
_oconnection_popup_populate_info(Instance *inst)
{
   _oconnection_popup_populate_info_network(inst);
   inst->ui.nth_item = 0;
   if (inst->ui.ifaces_wifi)
     _oconnection_popup_populate_info_scan(inst, 8);
}

static void
_oconnection_popup_populate_info_all(Instance *inst)
{
   Evas_Coord gy;
   int count, max;
   e_gadcon_client_geometry_get(inst->popup->gcc, NULL, &gy, NULL, NULL);
   /* FIXME arbitrary value, need to calculate the real space available */
   ODBG("size %d %d %d\n",
        gy, inst->ui.last_item_h, inst->ui.last_network_h);
   count = ((gy - inst->ui.last_network_h - 50) / inst->ui.last_item_h) - 1;
   if (oconnection_scan_hidden_wpa())
     --count;
   if (oconnection_scan_hidden_wep())
     --count;
   if (oconnection_scan_hidden_open())
     --count;
   ODBG("max %d items displayed\n", count);
   _oconnection_popup_populate_info_network(inst);
   if (inst->ui.ifaces_wifi)
     _oconnection_popup_populate_info_scan(inst, count);
   inst->ui.nth_item += count;
   max = eina_list_count(inst->ui.oconnection_items);
   if (max == inst->ui.nth_item)
     inst->ui.nth_item = 0;
   if (inst->ui.nth_item + count > max)
     inst->ui.nth_item = max - count;
}

static void
_oconnection_popup_info_label_wired_network_update(Instance *inst, Oconnection_Wired_Status *ods)
{
   char buf[PATH_MAX];
   Evas_Coord w, h;

   if (ods->connected)
     {
        if (ods->addr)
          snprintf(buf, sizeof(buf),
                   D_("Network cable plugged with<br>IP %s"),
                   ods->addr);
        else
          snprintf(buf, sizeof(buf),
                   D_("Network cable connected, no internet"));
     }
   else
     snprintf(buf, sizeof(buf),
              D_("Your network cable is disconnected"));
   edje_object_part_text_set(ods->obj, "label", buf);
   edje_object_size_min_calc(ods->obj, &w, &h);
   if (inst->ui.info_w < w) inst->ui.info_w = w;
   if (inst->ui.info_h < h) inst->ui.info_h = h;
   evas_object_resize(ods->obj, w, h);

  e_box_pack_options_set(ods->obj,
                         1, 1, /* fill */
                         1, 0, /* expand */
                         0.5, 0.5, /* align */
                         w, h, /* min */
                         -1, -1 /* max */
                        );
}

static void
_oconnection_popup_info_label_wifi_network_update(Instance *inst, Oconnection_Wifi_Status *ows)
{
   char buf[PATH_MAX];
   Evas_Coord w, h;
   if (ows->connected && ows->ssid)
     {
        if (ows->addr)
          snprintf(buf, sizeof(buf),
                   D_("Wi-Fi connected to network %s<br>with IP %s"),
                   ows->ssid, ows->addr);
        else
          snprintf(buf, sizeof(buf),
                   D_("Wi-Fi connected to network %s<br>without IP"),
                   ows->ssid);
     }
   else
     snprintf(buf, sizeof(buf),
              D_("Wi-Fi enabled, no network connected"));
   edje_object_part_text_set(ows->obj, "label", buf);
   edje_object_size_min_calc(ows->obj, &w, &h);
   if (inst->ui.info_w < w) inst->ui.info_w = w;
   if (inst->ui.info_h < h) inst->ui.info_h = h;
   evas_object_resize(ows->obj, w, h);

  e_box_pack_options_set(ows->obj,
                         1, 1, /* fill */
                         1, 0, /* expand */
                         0.5, 0.5, /* align */
                         w, h, /* min */
                         -1, -1 /* max */
                        );
}

static void
_oconnection_update_status(Instance *inst, OIface_Status *os)
{
   if (os && os->is_wifi)
     {
        //FIXME need to create the poller
        if (os->qual > 85)
          edje_object_signal_emit(inst->wifi_icon, "wifi.level5","");
        else if (os->qual > 65)
          edje_object_signal_emit(inst->wifi_icon, "wifi.level4","");
        else if (os->qual > 45)
          edje_object_signal_emit(inst->wifi_icon, "wifi.level3","");
        else if (os->qual > 25)
          edje_object_signal_emit(inst->wifi_icon,"wifi.level2","");
        else if (os->qual > 5)
          edje_object_signal_emit(inst->wifi_icon, "wifi.level1","");
        else
          edje_object_signal_emit(inst->wifi_icon, "wifi.level0","");
        if (inst->connection_type != OCONNECTION_TYPE_WIFI)
          {
             edje_object_signal_emit(inst->o_oconnection,
                                     "wifi_make_transition", "");
             inst->connection_type = OCONNECTION_TYPE_WIFI;
          }
     }
   else
     {
        if (os)
          {
             if (inst->connection_type != OCONNECTION_TYPE_WIRED)
               {
                  edje_object_signal_emit(inst->o_oconnection,
                                          "wired_make_transition", "");
                  inst->connection_type = OCONNECTION_TYPE_WIRED;
               }
          }
        else
          {
             if (inst->connection_type != OCONNECTION_TYPE_NONE)
               {
                  edje_object_signal_emit(inst->o_oconnection,
                                          "no_connection_make_transition", "");
                  inst->connection_type = OCONNECTION_TYPE_NONE;
               }
          }
     }
}

static void
_oconnection_popup_populate_info_network(Instance *inst)
{
   Eina_List *l;
   Oconnection_Wired_Status *ods;
   Oconnection_Wifi_Status *ows;
   Evas *evas;
   evas = inst->popup->win->evas;

   _oconnection_popup_info_label_new(
      inst, evas,
      D_("<title>State of your network connection</title>"));
   EINA_LIST_FOREACH(inst->ui.ifaces_wired, l, ods)
     {
        ods->obj =
           _oconnection_popup_info_label_new(inst, evas, NULL);
        _oconnection_popup_info_label_wired_network_update(inst, ods);
     }
   EINA_LIST_FOREACH(inst->ui.ifaces_wifi, l, ows)
     {
        ows->obj = _oconnection_popup_info_label_new(inst, evas, NULL);
        _oconnection_popup_info_label_wifi_network_update(inst, ows);
     }
}

static void
_oconnection_popup_populate_info_scan(Instance *inst, int max)
{
   Evas *evas;
   Eina_List *list = NULL, *l = NULL;
   Oconnection_Item *oi;
   char buf[PATH_MAX];

   evas = inst->popup->win->evas;
   if (inst->ui.oconnection_items)
     {
        _oconnection_popup_spacer_new(inst, evas);
        _oconnection_popup_info_label_new(
           inst, evas,
           D_("<title>Wi-Fi network scanned</title>"));
        list = eina_list_nth_list(inst->ui.oconnection_items,
                                  inst->ui.nth_item);
        EINA_LIST_FOREACH(list, l, oi)
          {
             _oconnection_popup_item_new(inst, evas, oi);
             if (oconnection_is_in_spool_get(oi->ssid))
               evas_object_show(oi->o_fav);
             if ((oi->flags) & (NW_FLAG_WEP | NW_FLAG_WPA))
               _oconnection_popup_item_security_set(oi, EINA_TRUE);
             snprintf(buf, sizeof(buf), "<right>%d %%</right>", oi->ow->signal);
             _oconnection_popup_item_level_set(oi, buf);
             if (max-- <= 0)
               break;
          }
     }
   if (oconnection_scan_hidden_wpa())
     _oconnection_popup_item_hidden_ssid_new(
        inst, evas, D_("Hidden SSID - WPA"), NW_FLAG_WPA);
   if (oconnection_scan_hidden_wep())
     _oconnection_popup_item_hidden_ssid_new(
        inst, evas, D_("Hidden SSID - WEP"), NW_FLAG_WEP);
   if (oconnection_scan_hidden_open())
     _oconnection_popup_item_hidden_ssid_new(
        inst, evas, D_("Hidden SSID - no security"), NW_FLAG_NONE);
   if ((l) || (list != inst->ui.oconnection_items))
     _oconnection_popup_item_expand_results_new(inst, evas,
                                                D_("More Results..."));
}

static void
_oconnection_popup_populate_ask_key(Instance *inst)
{
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf),
            D_("Connection to network %s"), inst->ui.ssid);
   _oconnection_popup_info_label_new(inst, inst->popup->win->evas,
                                     buf);
   if (inst->ui.key_type & (NW_FLAG_WPA | NW_FLAG_WEP))
     {
        _oconnection_popup_spacer_new(inst, inst->popup->win->evas);
        _oconnection_popup_info_label_new(inst, inst->popup->win->evas,
                                          D_("Enter the network key"));
        _oconnection_popup_item_entry_new(inst, inst->popup->win->evas,
                                          &inst->ui.key);
     }
   _oconnection_popup_item_button_new(
      inst, inst->popup->win->evas, D_("Cancel"),
      inst, _oconnection_popup_button_callback_cancel);
   _oconnection_popup_item_button_new(
      inst, inst->popup->win->evas, D_("Connection"),
      inst, _oconnection_popup_button_callback_connect);
}

static void
_oconnection_popup_populate_reask_key(Instance *inst)
{
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf),
            D_("Connection to network %s"), inst->ui.ssid);
   _oconnection_popup_info_label_new(inst, inst->popup->win->evas,
                                     buf);
   if (inst->ui.key_type & (NW_FLAG_WPA | NW_FLAG_WEP))
     {
        _oconnection_popup_spacer_new(inst, inst->popup->win->evas);
        _oconnection_popup_info_label_new(
           inst, inst->popup->win->evas,
           D_("The password you have typed is incorrect</br>"
              "Please retype the password"));
        _oconnection_popup_item_entry_new(inst, inst->popup->win->evas,
                                          &inst->ui.key);
     }
   _oconnection_popup_item_button_new(
      inst, inst->popup->win->evas, D_("Cancel"),
      inst, _oconnection_popup_button_callback_cancel);
   _oconnection_popup_item_button_new(
      inst, inst->popup->win->evas, D_("Connection"),
      inst, _oconnection_popup_button_callback_connect);
}

static void
_oconnection_popup_populate_ask_ssid(Instance *inst)
{
   _oconnection_popup_info_label_new(
      inst, inst->popup->win->evas, D_("Connection to network"));
   _oconnection_popup_spacer_new(inst, inst->popup->win->evas);
   _oconnection_popup_info_label_new(
      inst, inst->popup->win->evas,
      D_("Enter the name of your wireless network"));
   _oconnection_popup_item_entry_new(
      inst, inst->popup->win->evas, &inst->ui.ssid);
   if (inst->ui.key_type & (NW_FLAG_WPA | NW_FLAG_WEP))
     {
        _oconnection_popup_item_button_new(
           inst, inst->popup->win->evas, D_("Cancel"),
           inst, _oconnection_popup_button_callback_cancel);
        _oconnection_popup_item_button_new(
           inst, inst->popup->win->evas,
           D_("Next"), inst, _oconnection_popup_button_callback_ask_key);
     }
   else
     {
        _oconnection_popup_item_button_new(
           inst, inst->popup->win->evas, D_("Cancel"),
           inst, _oconnection_popup_button_callback_cancel);
        _oconnection_popup_item_button_new(
           inst, inst->popup->win->evas, D_("Connection"),
           inst, _oconnection_popup_button_callback_connect);
     }
}

static void
_oconnection_popup_populate_connect(Instance *inst)
{
   _oconnection_popup_info_label_new(inst,
                                     inst->popup->win->evas,
                                     D_("Connection in progress..."));
}


static void
_oconnection_popup_populate_dhcp(Instance *inst)
{
   _oconnection_popup_info_label_new(inst,
                                     inst->popup->win->evas,
                                     D_("Associated, obtaining address..."));
}

static void
_oconnection_popup_populate_status(Instance *inst)
{
   if (!inst->ui.connected)
     {
        if (inst->ui.associated)
          _oconnection_popup_info_label_new(
             inst, inst->popup->win->evas ,
             D_("Impossible to get an IP address"));
        else
          _oconnection_popup_info_label_new(
             inst, inst->popup->win->evas ,
             D_("Failed connection to the network: invalid key"));

     }
   else
     _oconnection_popup_info_label_new(
        inst, inst->popup->win->evas ,
        D_("Connection suceeded !"));
   inst->ui.status_timer =
      ecore_timer_add(7.0,
                      _oconnection_popup_status_end, inst);
}


static Eina_Bool
_oconnection_popup_status_end(void *data)
{
   Instance *inst;
   inst = data;
   if (inst)
     {
        inst->ui.step = OCONNECTION_STEP_INFO;
        inst->ui.status_timer = NULL;
        _oconnection_popup_del(inst);
     }
   return ECORE_CALLBACK_CANCEL;
}


static void
_oconnection_popup_position(Instance *inst, Evas_Coord ww, Evas_Coord hh)
{
   Evas_Coord gx, gy, gw, gh, zx, zy, zw, zh, nx, ny;
   double ratio;

   e_gadcon_client_geometry_get(inst->popup->gcc, &gx, &gy, &gw, &gh);
   zx = inst->popup->win->zone->x;
   zy = inst->popup->win->zone->y;
   zw = inst->popup->win->zone->w;
   zh = inst->popup->win->zone->h;
   nx = gx - (gw / 2);
   ny = gy - hh;
   if ((nx + ww) >= (zx + zw - OAPPRENDRE_MIN_SPACE))
     nx = (zx + zw - OAPPRENDRE_MIN_SPACE) - inst->popup->w;

   ratio = gx + (gw / 2.0) - nx;
   ratio = ratio / ww;

   e_popup_move_resize(inst->popup->win, nx, ny, ww, hh);
}

static void
_oconnection_popup_size_calc(Instance *inst, Evas_Coord *ww, Evas_Coord *hh)
{
   Evas_Object *o;
   Evas_Coord wx, wh;
   Eina_List *l;
   *hh = 0;

   // these items have a variable height, so we do things differently
   EINA_LIST_FOREACH(inst->ui.items_info, l, o)
     {
        edje_object_size_min_calc(o, &wx, &wh);
        *hh += wh;
     }
   inst->ui.last_network_h = *hh;
   if (inst->ui.items)
     *hh += inst->ui.items_h * eina_list_count(inst->ui.items);
   if (inst->ui.items_more)
     *hh += inst->ui.items_more_h * eina_list_count(inst->ui.items_more);
   if (inst->ui.items_space)
     *hh += inst->ui.space_h * eina_list_count(inst->ui.items_space);
   if (inst->ui.items_entry)
     *hh += inst->ui.entry_h * eina_list_count(inst->ui.items_entry);
   if (inst->ui.items_button)
     *hh += inst->ui.button_h + 10;// * eina_list_count(inst->ui.items_button);

   // 16 is the size of the icon, 4 is to add some extra space on the sides
   // of the popup (asked by the graphist)

   *ww = inst->ui.info_w;
   if (*ww < inst->ui.items_w)
     *ww = inst->ui.items_w;
   if (*ww < inst->ui.items_more_w)
     *ww = inst->ui.items_more_w;
   if (*ww < inst->ui.button_w)
     *ww = inst->ui.button_w;
   if (*ww < inst->ui.entry_w)
     *ww = inst->ui.entry_w;

   e_widget_size_min_set(inst->ui.list, *ww, *hh);
   e_gadcon_popup_content_set(inst->popup, inst->ui.list);
   *ww = inst->popup->w;
   *hh = inst->popup->h;
}


static void
_e_mod_main_popup_scan_results_update(Instance *inst, Eina_List *networks)
{
   OWireless_Network *ow;
   Oconnection_Item *oi;
   Eina_List *l, *ll;
   char buf[PATH_MAX];

   if (!inst->popup) return;
   EINA_LIST_FOREACH(networks, l, ow)
     {
        EINA_LIST_FOREACH(inst->ui.oconnection_items, ll, oi)
          {
             if (oi->ow == ow)
               {
                  if (ow->flags != oi->flags)
                    {
                       oi->flags = ow->flags;
                       if ((oi->flags)
                           & (NW_FLAG_WEP | NW_FLAG_WPA))
                         _oconnection_popup_item_security_set(oi,
                                                              EINA_TRUE);
                       else
                         _oconnection_popup_item_security_set(oi,
                                                              EINA_FALSE);
                    }
                  snprintf(buf, sizeof(buf),
                           "<right>%d %%</right>", ow->signal);
                  _oconnection_popup_item_level_set(oi, buf);
                  break;
               }
          }
     }
}

static Eina_Bool
_e_mod_oconnection_scan_results_cb(void *data, int type, void *event)
{
   Instance *inst;
   Oconnection_Event_Scan_Results *ev;
   Eina_List *l;

   ev = event;
   if (!ev) return ECORE_CALLBACK_DONE;

   EINA_LIST_FOREACH(instances, l, inst)
      _e_mod_main_popup_scan_results_update(inst, ev->networks);
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_mod_main_popup_iface_update(Instance *inst, Eina_List *ifaces)
{
   OIface_Status *os;
   Oconnection_Wired_Status *ods;
   Oconnection_Wifi_Status *ows;
   Eina_List *l, *ll;
   Eina_Bool update = EINA_FALSE;
   Eina_Bool size_update = EINA_FALSE;
   Evas_Coord ww, hh;

   if (!inst->popup) return;

   EINA_LIST_FOREACH(ifaces, ll, os)
     {
        update = EINA_FALSE;
        if (!os->is_wifi)
          {
             EINA_LIST_FOREACH(inst->ui.ifaces_wired, l, ods)
               {
                  if (os->name == ods->name)
                    {
                       if (os->addr != ods->addr)
                         {
                            eina_stringshare_replace(&ods->addr, os->addr);
                            update = EINA_TRUE;
                         }
                       if (os->connected != ods->connected)
                         {
                            ods->connected = os->connected;
                            update = EINA_TRUE;
                         }
                       if (update)
                         _oconnection_popup_info_label_wired_network_update
                            (inst, ods);
                       break;
                    }
               }
          }
        else
          {
             EINA_LIST_FOREACH(inst->ui.ifaces_wifi, l, ows)
               {
                  if (os->name == ows->name)
                    {
                       if (os->addr != ows->addr)
                         {
                            eina_stringshare_replace(&ows->addr, os->addr);
                            update = EINA_TRUE;
                         }
                       if (os->connected != ows->connected)
                         {
                            ows->connected = os->connected;
                            update = EINA_TRUE;
                         }
                       if (os->ssid != ows->ssid)
                         {
                            eina_stringshare_replace(&ows->ssid, os->ssid);
                            update = EINA_TRUE;
                         }
                       if (update)
                         _oconnection_popup_info_label_wifi_network_update(inst,
                                                                           ows);
                       break;
                    }
               }
          }
        if ((update) && (update != size_update))
          size_update = update;
     }
   if (size_update)
     {
        _oconnection_popup_size_calc(inst, &ww, &hh);
        _oconnection_popup_position(inst, ww, hh);
     }
}


static Eina_Bool
_e_mod_oconnection_iface_update_cb(void *data, int type, void *event)
{
   Instance *inst;
   Oconnection_Event_Iface_Update *ev;
   Eina_List *l;
   OIface_Status *os = NULL;

   ev = event;
   if (!ev) return ECORE_CALLBACK_DONE;

   EINA_LIST_FOREACH(oconnection_iface_wifi_get(), l, os)
     {
        if (os->connected)
          break;
     }
   if (!os)
     {
        EINA_LIST_FOREACH(oconnection_iface_wired_get(), l, os)
          {
             if (os->connected)
               break;
          }
     }

   EINA_LIST_FOREACH(instances, l, inst)
     {
        _e_mod_main_popup_iface_update(inst, ev->ifaces);
        _oconnection_update_status(inst, os);
     }

   return ECORE_CALLBACK_PASS_ON;
}

