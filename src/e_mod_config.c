#include <e.h>
#include "e_mod_main.h"

/* The typedef for this structure is declared inside the E code in order to
 * allow everybody to use this type, you dont need to declare the typedef, 
 * just use the E_Config_Dialog_Data for your data structures declarations */
struct _E_Config_Dialog_Data 
{
   int switch1;
};

/* Local Function Prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

/* External Functions */

/* Function for calling our personal dialog menu */
EAPI E_Config_Dialog *
e_int_config_oconnection_module(E_Container *con, const char *path) 
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   char buf[PATH_MAX];

   /* is this config dialog already visible ? */
   if (e_config_dialog_find("Oconnection", "advanced/oconnection")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   /* Icon in the theme */
   snprintf(buf, sizeof(buf), "%s/e-module-oconnection.edj", oconnection_conf->module->dir);

   /* create our config dialog */
   cfd = e_config_dialog_new(con, D_("Oconnection Module"), "Oconnection", 
                             "advanced/oconnection", buf, 0, v, NULL);

   e_dialog_resizable_set(cfd->dia, 1);
   oconnection_conf->cfd = cfd;
   return cfd;
}

/* Local Functions */
static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata = NULL;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   oconnection_conf->cfd = NULL;
   E_FREE(cfdata);
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   /* load a temp copy of the config variables */
   cfdata->switch1 = oconnection_conf->switch1;
}

void entry_signal_cb_(void* data, Evas_Object *o, const char* emission, const char* source)
{
  printf("Champ de texte\n");
  printf("emission %s\nsource %s\n",emission, source);

}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o = NULL;//, *of = NULL, *ow = NULL;
   
   Evas_Object *entry = NULL ;

   o = e_widget_list_add(evas, 0, 0);

   //of = e_widget_framelist_add(evas, D_("General"), 0);
   //e_widget_framelist_content_align_set(of, 0.0, 0.0);
   //ow = e_widget_check_add(evas, D_("Use Switch 1"), 
                          // &(cfdata->switch1));

   
   char **textfield = NULL ;
   entry = e_widget_entry_add(evas,textfield,NULL,NULL,NULL);
   edje_object_signal_callback_add(entry,"*","*",entry_signal_cb_,NULL);

   //e_widget_framelist_object_append(of, ow);
   //e_widget_framelist_object_append(of, entry);
   e_widget_list_object_append(o, entry, 1, 1, 0.5);

   return o;
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   oconnection_conf->switch1 = cfdata->switch1;
   e_config_save_queue();
   return 1;
}
