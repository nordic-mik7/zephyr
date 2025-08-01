/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUPP_MGMT_H
#define ZEPHYR_SUPP_MGMT_H

#include <defs.h>
#include <zephyr/net/wifi_mgmt.h>

#ifndef MAX_SSID_LEN
#define MAX_SSID_LEN 32
#endif
#ifndef MAC_ADDR_LEN
#define MAC_ADDR_LEN 6
#endif

#define MAC_STR_LEN 18 /* for ':' or '-' separated MAC address string */
#define CHAN_NUM_LEN 6 /* for space-separated channel numbers string */

enum wifi_frequency_bands wpas_band_to_zephyr(enum wpa_radio_work_band band);

enum wifi_wpa3_enterprise_type wpas_key_mgmt_to_zephyr_wpa3_ent(int key_mgmt);

enum wifi_security_type wpas_key_mgmt_to_zephyr(bool is_hapd, void *config,
						int key_mgmt, int proto, int pwe);

const struct wifi_mgmt_ops *const get_wifi_mgmt_api(const struct device *dev);

enum wifi_mfp_options get_mfp(enum mfp_options supp_mfp_option);

/**
 * @brief Get version
 *
 * @param dev: Wi-Fi interface name to use
 * @param params: version to fill
 *
 * @return: 0 for OK; <0 for ERROR
 */
int supplicant_get_version(const struct device *dev, struct wifi_version *params);

/**
 * @brief Request a connection
 *
 * @param dev: Wi-Fi interface name to use
 * @param params: Connection details
 *
 * @return: 0 for OK; -1 for ERROR
 */
int supplicant_connect(const struct device *dev, struct wifi_connect_req_params *params);

/**
 * @brief Forces station to disconnect and stops any subsequent scan
 *  or connection attempts
 *
 * @param dev: Wi-Fi interface name to use
 *
 * @return: 0 for OK; -1 for ERROR
 */
int supplicant_disconnect(const struct device *dev);

/**
 * @brief
 *
 * @param dev: Wi-Fi interface name to use
 * @param status: Status structure to fill
 *
 * @return: 0 for OK; -1 for ERROR
 */
int supplicant_status(const struct device *dev, struct wifi_iface_status *status);

/**
 * @brief Request a scan
 *
 * @param dev Wi-Fi interface name to use
 * @param params Scan parameters
 * @param cb Callback to be called for each scan result
 *
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_scan(const struct device *dev, struct wifi_scan_params *params,
		    scan_result_cb_t cb);

#if defined(CONFIG_NET_STATISTICS_WIFI) || defined(__DOXYGEN__)
/**
 * @brief Get Wi-Fi statistics
 *
 * @param dev Wi-Fi interface name to use
 * @param stats Pointer to stats structure to fill
 *
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_get_stats(const struct device *dev, struct net_stats_wifi *stats);
/**
 * @brief Reset Wi-Fi statistics
 *
 * @param dev Wi-Fi interface name to use
 *
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_reset_stats(const struct device *dev);
#endif /* CONFIG_NET_STATISTICS_WIFI || __DOXYGEN__ */

/** Flush PMKSA cache entries
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 if ok, < 0 if error
 */
int supplicant_pmksa_flush(const struct device *dev);

/** Set or get 11K status
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param params 11k parameters
 *
 * @return 0 if ok, < 0 if error
 */
int supplicant_11k_cfg(const struct device *dev, struct wifi_11k_params *params);

/** Send 11k neighbor request
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param params 11k parameters
 *
 * @return 0 if ok, < 0 if error
 */
int supplicant_11k_neighbor_request(const struct device *dev, struct wifi_11k_params *params);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_ROAMING
/** Send candidate scan request
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param params Scan parameters
 *
 * @return 0 if ok, < 0 if error
 */
int supplicant_candidate_scan(const struct device *dev, struct wifi_scan_params *params);

/** Send 11r roaming request
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 if ok, < 0 if error
 */
int supplicant_11r_roaming(const struct device *dev);
#endif

/**
 * @brief Set Wi-Fi power save configuration
 *
 * @param dev Wi-Fi interface name to use
 * @param params Power save parameters to set
 *
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_set_power_save(const struct device *dev, struct wifi_ps_params *params);

/**
 * @brief Set Wi-Fi TWT parameters
 *
 * @param dev Wi-Fi interface name to use
 * @param params TWT parameters to set
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_set_twt(const struct device *dev, struct wifi_twt_params *params);

/**
 * @brief Set Wi-Fi BTWT parameters
 *
 * @param dev Wi-Fi interface name to use
 * @param params BTWT parameters to set
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_set_btwt(const struct device *dev, struct wifi_twt_params *params);

/**
 * @brief Get Wi-Fi power save configuration
 *
 * @param dev Wi-Fi interface name to use
 * @param config Address of power save configuration to fill
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_get_power_save_config(const struct device *dev, struct wifi_ps_config *config);

/**
 * @brief Set Wi-Fi Regulatory domain
 *
 * @param dev Wi-Fi interface name to use
 * @param reg_domain Regulatory domain to set
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_reg_domain(const struct device *dev, struct wifi_reg_domain *reg_domain);

/**
 * @brief Set Wi-Fi mode of operation
 *
 * @param dev Wi-Fi interface name to use
 * @param mode Mode setting to set
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_mode(const struct device *dev, struct wifi_mode_info *mode);

#if defined CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE || \
	defined CONFIG_WIFI_NM_HOSTAPD_CRYPTO_ENTERPRISE
int is_eap_valid_security(int security);

int process_cipher_config(struct wifi_connect_req_params *params,
			  struct wifi_eap_cipher_config *cipher_config);

/** Set Wi-Fi enterprise mode CA/client Cert and key
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param file Pointer to the CA/client Cert and key.
 *
 * @return 0 if ok, < 0 if error
 */
int supplicant_add_enterprise_creds(const struct device *dev,
		struct wifi_enterprise_creds_params *creds);
#endif

/**
 * @brief Set Wi-Fi packet filter for sniffing operation
 *
 * @param dev Wi-Fi interface name to use
 * @param filter Filter settings to set
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_filter(const struct device *dev, struct wifi_filter_info *filter);

/**
 * @brief Set Wi-Fi channel for monitor or TX injection mode
 *
 * @param dev Wi-Fi interface name to use
 * @param channel Channel settings to set
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_channel(const struct device *dev, struct wifi_channel_info *channel);

/**
 * @brief Set Wi-Fi RTS threshold
 *
 * @param dev Wi-Fi interface handle to use
 * @param rts_threshold RTS threshold to set
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_set_rts_threshold(const struct device *dev, unsigned int rts_threshold);

/**
 * @brief Get Wi-Fi RTS threshold
 *
 * @param dev Wi-Fi interface handle to use
 * @param rts_threshold Pointer to the RTS threshold value.
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_get_rts_threshold(const struct device *dev, unsigned int *rts_threshold);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_WNM
/** Send bss transition query
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reason query reason
 *
 * @return 0 if ok, < 0 if error
 */
int supplicant_btm_query(const struct device *dev, uint8_t reason);
#endif

/** Send legacy roam
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 if ok, < 0 if error
 */
int supplicant_legacy_roam(const struct device *dev);

/** Judge ap whether support the capability
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param capab is the capability to judge
 *
 * @return 1 if support, 0 if not support
 */
int supplicant_bss_ext_capab(const struct device *dev, int capab);

/** Get Wi-Fi connection parameters recently used
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param params the Wi-Fi connection parameters recently used
 *
 * @return 0 if ok, < 0 if error
 */
int supplicant_get_wifi_conn_params(const struct device *dev,
			 struct wifi_connect_req_params *params);

/** Start a WPS PBC/PIN connection
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param params wps operarion parameters
 *
 * @return 0 if ok, < 0 if error
 */
int supplicant_wps_config(const struct device *dev, struct wifi_wps_config_params *params);

/** @ Set Wi-Fi max idle period
 *
 * @param dev Wi-Fi interface handle to use
 * @param bss_max_idle_period Maximum idle period to set
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_set_bss_max_idle_period(const struct device *dev,
				       unsigned short bss_max_idle_period);
#ifdef CONFIG_AP
int set_ap_bandwidth(const struct device *dev, enum wifi_frequency_bandwidths bandwidth);

/**
 * @brief Set Wi-Fi AP configuration
 *
 * @param dev Wi-Fi interface name to use
 * @param params AP configuration parameters to set
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_ap_enable(const struct device *dev,
			 struct wifi_connect_req_params *params);

/**
 * @brief Disable Wi-Fi AP
 * @param dev Wi-Fi interface name to use
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_ap_disable(const struct device *dev);

/**
 * @brief Set Wi-Fi AP STA disconnect
 * @param dev Wi-Fi interface name to use
 * @param mac_addr MAC address of the station to disconnect
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_ap_sta_disconnect(const struct device *dev,
				 const uint8_t *mac_addr);

#endif /* CONFIG_AP */

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
#define SUPPLICANT_DPP_CMD_BUF_SIZE 384

int dpp_params_to_cmd(struct wifi_dpp_params *params, char *cmd, size_t max_len);

/**
 * @brief Dispatch DPP operations for STA
 *
 * @param dev Wi-Fi interface name to use
 * @param dpp_params DPP action enum and params in string
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_dpp_dispatch(const struct device *dev, struct wifi_dpp_params *params);
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP */

/**
 * @brief Wi-Fi STA configuration parameter.
 *
 * @param dev Wi-Fi interface handle to use
 * @param params STA parameters
 * @return 0 for OK; -1 for ERROR
 */
int supplicant_config_params(const struct device *dev, struct wifi_config_params *params);
#endif /* ZEPHYR_SUPP_MGMT_H */
