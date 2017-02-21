#ifndef OCONNECTION_NETWORK_MANAGER_H_
#define OCONNECTION_NETWORK_MANAGER_H_

void oconnection_nm_init(void);
void oconnection_nm_shutdown(void);
void oconnection_nm_scan(void);
void oconnection_nm_connect(OWireless_Network *ow, const char *psk);
void oconnection_nm_connect_hidden(const char *ssid, const char *psk, Oconnection_Network_Flags key_type);
Eina_Bool oconnection_nm_is_in_spool_get(const char *key);

#endif /* OCONNECTION_NETWORK_MANAGER_H_ */

