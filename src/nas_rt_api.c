/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/*
 * filename: nas_route_api.c
 */


#include "dell-base-routing.h"
#include "nas_rt_api.h"
#include "nas_os_l3.h"
#include "hal_rt_util.h"
#include "event_log_types.h"
#include "event_log.h"
#include "std_mutex_lock.h"

#include "cps_class_map.h"
#include "cps_api_object_key.h"
#include "cps_api_operation.h"
#include "cps_api_events.h"

#include <stdio.h>
#include <stdint.h>

BASE_ROUTE_OBJ_t nas_route_check_route_key_attr(cps_api_object_t obj) {

    BASE_ROUTE_OBJ_t  default_type = BASE_ROUTE_OBJ_ENTRY;

    /*
     * Route Key
     */
    cps_api_object_attr_t route_af;
    cps_api_object_attr_t prefix;
    cps_api_object_attr_t pref_len;
    /*
     * Nbr Key
     */
    cps_api_object_attr_t nbr_af;
    cps_api_object_attr_t nbr_addr;

    /*
     * Check mandatory key attributes
     */
    route_af     = cps_api_get_key_data(obj, BASE_ROUTE_OBJ_ENTRY_AF);
    prefix       = cps_api_get_key_data(obj, BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX);
    pref_len     = cps_api_get_key_data(obj, BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN);
    nbr_af       = cps_api_get_key_data(obj, BASE_ROUTE_OBJ_NBR_AF);
    nbr_addr     = cps_api_get_key_data(obj, BASE_ROUTE_OBJ_NBR_ADDRESS);

    /*
     * If route delete case, check the key mandatory attrs for delete case first
     */

    if (route_af != CPS_API_ATTR_NULL && prefix != CPS_API_ATTR_NULL &&
         pref_len != CPS_API_ATTR_NULL) {
        return BASE_ROUTE_OBJ_ENTRY;
    } else if ((nbr_af != CPS_API_ATTR_NULL && nbr_addr != CPS_API_ATTR_NULL) ||
               (nbr_addr != CPS_API_ATTR_NULL)){
        return BASE_ROUTE_OBJ_NBR;
    }

    return default_type;
}


static inline bool nas_route_validate_route_attr(cps_api_object_t obj, bool del) {

    cps_api_object_attr_t af;
    cps_api_object_attr_t vrf_id;
    cps_api_object_attr_t prefix;
    cps_api_object_attr_t pref_len;
    cps_api_object_attr_t nh_count;

    /*
     * Check mandatory route attributes
     */
    af       = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_AF);
    vrf_id   = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_VRF_ID);
    prefix   = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX);
    pref_len = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN);

    /*
     * If route delete case, check the key mandatory attrs for delete case first
     */

    if  (af == CPS_API_ATTR_NULL || vrf_id == CPS_API_ATTR_NULL ||
            prefix == CPS_API_ATTR_NULL || pref_len == CPS_API_ATTR_NULL) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "Missing route mandatory attr params");
        return false;
    }

    /*
     * Route delete case
     */
    if (del == true) {
        return true;
    }

    /*
     * If route add/update case, check for NH attributes also
     */
    nh_count = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_NH_COUNT);

    cps_api_attr_id_t ids[3] = { BASE_ROUTE_OBJ_ENTRY_NH_LIST, 0,
                                 BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR};
    const int ids_len = sizeof(ids)/sizeof(*ids);

    cps_api_object_attr_t gw = cps_api_object_e_get(obj, ids, ids_len);

    ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_IFINDEX;
    cps_api_object_attr_t gwix = cps_api_object_e_get(obj, ids, ids_len);

    if (nh_count == CPS_API_ATTR_NULL || (gw == CPS_API_ATTR_NULL && gwix == CPS_API_ATTR_NULL)) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "Missing route nh params");
        return false;
    }

    return true;
}


static cps_api_return_code_t nas_route_filter_route_func(cps_api_object_t obj,
                    cps_api_transaction_params_t * param,size_t ix,
                    cps_api_operation_types_t op) {
    /*
     *  @@TODO     Call filter function to filter the route
     *  either as Management route, App route etc
     *
     */
    return cps_api_ret_code_OK;

}


static inline bool nas_route_validate_nbr_attr(cps_api_object_t obj, bool del) {

    cps_api_object_attr_t af;
    cps_api_object_attr_t ipaddr;
    cps_api_object_attr_t mac_addr;
    cps_api_object_attr_t if_index;

    /*
     * Check mandatory nbr attributes
     */
    ipaddr   = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_NBR_ADDRESS);
    af       = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_NBR_AF);
    mac_addr = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_NBR_MAC_ADDR);
    if_index = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_NBR_IFINDEX);

    /*
     * If route delete case, check the key mandatory attrs for delete case first
     */
    if (ipaddr == CPS_API_ATTR_NULL || af == CPS_API_ATTR_NULL ||
        if_index == CPS_API_ATTR_NULL) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "Missing Neighbor mandatory attr params");
        return false;
    }

    /*
     * Route delete case
     */
    if (del == true) {
        return true;
    }

    /*
     * If route add/update case, check for NH attributes also
     */

    if (mac_addr == CPS_API_ATTR_NULL){
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "Missing Neighbor mac attr for update");
        return false;
    }

    return true;
}

static cps_api_return_code_t nas_route_filter_nbr_func(cps_api_object_t obj,
                    cps_api_transaction_params_t * param,size_t ix,
                    cps_api_operation_types_t op) {
    /*
     *  @@TODO     Call filter function to filter the Nbr
     *  either as Management or App entry etc
     *
     */
    return cps_api_ret_code_OK;

}

cps_api_return_code_t  nas_route_process_cps_route(cps_api_transaction_params_t * param, size_t ix) {

    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_return_code_t rc = cps_api_ret_code_OK;
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    if (nas_route_validate_route_attr(obj, (op == cps_api_oper_DELETE)? true:false) == false) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "Missing route key params");
        return cps_api_ret_code_ERR;
    }

    /*
     *  Call filter function to filter the route
     *  either as Management route, App route and
     *  once conditions are satisfied, install the route into kernel
     *  via cps-api-os interface
     */
    if(nas_route_filter_route_func(obj, param, ix, op) != cps_api_ret_code_OK){
        rc = cps_api_ret_code_ERR;
    }

    cps_api_object_t cloned = cps_api_object_create();
    if (!cloned) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "CPS malloc error");
        return cps_api_ret_code_ERR;
    }
    cps_api_object_clone(cloned,obj);
    cps_api_object_list_append(param->prev,cloned);

    if (op == cps_api_oper_CREATE) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "In OS Route CREATE ");
        if(nas_os_add_route(obj) != STD_ERR_OK){
            EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "OS Route add failed");
            rc = cps_api_ret_code_ERR;
        }
    } else if (op == cps_api_oper_DELETE) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "In Route del ");
        if(nas_os_del_route(obj) != STD_ERR_OK){
            EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "OS Route del failed");
            rc = cps_api_ret_code_ERR;
        }
    } else if (op == cps_api_oper_SET) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "In Route update ");
        if(nas_os_set_route(obj) != STD_ERR_OK){
            EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", " OS Route update failed");
            rc = cps_api_ret_code_ERR;
        }
    }
    return rc;
}
cps_api_return_code_t nas_route_process_cps_nbr(cps_api_transaction_params_t * param, size_t ix) {

    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);

    cps_api_return_code_t rc = cps_api_ret_code_OK;
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    if (nas_route_validate_nbr_attr(obj, (op == cps_api_oper_DELETE)? true:false) == false) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "Missing Neighbbr key params");
        return cps_api_ret_code_ERR;
    }

    /*
     *  Call filter function to filter the Nbr
     *  either as Management entry, App. entry
     *  once conditions are satisfied, install the entry into kernel
     *  via cps-api-os interface
     */
    if(nas_route_filter_nbr_func(obj, param, ix, op) != cps_api_ret_code_OK){
        rc = cps_api_ret_code_ERR;
    }

    cps_api_object_t cloned = cps_api_object_create();
    if (!cloned) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "CPS malloc error");
        return cps_api_ret_code_ERR;
    }
    cps_api_object_clone(cloned,obj);
    cps_api_object_list_append(param->prev,cloned);

    if (op == cps_api_oper_CREATE) {
        if(nas_os_add_neighbor(obj) != STD_ERR_OK){
            EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "OS Neighbor add failed");
            rc = cps_api_ret_code_ERR;
        }
    } else if (op == cps_api_oper_DELETE) {
        if(nas_os_del_neighbor(obj) != STD_ERR_OK){
            EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", "OS Neighbor del failed");
            rc = cps_api_ret_code_ERR;
        }
    } else if (op == cps_api_oper_SET) {
        if(nas_os_set_neighbor(obj) != STD_ERR_OK){
            EV_LOG_TRACE(ev_log_t_ROUTE, 3, "NAS-RT-CPS-SET", " OS Neighbor update failed");
            rc = cps_api_ret_code_ERR;
        }
    }

    return rc;
}

static cps_api_object_t nas_route_info_to_cps_object(t_fib_dr *entry){
    t_fib_nh       *p_nh = NULL;
    t_fib_nh_holder nh_holder1;
    int weight = 0;
    int addr_len = 0, nh_itr = 0, is_arp_resolved = false;

    if(entry == NULL){
        EV_LOG(ERR,ROUTE,0,"HAL-RT-API","Null DR entry pointer passed to convert it to cps object");
        return NULL;
    }

    cps_api_object_t obj = cps_api_object_create();
    if(obj == NULL){
        EV_LOG(ERR,ROUTE,0,"HAL-RT-API","Failed to allocate memory to cps object");
        return NULL;
    }

    cps_api_key_t key;
    cps_api_key_from_attr_with_qual(&key, BASE_ROUTE_OBJ_ENTRY,
                                    cps_api_qualifier_TARGET);
    cps_api_object_set_key(obj,&key);

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_VRF_ID,entry->vrf_id);
    if(entry->key.prefix.af_index == HAL_INET4_FAMILY){
        addr_len = HAL_INET4_LEN;
        cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&(entry->key.prefix.u.v4_addr), addr_len);
    }else{
        addr_len = HAL_INET6_LEN;
        cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&(entry->key.prefix.u.v6_addr), addr_len);
    }
    cps_api_object_attr_add_u32(obj, BASE_ROUTE_OBJ_ENTRY_AF, entry->key.prefix.af_index);
    cps_api_object_attr_add_u32(obj, BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN, entry->prefix_len);
    cps_api_object_attr_add_u32(obj, BASE_ROUTE_OBJ_ENTRY_PROTOCOL, entry->proto);
    cps_api_object_attr_add_u32(obj, BASE_ROUTE_OBJ_ENTRY_OWNER, entry->default_dr_owner);

    FIB_FOR_EACH_NH_FROM_DR (entry, p_nh, nh_holder1)
    {
        cps_api_attr_id_t parent_list[3];

        parent_list[0] = BASE_ROUTE_OBJ_ENTRY_NH_LIST;
        parent_list[1] = nh_itr;
        parent_list[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR;

        if(entry->key.prefix.af_index == HAL_INET4_FAMILY) {
            cps_api_object_e_add(obj, parent_list, 3,
                                 cps_api_object_ATTR_T_BIN, &p_nh->key.ip_addr.u.v4_addr, addr_len);
        } else {
            cps_api_object_e_add(obj, parent_list, 3,
                                 cps_api_object_ATTR_T_BIN, &p_nh->key.ip_addr.u.v6_addr, addr_len);
        }

        parent_list[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_IFINDEX;
        cps_api_object_e_add(obj, parent_list, 3,
                             cps_api_object_ATTR_T_U32, &p_nh->key.if_index, sizeof(p_nh->key.if_index));

        parent_list[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_WEIGHT;
        cps_api_object_e_add(obj, parent_list, 3,
                             cps_api_object_ATTR_T_U32, &weight, sizeof(weight));

        parent_list[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_RESOLVED;
        if (p_nh->p_arp_info != NULL)
        {
            is_arp_resolved = ((p_nh->p_arp_info->state == FIB_ARP_RESOLVED) ? true : false);
        }
        cps_api_object_e_add(obj, parent_list, 3,
                             cps_api_object_ATTR_T_U32, &is_arp_resolved, sizeof(is_arp_resolved));

        nh_itr++;
    }
    cps_api_object_attr_add_u32(obj, BASE_ROUTE_OBJ_ENTRY_NH_COUNT, nh_itr);

    return obj;
}

t_std_error nas_route_get_all_route_info(cps_api_object_list_t list, uint32_t vrf_id, uint32_t af,
                                         hal_ip_addr_t prefix, uint32_t pref_len, bool is_specific_prefix_get) {

    t_fib_dr *p_dr = NULL;

    if (af >= FIB_MAX_AFINDEX)
    {
        EV_LOG(ERR,ROUTE,0,"HAL-RT-ARP","Invalid Address family");
        return STD_ERR(ROUTE,FAIL,0);
    }

    if (is_specific_prefix_get) {
        p_dr = fib_get_dr (vrf_id, &prefix, pref_len);
    } else {
        p_dr = fib_get_first_dr(vrf_id, af);
    }
    while (p_dr != NULL){
        cps_api_object_t obj = nas_route_info_to_cps_object(p_dr);
        if(obj != NULL){
            if (!cps_api_object_list_append(list,obj)) {
                cps_api_object_delete(obj);
                EV_LOG(ERR,ROUTE,0,"HAL-RT-API","Failed to append object to object list");
                return STD_ERR(ROUTE,FAIL,0);
            }
        }

        if (is_specific_prefix_get)
            break;

        p_dr = fib_get_next_dr (vrf_id, &p_dr->key.prefix, p_dr->prefix_len);
    }
    return STD_ERR_OK;
}

