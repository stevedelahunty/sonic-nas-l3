/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/*
 * filename: nas_route_cps.c
 */


#include "nas_rt_api.h"
#include "event_log_types.h"
#include "event_log.h"

#include "cps_class_map.h"
#include "cps_api_object_key.h"
#include "cps_api_operation.h"
#include "cps_api_events.h"
#include "hal_rt_util.h"

#include <stdlib.h>

static cps_api_event_service_handle_t handle;

static cps_api_return_code_t nas_route_cps_route_set_func(void *ctx,
                             cps_api_transaction_params_t * param, size_t ix) {

    if(param == NULL){
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "Route Set with no param: "
                    "nas_route_cps_route_set_func");
        return cps_api_ret_code_ERR;
    }

    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    if (obj==NULL) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "nas_route_cps_route_set_func: "
                            "NULL obj");
        return cps_api_ret_code_ERR;
    }

    /*
     * Check for keys in filter either Route Key:BASE_ROUTE_OBJ_ENTRY or
     * Neighbor Key:BASE_ROUTE_OBJ_NBR
     *
     */
    cps_api_return_code_t rc = cps_api_ret_code_ERR;

    switch (nas_route_check_route_key_attr(obj)) {
          case BASE_ROUTE_OBJ_ENTRY:
              rc = nas_route_process_cps_route(param,ix);
              break;

          case BASE_ROUTE_OBJ_NBR:
              rc = nas_route_process_cps_nbr(param,ix);
              break;

          default:
              EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "base route obj type unknown %d",
                      cps_api_key_get_subcat(cps_api_object_key(obj)));
              break;
      }

    return rc;
}

static cps_api_return_code_t nas_route_cps_all_route_get_func (void *ctx,
                              cps_api_get_params_t * param, size_t ix) {
    uint32_t af = 0, vrf = 0, pref_len = 0;
    hal_ip_addr_t ip;
    bool is_specific_prefix_get = false;

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "All route Get function");
    cps_api_object_t filt = cps_api_object_list_get(param->filters,ix);
    cps_api_object_attr_t vrf_attr = cps_api_get_key_data(filt,BASE_ROUTE_OBJ_ENTRY_VRF_ID);
    cps_api_object_attr_t af_attr = cps_api_get_key_data(filt,BASE_ROUTE_OBJ_ENTRY_AF);
    cps_api_object_attr_t prefix_attr = cps_api_get_key_data(filt,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX);
    cps_api_object_attr_t pref_len_attr = cps_api_get_key_data(filt,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN);

    if(af_attr == NULL){
        EV_LOG(ERR,ROUTE,0,"NAS-RT-CPS","No address family passed to get Route entries");
        return cps_api_ret_code_ERR;
    } else if (((prefix_attr != NULL) && (pref_len_attr == NULL)) ||
               ((prefix_attr == NULL) && (pref_len_attr != NULL))) {
        EV_LOG(ERR,ROUTE,0,"NAS-RT-CPS","Invlaid prefix info prefix:%s len:%s",
               ((prefix_attr == NULL) ? "Not Present" : "Present"),
               ((pref_len_attr == NULL) ? "Not Present" : "Present"));
        return cps_api_ret_code_ERR;
    }

    af = cps_api_object_attr_data_u32(af_attr);
    if (vrf_attr != NULL) {
        vrf = cps_api_object_attr_data_u32(vrf_attr);
        if (!(FIB_IS_VRF_ID_VALID (vrf))) {
            EV_LOG_ERR (ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "VRF-id:%d is not valid!", vrf);
            return cps_api_ret_code_ERR;
        }
    }
    if (pref_len_attr != NULL) {
        is_specific_prefix_get = true;
        pref_len = cps_api_object_attr_data_u32(pref_len_attr);
        if(af == AF_INET) {
            struct in_addr *inp = (struct in_addr *) cps_api_object_attr_data_bin(prefix_attr);
            std_ip_from_inet(&ip,inp);
        } else {
            struct in6_addr *inp6 = (struct in6_addr *) cps_api_object_attr_data_bin(prefix_attr);
            std_ip_from_inet6(&ip,inp6);
        }
    }
    t_std_error rc;

    nas_l3_lock();
    if((rc = nas_route_get_all_route_info(param->list,vrf, af, ip, pref_len, is_specific_prefix_get)) != STD_ERR_OK){
        nas_l3_unlock();
        return cps_api_ret_code_ERR;
    }
    nas_l3_unlock();
    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_route_cps_route_get_func (void *ctx,
                              cps_api_get_params_t * param, size_t ix) {

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "Route Get function");
    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_route_cps_route_rollback_func (void * ctx,
                              cps_api_transaction_params_t * param, size_t ix){

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "Route Rollback function");
    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_route_cps_nht_set_func (void * ctx,
                             cps_api_transaction_params_t * param, size_t ix) {

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "NHT Set function");
    return cps_api_ret_code_OK;
}
static cps_api_return_code_t nas_route_cps_nht_get_func (void * ctx,
                                cps_api_get_params_t * param, size_t ix) {

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "NHT Get function");
    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_route_cps_nht_rollback_func (void * ctx,
                             cps_api_transaction_params_t * param, size_t ix) {

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "NHT Rollback function");
    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_route_cps_arp_get_func(void *context,
                                cps_api_get_params_t * param, size_t ix) {
    cps_api_object_t filt = cps_api_object_list_get(param->filters,ix);
    cps_api_object_attr_t af_attr = cps_api_get_key_data(filt,BASE_ROUTE_OBJ_NBR_AF);

    if(af_attr == NULL){
        EV_LOG(ERR,ROUTE,0,"NAS-RT-CPS","No address family passed to get ARP entries");
        return cps_api_ret_code_ERR;
    }
    unsigned short af = cps_api_object_attr_data_u16(af_attr);
    t_std_error rc;

    if((rc = nas_route_get_all_arp_info(param->list,af)) != STD_ERR_OK){
        return (cps_api_return_code_t)rc;
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_route_cps_peer_routing_set_func(void *ctx,
                                                                 cps_api_transaction_params_t * param,
                                                                 size_t ix) {
    cps_api_object_t          obj;

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "Peer Routing Entry");
    if(param == NULL){
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "NAS-RT-CPS", "Route Set with no param: "
                     "nas_route_cps_route_set_func");
        return cps_api_ret_code_ERR;
    }

    obj = cps_api_object_list_get (param->change_list, ix);

    if (obj == NULL) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "NAS-RT-CPS", "Missing Peer routing Object");
        return cps_api_ret_code_ERR;
    }

    cps_api_return_code_t rc = cps_api_ret_code_ERR;

    switch (cps_api_key_get_subcat (cps_api_object_key (obj))) {
        case BASE_ROUTE_PEER_ROUTING_CONFIG_OBJ:
            rc = nas_route_process_cps_peer_routing(param,ix);
            break;

        default:
            EV_LOG_ERR (ev_log_t_ROUTE, 3, "NAS-RT-CPS", "base peer route obj type unknown %d",
                         cps_api_key_get_subcat(cps_api_object_key(obj)));
            break;
    }

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "Peer Routing Exit");
    return rc;
}

static cps_api_return_code_t nas_route_cps_peer_routing_get_func (void *ctx,
                                                                  cps_api_get_params_t * param,
                                                                  size_t ix) {
    t_std_error rc;

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "Peer Routing Status Get function");

    nas_l3_lock();
    if((rc = nas_route_get_all_peer_routing_config(param->list)) != STD_ERR_OK){
        nas_l3_unlock();
        return (cps_api_return_code_t)rc;
    }
    nas_l3_unlock();

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_route_cps_peer_routing_rollback_func(void * ctx,
                              cps_api_transaction_params_t * param, size_t ix){

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "Peer Routing Status Rollback function");
    return cps_api_ret_code_OK;
}


static t_std_error nas_route_event_handle_init(){

    if (cps_api_event_service_init() != cps_api_ret_code_OK) {
        EV_LOG(ERR,ROUTE,0,"NAS-RT-CPS","Failed to init cps event service");
        return STD_ERR(ROUTE,FAIL,0);
    }

    if (cps_api_event_client_connect(&handle) != cps_api_ret_code_OK) {
        EV_LOG(ERR,ROUTE,0,"NAS-RT-CPS","Failed to connect handle to cps event service");
        return STD_ERR(ROUTE,FAIL,0);
    }

    return STD_ERR_OK;
}

static t_std_error nas_route_object_entry_init(cps_api_operation_handle_t nas_route_cps_handle ) {

    cps_api_registration_functions_t f;
    char buff[CPS_API_KEY_STR_MAX];

    memset(&f,0,sizeof(f));

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "NAS Routing CPS Initialization");


    /*
     * Initialize Base Route object Entry
     */



    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "Registering for %s",
            cps_api_key_print(&f.key,buff,sizeof(buff)-1));


    f.handle                 = nas_route_cps_handle;
    f._read_function         = nas_route_cps_route_get_func;
    f._write_function         = nas_route_cps_route_set_func;
    f._rollback_function     = nas_route_cps_route_rollback_func;

   if (!cps_api_key_from_attr_with_qual(&f.key,BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET)) {
        EV_LOG_ERR(ev_log_t_ROUTE, 3,"NAS-RT-CPS","Could not translate %d to key %s",
                (int)(BASE_ROUTE_OBJ_OBJ),cps_api_key_print(&f.key,buff,sizeof(buff)-1));
           return STD_ERR(ROUTE,FAIL,0);
    }


    if (cps_api_register(&f)!=cps_api_ret_code_OK) {
            return STD_ERR(ROUTE,FAIL,0);
        }
    return STD_ERR_OK;
}

static t_std_error nas_route_object_route_init(cps_api_operation_handle_t nas_route_cps_handle ) {

    cps_api_registration_functions_t f;
    char buff[CPS_API_KEY_STR_MAX];

    memset(&f,0,sizeof(f));

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "NAS ROUTE CPS Initialization");

    f.handle                 = nas_route_cps_handle;
    f._read_function         = nas_route_cps_all_route_get_func;

    if (!cps_api_key_from_attr_with_qual(&f.key,BASE_ROUTE_OBJ_ENTRY,cps_api_qualifier_TARGET)) {
        EV_LOG_ERR(ev_log_t_ROUTE, 3,"NAS-RT-CPS","Could not translate %d to key %s",
                    (int)(BASE_ROUTE_OBJ_ENTRY),cps_api_key_print(&f.key,buff,sizeof(buff)-1));
        return STD_ERR(ROUTE,FAIL,0);
    }

    if (cps_api_register(&f)!=cps_api_ret_code_OK) {
        return STD_ERR(ROUTE,FAIL,0);
    }
    return STD_ERR_OK;
}

static t_std_error nas_route_object_arp_init(cps_api_operation_handle_t nas_route_cps_handle ) {

    cps_api_registration_functions_t f;
    char buff[CPS_API_KEY_STR_MAX];

    memset(&f,0,sizeof(f));

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "NAS ARP CPS Initialization");

    f.handle                 = nas_route_cps_handle;
    f._read_function         = nas_route_cps_arp_get_func;

    if (!cps_api_key_from_attr_with_qual(&f.key,BASE_ROUTE_OBJ_NBR,cps_api_qualifier_TARGET)) {
        EV_LOG_ERR(ev_log_t_ROUTE, 3,"NAS-RT-CPS","Could not translate %d to key %s",
                    (int)(BASE_ROUTE_OBJ_OBJ),cps_api_key_print(&f.key,buff,sizeof(buff)-1));
        return STD_ERR(ROUTE,FAIL,0);
    }

    if (cps_api_register(&f)!=cps_api_ret_code_OK) {
        return STD_ERR(ROUTE,FAIL,0);
    }
    return STD_ERR_OK;
}


static t_std_error nas_route_object_nht_init(cps_api_operation_handle_t nas_route_cps_handle ) {

    cps_api_registration_functions_t f;
    char buff[CPS_API_KEY_STR_MAX];

    memset(&f,0,sizeof(f));

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "NAS Routing CPS Initialization");


    /*
     * Initialize Base Route NHT object
     */
    if (!cps_api_key_from_attr_with_qual(&f.key,BASE_ROUTE_NH_TRACK_OBJ,cps_api_qualifier_TARGET)) {
        EV_LOG_ERR(ev_log_t_ROUTE, 3,"NAS-RT-CPS","Could not translate %d to key %s",
                (int)(BASE_ROUTE_NH_TRACK_OBJ),cps_api_key_print(&f.key,buff,sizeof(buff)-1));
           return STD_ERR(ROUTE,FAIL,0);
    }


    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "Registering for %s",
            cps_api_key_print(&f.key,buff,sizeof(buff)-1));


    f.handle                 = nas_route_cps_handle;
    f._read_function         = nas_route_cps_nht_get_func;
    f._write_function         = nas_route_cps_nht_set_func;
    f._rollback_function     = nas_route_cps_nht_rollback_func;

    if (cps_api_register(&f)!=cps_api_ret_code_OK) {
            return STD_ERR(ROUTE,FAIL,0);
        }

    return STD_ERR_OK;
}

static t_std_error nas_route_object_peer_routing_init(cps_api_operation_handle_t
                                                      nas_route_cps_handle ) {

    cps_api_registration_functions_t f;
    char buff[CPS_API_KEY_STR_MAX];

    memset(&f,0,sizeof(f));

    EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS", "NAS Peer Routing CPS Initialization");

    f.handle                 = nas_route_cps_handle;
    f._read_function         = nas_route_cps_peer_routing_get_func;
    f._write_function        = nas_route_cps_peer_routing_set_func;
    f._rollback_function     = nas_route_cps_peer_routing_rollback_func;

    if (!cps_api_key_from_attr_with_qual(&f.key,BASE_ROUTE_PEER_ROUTING_CONFIG_OBJ,
                                         cps_api_qualifier_TARGET)) {
        EV_LOG_ERR(ev_log_t_ROUTE, 3,"NAS-RT-CPS","Could not translate %d to key %s",
                    (int)(BASE_ROUTE_OBJ_OBJ),cps_api_key_print(&f.key,buff,sizeof(buff)-1));
        return STD_ERR(ROUTE,FAIL,0);
    }

    if (cps_api_register(&f)!=cps_api_ret_code_OK) {
        return STD_ERR(ROUTE,FAIL,0);
    }
    return STD_ERR_OK;
}


t_std_error nas_routing_cps_init(cps_api_operation_handle_t nas_route_cps_handle) {

    t_std_error ret;

    if((ret = nas_route_object_entry_init(nas_route_cps_handle)) != STD_ERR_OK){
        return ret;
    }

    if((ret = nas_route_object_nht_init(nas_route_cps_handle)) != STD_ERR_OK){
        return ret;
    }

    if((ret = nas_route_object_route_init(nas_route_cps_handle)) != STD_ERR_OK){
        return ret;
    }

    if((ret = nas_route_object_arp_init(nas_route_cps_handle)) != STD_ERR_OK){
        return ret;
    }

    if((ret = nas_route_object_peer_routing_init(nas_route_cps_handle)) != STD_ERR_OK){
        return ret;
    }

    if((ret = nas_route_event_handle_init()) != STD_ERR_OK){
        return ret;
    }

    return ret;
}

t_std_error nas_route_publish_object(cps_api_object_t obj){
    cps_api_return_code_t rc;
    if((rc = cps_api_event_publish(handle,obj))!= cps_api_ret_code_OK){
        EV_LOG(INFO,ROUTE,3,"NAS-RT-CPS","Failed to publish cps event");
        cps_api_object_delete(obj);
        return (t_std_error)rc;
    }
    cps_api_object_delete(obj);
    return STD_ERR_OK;
}

t_std_error nas_route_process_cps_peer_routing(cps_api_transaction_params_t * param, size_t ix) {

    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_return_code_t rc = cps_api_ret_code_OK;
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));
    cps_api_object_attr_t vrf_id_attr;
    cps_api_object_attr_t mac_addr_attr;
    t_peer_routing_config peer_routing_config;
    hal_mac_addr_t mac_addr;
    void *data = NULL;
    uint32_t vrf_id = 0;
    char     p_buf[HAL_RT_MAX_BUFSZ];

    /*
     * Check mandatory peer status attributes
     */
    vrf_id_attr   = cps_api_object_attr_get(obj, BASE_ROUTE_PEER_ROUTING_CONFIG_VRF_ID);
    mac_addr_attr = cps_api_object_attr_get(obj, BASE_ROUTE_PEER_ROUTING_CONFIG_PEER_MAC_ADDR);
    if (mac_addr_attr == NULL) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "Missing peer routing key params");
        return cps_api_ret_code_ERR;
    }
    switch(op) {
        case cps_api_oper_CREATE:
            EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "In Peer Routing Create ");
            peer_routing_config.status = true;
            break;
        case cps_api_oper_SET:
            EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "In Peer Routing Set");
            peer_routing_config.status = true;
            break;
        case cps_api_oper_DELETE:
            EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "In Peer Routing Del");
            peer_routing_config.status = false;
            break;
        default:
            break;
    }

    if (vrf_id_attr) {
        vrf_id =  cps_api_object_attr_data_u32(vrf_id_attr);
        if (!(FIB_IS_VRF_ID_VALID (vrf_id))) {
            EV_LOG_ERR (ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "VRF-id:%d  is not valid!", vrf_id);
            return cps_api_ret_code_ERR;
        }
    }
    data = cps_api_object_attr_data_bin(mac_addr_attr);
    memcpy(peer_routing_config.peer_mac_addr,data,sizeof(mac_addr));

    EV_LOG_TRACE(ev_log_t_ROUTE, 2, "NAS-RT-CPS-SET", "Peer-VRF:%d MAC:%s status:%d",
                 vrf_id, hal_rt_mac_to_str (&peer_routing_config.peer_mac_addr,
                                            p_buf, HAL_RT_MAX_BUFSZ),
                 peer_routing_config.status);
    if ((rc = hal_rt_process_peer_routing_config(vrf_id, &peer_routing_config)) != STD_ERR_OK) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "hal_rt_process_peer_routing_config failed");
        return cps_api_ret_code_ERR;
    }
    return rc;
}


