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

/*!
 * \file   hal_rt_arp.c
 * \brief  Hal Routing arp functionality
 */

#include "hal_rt_main.h"
#include "hal_rt_route.h"
#include "hal_rt_util.h"
#include "hal_rt_debug.h"
#include "nas_rt_api.h"
#include "hal_rt_util.h"

#include "ds_common_types.h"
#include "cps_api_interface_types.h"
#include "cps_api_route.h"
#include "cps_api_operation.h"
#include "cps_api_events.h"
#include "cps_class_map.h"
#include "dell-base-routing.h"


#include "event_log.h"
#include "std_ip_utils.h"

#include <string.h>

typedef struct  {
    unsigned short  family;
    db_nbr_event_type_t    msg_type;
    hal_ip_addr_t         nbr_addr;
    hal_mac_addr_t      nbr_hwaddr;
    hal_ifindex_t   if_index;
    hal_ifindex_t   phy_if_index;
    unsigned long   vrfid;
    unsigned long   expire;
    unsigned long   flags;
    unsigned long   status;
}db_neighbour_entry_t;


static cps_api_object_t nas_neigh_to_cps_obj(db_neighbour_entry_t *entry,cps_api_operation_types_t op){
    if(entry == NULL){
        EV_LOG(ERR,ROUTE,0,"HAL-RT-ARP","Null ARP entry pointer passed to convert it to cps object");
        return NULL;
    }

    cps_api_object_t obj = cps_api_object_create();
    if(obj == NULL){
        EV_LOG(ERR,ROUTE,0,"HAL-RT-ARP","Failed to allocate memory to cps object");
        return NULL;
    }

    cps_api_key_t key;
    cps_api_key_from_attr_with_qual(&key, BASE_ROUTE_OBJ_NBR,
                                        cps_api_qualifier_OBSERVED);
    cps_api_object_set_type_operation(&key,op);
    cps_api_object_set_key(obj,&key);
    if(entry->family == HAL_INET4_FAMILY){
        cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,entry->nbr_addr.u.ipv4.s_addr);
    }else{
        cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,(void *)entry->nbr_addr.u.ipv6.s6_addr,HAL_INET6_LEN);
    }
    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_MAC_ADDR,(void*)entry->nbr_hwaddr,HAL_MAC_ADDR_LEN);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_VRF_ID,entry->vrfid);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_AF,nas_route_af_to_cps_af(entry->family));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_IFINDEX,entry->if_index);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_FLAGS,entry->flags);
    if ((entry->status & RT_NUD_REACHABLE) || (entry->status & RT_NUD_PERMANENT)) {
        cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_STATE,FIB_ARP_RESOLVED);
    } else {
        cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_STATE,FIB_ARP_UNRESOLVED);
    }
    if (entry->status & RT_NUD_PERMANENT) {
        cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_TYPE,BASE_ROUTE_RT_TYPE_STATIC);
    } else {
        cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_TYPE,BASE_ROUTE_RT_TYPE_DYNAMIC);
    }
    return obj;
}


void cps_obj_to_neigh(cps_api_object_t obj,db_neighbour_entry_t *n) {
    cps_api_object_attr_t list[cps_api_if_NEIGH_A_MAX];
    cps_api_object_attr_fill_list(obj,0,list,sizeof(list)/sizeof(*list));

    memset(n,0,sizeof(*n));

    if (list[cps_api_if_NEIGH_A_FAMILY]!=NULL)
        n->family = cps_api_object_attr_data_u32(list[cps_api_if_NEIGH_A_FAMILY]);
    if (list[cps_api_if_NEIGH_A_OPERATION]!=NULL)
        n->msg_type = cps_api_object_attr_data_u32(list[cps_api_if_NEIGH_A_OPERATION]);
    if (list[cps_api_if_NEIGH_A_NBR_ADDR]!=NULL)
        memcpy(&n->nbr_addr,
                cps_api_object_attr_data_bin(list[cps_api_if_NEIGH_A_NBR_ADDR]),
                sizeof(n->nbr_addr));
    if (list[cps_api_if_NEIGH_A_NBR_MAC]!=NULL)
        memcpy(n->nbr_hwaddr,
                cps_api_object_attr_data_bin(list[cps_api_if_NEIGH_A_NBR_MAC]),
                sizeof(n->nbr_hwaddr));
    if (list[cps_api_if_NEIGH_A_IFINDEX]!=NULL)
        n->if_index = cps_api_object_attr_data_u32(list[cps_api_if_NEIGH_A_IFINDEX]);
    if (list[cps_api_if_NEIGH_A_PHY_IFINDEX]!=NULL)
        n->phy_if_index = cps_api_object_attr_data_u32(list[cps_api_if_NEIGH_A_PHY_IFINDEX]);
    if (list[cps_api_if_NEIGH_A_VRF]!=NULL)
        n->vrfid = cps_api_object_attr_data_u32(list[cps_api_if_NEIGH_A_VRF]);
    if (list[cps_api_if_NEIGH_A_EXPIRE]!=NULL)
        n->expire= cps_api_object_attr_data_u32(list[cps_api_if_NEIGH_A_EXPIRE]);
    if (list[cps_api_if_NEIGH_A_FLAGS]!=NULL)
        n->flags = cps_api_object_attr_data_u32(list[cps_api_if_NEIGH_A_FLAGS]);
    if (list[cps_api_if_NEIGH_A_STATE]!=NULL)
        n->status = cps_api_object_attr_data_u32(list[cps_api_if_NEIGH_A_STATE]);
}

t_std_error fib_proc_nbr_download (cps_api_object_t obj)
{
    db_neighbour_entry_t     arp_msg;

    db_neighbour_entry_t      *p_arp_info_msg = NULL;

    uint32_t           vrf_id = 0;

    uint32_t           sub_cmd = 0;
    uint8_t            af_index = 0;
    bool               nbr_change = false;
    char               p_buf[HAL_RT_MAX_BUFSZ];

    cps_obj_to_neigh(obj,&arp_msg);

    p_arp_info_msg = &arp_msg;
    if(!p_arp_info_msg) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "%s (): NULL nbr entry\n", __FUNCTION__);
        return (STD_ERR_MK(e_std_err_ROUTE, e_std_err_code_FAIL, 0));
    }

    EV_LOG_TRACE (ev_log_t_ROUTE, 1, "HAL-RT-ARP_OStoNAS", "cmd:%s(%d) vrf_id:%d, family:%d state:0x%x ip_addr:%s, "
                  "mac_addr:%s, out_if_index:%d phy:%d expire:%d status:0x%x",
                  ((p_arp_info_msg->msg_type == NBR_ADD) ? "Nbr-Add" : ((p_arp_info_msg->msg_type == NBR_DEL) ?
                                                                       "Nbr-Del" : "Unknown")),
                  p_arp_info_msg->msg_type, p_arp_info_msg->vrfid,
                  p_arp_info_msg->family, p_arp_info_msg->status,
                  FIB_IP_ADDR_TO_STR (&p_arp_info_msg->nbr_addr),
                  hal_rt_mac_to_str (&p_arp_info_msg->nbr_hwaddr, p_buf, HAL_RT_MAX_BUFSZ),
                  p_arp_info_msg->if_index,
                  p_arp_info_msg->phy_if_index, p_arp_info_msg->expire, p_arp_info_msg->status);


    if(hal_rt_validate_intf(p_arp_info_msg->if_index) != STD_ERR_OK) {
        return STD_ERR_OK;
    }

    sub_cmd  = p_arp_info_msg->msg_type;
    vrf_id   = p_arp_info_msg->vrfid;
    af_index = HAL_RT_ADDR_FAM_TO_AFINDEX(p_arp_info_msg->family);

    if (!(FIB_IS_VRF_ID_VALID (vrf_id))) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "%s (): Invalid vrf_id. vrf_id: %d\r\n",
                    __FUNCTION__, vrf_id);
        return STD_ERR_OK;
    }

    p_arp_info_msg->nbr_addr.af_index = af_index;
    if (hal_rt_is_reserved_ipv6(&p_arp_info_msg->nbr_addr)) {
        EV_LOG_TRACE(ev_log_t_ROUTE, 3, "HAL-RT-DR", "Skipping rsvd ipv6 addr %s on if_indx %d",
                     FIB_IP_ADDR_TO_STR(&p_arp_info_msg->nbr_addr), p_arp_info_msg->if_index);
        return STD_ERR_OK;
    }

    switch (sub_cmd) {
        case NBR_ADD:
            FIB_INCR_CNTRS_NBR_ADD (vrf_id, af_index);
            if (fib_proc_arp_add (af_index, p_arp_info_msg) == STD_ERR_OK){
                nbr_change = true;
            }
            break;

        case NBR_DEL:
            FIB_INCR_CNTRS_NBR_DEL (vrf_id, af_index);
            if (fib_proc_arp_del (af_index, p_arp_info_msg) == STD_ERR_OK){
                nbr_change = true;
            }
            break;

        default:
            EV_LOG_TRACE (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "Unknown sub_cmd: %d\r\n", sub_cmd);
            FIB_INCR_CNTRS_UNKNOWN_MSG (vrf_id, af_index);
            break;
    }

    if(nbr_change) {
        cps_api_operation_types_t op = (sub_cmd == NBR_ADD) ?cps_api_oper_CREATE :cps_api_oper_DELETE;
        cps_api_object_t obj = nas_neigh_to_cps_obj(p_arp_info_msg,op);
        if(obj != NULL){
            if(nas_route_publish_object(obj)!= STD_ERR_OK){
                EV_LOG(INFO,ROUTE,3,"HAL-RT-DR","Failed to publish ARP entry");
            }
        }
        EV_LOG(INFO,ROUTE,3,"HAL-RT-DR","Published an ARP entry with CPS operation %d",op);
        fib_resume_nh_walker_thread(af_index);
    }

    return STD_ERR_OK;
}

t_std_error fib_proc_arp_add (uint8_t af_index, void *p_arp_info)
{
    t_fib_arp_msg_info   fib_arp_msg_info;
    t_fib_nh          *p_nh = NULL;
    char               p_buf[HAL_RT_MAX_BUFSZ];

    if (!p_arp_info) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "%s (): Invalid input param. p_arp_info: %p\r\n",
                    __FUNCTION__, p_arp_info);
        return (STD_ERR_MK(e_std_err_ROUTE, e_std_err_code_FAIL, 0));
    }

    memset (&fib_arp_msg_info, 0, sizeof (t_fib_arp_msg_info));

    fib_form_arp_msg_info (af_index, p_arp_info, &fib_arp_msg_info, false);

    nas_l3_lock();

    p_nh = fib_get_nh (fib_arp_msg_info.vrf_id, &fib_arp_msg_info.ip_addr,
                       fib_arp_msg_info.if_index);
    if(p_nh != NULL) {
        if((p_nh->p_arp_info) && (p_nh->p_arp_info->if_index == fib_arp_msg_info.out_if_index) &&
           (!memcmp((uint8_t *)&p_nh->p_arp_info->mac_addr,
                    (uint8_t *)&fib_arp_msg_info.mac_addr,
                    HAL_RT_MAC_ADDR_LEN))) {
            EV_LOG_TRACE (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "Duplicate Neighbor add Msg "
                    "vrf_id: %d, ip_addr: %s, if_index: 0x%x status:0x%x\r\n",
                    fib_arp_msg_info.vrf_id, FIB_IP_ADDR_TO_STR (&fib_arp_msg_info.ip_addr),
                    fib_arp_msg_info.if_index, fib_arp_msg_info.status);

            p_nh->p_arp_info->arp_status = fib_arp_msg_info.status;
            nas_l3_unlock();
            return (STD_ERR_MK(e_std_err_ROUTE, e_std_err_code_FAIL, 0));
        }

    }

    EV_LOG_TRACE (ev_log_t_ROUTE, 1, "HAL-RT-ARP(ARP-START)", "vrf_id: %d, ip_addr: %s, "
            "if_index: %d, mac_addr: %s, out_if_index: %d status:%d", fib_arp_msg_info.vrf_id,
            FIB_IP_ADDR_TO_STR (&fib_arp_msg_info.ip_addr),
            fib_arp_msg_info.if_index,
            hal_rt_mac_to_str (&fib_arp_msg_info.mac_addr, p_buf, HAL_RT_MAX_BUFSZ),
            fib_arp_msg_info.out_if_index, fib_arp_msg_info.status);

    p_nh = fib_proc_nh_add (fib_arp_msg_info.vrf_id, &fib_arp_msg_info.ip_addr,
                            fib_arp_msg_info.if_index, FIB_NH_OWNER_TYPE_ARP, 0);
    if (p_nh == NULL) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "%s (): NH addition failed. "
                    "vrf_id: %d, ip_addr: %s, if_index: 0x%x\r\n", __FUNCTION__,
                    fib_arp_msg_info.vrf_id, FIB_IP_ADDR_TO_STR (&fib_arp_msg_info.ip_addr),
                    fib_arp_msg_info.if_index);

        nas_l3_unlock();
        return (STD_ERR_MK(e_std_err_ROUTE, e_std_err_code_FAIL, 0));
    }

    if (p_nh->p_arp_info == NULL) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "%s (): NH's arp info NULL. "
                "vrf_id: %d, ip_addr: %s, if_index: 0x%x\r\n", __FUNCTION__,
                fib_arp_msg_info.vrf_id, FIB_IP_ADDR_TO_STR (&fib_arp_msg_info.ip_addr),
                fib_arp_msg_info.if_index);

        nas_l3_unlock();
        return (STD_ERR_MK(e_std_err_ROUTE, e_std_err_code_FAIL, 0));
    }

    p_nh->p_arp_info->if_index = fib_arp_msg_info.out_if_index;
    p_nh->p_arp_info->vlan_id = 0;  //unused
    memcpy((uint8_t *)&p_nh->p_arp_info->mac_addr, (uint8_t *)&fib_arp_msg_info.mac_addr, HAL_RT_MAC_ADDR_LEN);
    p_nh->p_arp_info->arp_status = fib_arp_msg_info.status;
    if(hal_rt_is_mac_address_zero((const hal_mac_addr_t *)&fib_arp_msg_info.mac_addr)) {
        p_nh->p_arp_info->state = FIB_ARP_UNRESOLVED;
    } else {
        p_nh->p_arp_info->state = FIB_ARP_RESOLVED;
    }
    p_nh->p_arp_info->is_l2_fh = fib_arp_msg_info.is_l2_fh;

    nas_l3_unlock();
    return STD_ERR_OK;
}

t_std_error fib_proc_arp_del (uint8_t af_index, void *p_arp_info)
{
    t_fib_arp_msg_info  fib_arp_msg_info;
    t_fib_nh           *p_nh = NULL;
    char                p_buf[HAL_RT_MAX_BUFSZ];

    if (!p_arp_info) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "%s (): Invalid input param. p_arp_info: %p\r\n",
                    __FUNCTION__, p_arp_info);
        return (STD_ERR_MK(e_std_err_ROUTE, e_std_err_code_FAIL, 0));
    }

    memset (&fib_arp_msg_info, 0, sizeof (t_fib_arp_msg_info));
    fib_form_arp_msg_info (af_index, p_arp_info, &fib_arp_msg_info, false);

    EV_LOG_TRACE (ev_log_t_ROUTE, 1, "HAL-RT-ARP(ARP-START)", "vrf_id: %d, ip_addr: %s, "
               "if_index: %d, mac_addr: %s, out_if_index: %d", fib_arp_msg_info.vrf_id,
               FIB_IP_ADDR_TO_STR (&fib_arp_msg_info.ip_addr),
               fib_arp_msg_info.if_index,
               hal_rt_mac_to_str (&fib_arp_msg_info.mac_addr, p_buf, HAL_RT_MAX_BUFSZ),
               fib_arp_msg_info.out_if_index);

    if (!(FIB_IS_VRF_ID_VALID (fib_arp_msg_info.vrf_id))) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "%s (): Invalid vrf_id. vrf_id: %d\r\n",
                    __FUNCTION__, fib_arp_msg_info.vrf_id);
        return (STD_ERR_MK(e_std_err_ROUTE, e_std_err_code_FAIL, 0));
    }

    nas_l3_lock();

    p_nh = fib_get_nh (fib_arp_msg_info.vrf_id,
                       &fib_arp_msg_info.ip_addr, fib_arp_msg_info.if_index);
    if (p_nh == NULL) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "%s (): NH not found. "
                   "vrf_id: %d, ip_addr: %s, if_index: %d\r\n", __FUNCTION__,
                   fib_arp_msg_info.vrf_id, FIB_IP_ADDR_TO_STR (&fib_arp_msg_info.ip_addr),
                   fib_arp_msg_info.if_index);

        nas_l3_unlock();
        return (STD_ERR_MK(e_std_err_ROUTE, e_std_err_code_FAIL, 0));
    }

    if (p_nh->p_arp_info == NULL) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "%s (): NH's arp info NULL. "
                   "vrf_id: %d, ip_addr: %s, if_index: %d\r\n", __FUNCTION__,
                   fib_arp_msg_info.vrf_id, FIB_IP_ADDR_TO_STR (&fib_arp_msg_info.ip_addr),
                   fib_arp_msg_info.if_index);

        nas_l3_unlock();
        return (STD_ERR_MK(e_std_err_ROUTE, e_std_err_code_FAIL, 0));
    }

    fib_proc_nh_delete (p_nh, FIB_NH_OWNER_TYPE_ARP, 0);

    nas_l3_unlock();

    return STD_ERR_OK;
}

t_std_error fib_form_arp_msg_info (uint8_t af_index, void *p_arp_info,
                                   t_fib_arp_msg_info *p_fib_arp_msg_info, bool is_clear_msg)
{
    db_neighbour_entry_t *p_arp_info_msg = NULL;
    db_neighbour_entry_t *p_ndpm_info = NULL;
    char                  p_buf[HAL_RT_MAX_BUFSZ];

    if ((!p_arp_info) || (!p_fib_arp_msg_info)) {
        EV_LOG_ERR (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "%s (): Invalid input param. p_arp_info: %p, "
                    "p_fib_arp_msg_info: %p\r\n", __FUNCTION__, p_arp_info, p_fib_arp_msg_info);
        return (STD_ERR_MK(e_std_err_ROUTE, e_std_err_code_FAIL, 0));
    }

    EV_LOG_TRACE (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "af_index: %d, is_clear_msg: %d\r\n",
                  af_index, is_clear_msg);

    memset (p_fib_arp_msg_info, 0, sizeof (t_fib_arp_msg_info));

    if (STD_IP_IS_AFINDEX_V4 (af_index))
    {
        if (is_clear_msg == false)
        {
            p_arp_info_msg = (db_neighbour_entry_t *)p_arp_info;
            p_fib_arp_msg_info->vrf_id = p_arp_info_msg->vrfid;
            memcpy(&p_fib_arp_msg_info->ip_addr,&p_arp_info_msg->nbr_addr,
                    sizeof(p_fib_arp_msg_info->ip_addr));
            p_fib_arp_msg_info->ip_addr.af_index = HAL_RT_V4_AFINDEX;

            p_fib_arp_msg_info->if_index    = p_arp_info_msg->if_index;
            p_fib_arp_msg_info->out_if_index = p_arp_info_msg->if_index;
            memcpy (&p_fib_arp_msg_info->mac_addr, &p_arp_info_msg->nbr_hwaddr, HAL_RT_MAC_ADDR_LEN);
            p_fib_arp_msg_info->status = p_arp_info_msg->status;
        }
    }
    else if (FIB_IS_AFINDEX_V6 (af_index))
    {
        if (is_clear_msg == false)
        {
            p_ndpm_info = (db_neighbour_entry_t *)p_arp_info;
            p_fib_arp_msg_info->vrf_id = FIB_DEFAULT_VRF;

            memcpy(&p_fib_arp_msg_info->ip_addr,
                    &p_ndpm_info->nbr_addr,sizeof(p_fib_arp_msg_info->ip_addr));
            p_fib_arp_msg_info->ip_addr.af_index = HAL_RT_V6_AFINDEX;

            p_fib_arp_msg_info->if_index    = p_ndpm_info->if_index;
            p_fib_arp_msg_info->out_if_index = p_ndpm_info->if_index;
            memcpy (&p_fib_arp_msg_info->mac_addr, &p_ndpm_info->nbr_hwaddr, HAL_RT_MAC_ADDR_LEN);
            p_fib_arp_msg_info->status = p_ndpm_info->status;
        }
    }

    EV_LOG_TRACE (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "vrf_id: %d, ip_addr: %s, if_index: 0x%x, "
               "mac_addr: %s, out_if_index: 0x%x status:0x%x\r\n", p_fib_arp_msg_info->vrf_id,
               FIB_IP_ADDR_TO_STR (&p_fib_arp_msg_info->ip_addr),
               p_fib_arp_msg_info->if_index,
               hal_rt_mac_to_str (&p_fib_arp_msg_info->mac_addr, p_buf, HAL_RT_MAX_BUFSZ),
               p_fib_arp_msg_info->out_if_index, p_fib_arp_msg_info->status);
    return STD_ERR_OK;
}

t_fib_cmp_result fib_arp_info_cmp (t_fib_nh *p_fh, t_fib_arp_msg_info *p_fib_arp_msg_info, uint32_t state)
{
    uint16_t  new_vlan_id = 0;
    char      p_buf[HAL_RT_MAX_BUFSZ];

    if ((!p_fh) || (!p_fib_arp_msg_info)) {
        EV_LOG_TRACE (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "Invalid input param. p_fh: %p, "
                      "p_fib_arp_msg_info: %p\r\n", p_fh, p_fib_arp_msg_info);
        return FIB_CMP_RESULT_NOT_EQUAL;
    }

    if (p_fh->p_arp_info == NULL) {
        EV_LOG_TRACE (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "Arp info NULL. vrf_id: %d, ip_addr: %s, "
                      "if_index: 0x%x\r\n", p_fh->vrf_id, FIB_IP_ADDR_TO_STR (&p_fh->key.ip_addr),
                      p_fh->key.if_index);
        return FIB_CMP_RESULT_NOT_EQUAL;
    }

    EV_LOG_TRACE (ev_log_t_ROUTE, 3, "HAL-RT-ARP", "vrf_id: %d, ip_addr: %s, if_index: 0x%x, "
               "mac_addr: %s, out_if_index: 0x%x, state: %d\r\n",
               p_fh->vrf_id, FIB_IP_ADDR_TO_STR (&p_fib_arp_msg_info->ip_addr),
               p_fib_arp_msg_info->if_index,
               hal_rt_mac_to_str (&p_fib_arp_msg_info->mac_addr, p_buf, HAL_RT_MAX_BUFSZ),
               p_fib_arp_msg_info->out_if_index, state);

    new_vlan_id = 0;    //unused

    if ((p_fh->p_arp_info->vlan_id == new_vlan_id) &&
        ((memcmp (&p_fh->p_arp_info->mac_addr, &p_fib_arp_msg_info->mac_addr, HAL_RT_MAC_ADDR_LEN))==0)&&
        (p_fh->p_arp_info->state == state) &&
        (p_fh->p_arp_info->if_index == p_fib_arp_msg_info->out_if_index)) {
        return FIB_CMP_RESULT_EQUAL;
    }

    return FIB_CMP_RESULT_NOT_EQUAL;
}

static cps_api_object_t nas_route_nh_to_arp_cps_object(t_fib_nh *entry){

    if(entry == NULL){
        EV_LOG(ERR,ROUTE,0,"HAL-RT-ARP","Null NH entry pointer passed to convert it to cps object");
        return NULL;
    }

    if(entry->p_arp_info == NULL){
        EV_LOG(ERR,ROUTE,0,"HAL-RT-ARP","No ARP info associated with next hop");
        return NULL;
    }

    cps_api_object_t obj = cps_api_object_create();
    if(obj == NULL){
        EV_LOG(ERR,ROUTE,0,"HAL-RT-ARP","Failed to allocate memory to cps object");
        return NULL;
    }

    cps_api_key_t key;
    cps_api_key_from_attr_with_qual(&key, BASE_ROUTE_OBJ_NBR,
                                        cps_api_qualifier_TARGET);
    cps_api_object_set_key(obj,&key);

    if(entry->key.ip_addr.af_index == HAL_INET4_FAMILY){
        cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,entry->key.ip_addr.u.ipv4.s_addr);
    }else{
        cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,(void *)entry->key.ip_addr.u.ipv6.s6_addr,HAL_INET6_LEN);
    }
    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_MAC_ADDR,(void*)entry->p_arp_info->mac_addr,HAL_MAC_ADDR_LEN);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_VRF_ID,entry->vrf_id);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_AF,nas_route_af_to_cps_af(entry->key.ip_addr.af_index));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_IFINDEX,entry->p_arp_info->if_index);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_STATE,entry->p_arp_info->state);
    if (entry->p_arp_info->arp_status & RT_NUD_PERMANENT) {
        cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_TYPE,BASE_ROUTE_RT_TYPE_STATIC);
    }else {
        cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_TYPE,BASE_ROUTE_RT_TYPE_DYNAMIC);
    }

    return obj;
}

t_std_error nas_route_get_all_arp_info(cps_api_object_list_t list, unsigned short af){

    if (af >= FIB_MAX_AFINDEX)
    {
        EV_LOG(ERR,ROUTE,0,"HAL-RT-ARP","Invalid Address family");
        return STD_ERR(ROUTE,FAIL,0);
    }

    t_fib_nh *p_nh = NULL;
    const unsigned int vrf_id =0;

    nas_l3_lock();

    p_nh = fib_get_first_nh (vrf_id, af);

    while (p_nh != NULL){

        if (p_nh->p_arp_info != NULL){
            cps_api_object_t obj = nas_route_nh_to_arp_cps_object(p_nh);
            if(obj != NULL){
                if (!cps_api_object_list_append(list,obj)) {
                    cps_api_object_delete(obj);
                    EV_LOG(ERR,ROUTE,0,"HAL-RT-ARP","Failed to append object to object list");
                    nas_l3_unlock();
                    return STD_ERR(ROUTE,FAIL,0);
                }
            }
        }
        p_nh = fib_get_next_nh (vrf_id, &p_nh->key.ip_addr, p_nh->key.if_index);
    }
    nas_l3_unlock();
    return STD_ERR_OK;
}
