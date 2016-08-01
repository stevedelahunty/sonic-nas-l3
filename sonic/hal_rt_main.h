
/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */


/*!
 * \file   hal_rt_main.h
 * \brief  Hal Routing Core functionality
 * \date   05-2014
 * \author Prince Sunny & Satish Mynam
 */

#ifndef __HAL_RT_MAIN_H__
#define __HAL_RT_MAIN_H__

#include "std_radical.h"
#include "std_radix.h"
#include "ds_common_types.h"
#include "nas_ndi_router_interface.h"
#include "std_llist.h"
#include "std_mutex_lock.h"

#include <stdbool.h>
#include <sys/socket.h>
#include <stdint.h>

#define HAL_RT_MAC_ADDR_LEN               HAL_MAC_ADDR_LEN
#define HAL_RT_V4_ADDR_LEN                HAL_INET4_LEN
#define HAL_RT_V6_ADDR_LEN                HAL_INET6_LEN
#define HAL_RT_V4_AFINDEX                 HAL_INET4_FAMILY
#define HAL_RT_V6_AFINDEX                 HAL_INET6_FAMILY
#define HAL_RT_MAX_INSTANCE               1
#define HAL_RT_V4_PREFIX_LEN              (8 * HAL_INET4_LEN)
#define HAL_RT_V6_PREFIX_LEN              (8 * HAL_INET6_LEN)
#define HAL_RT_DEF_MAX_ECMP_PATH          16   /* Default Maximum supported ECMP paths per Group */
#define HAL_RT_MAX_ECMP_PATH              64   /* Maximum supported ECMP paths per Group */
#define MAX_LEN_VRF_NAME                  32
#define FIB_MIN_AFINDEX                   HAL_RT_V4_AFINDEX
#define FIB_MAX_AFINDEX                   (HAL_RT_V6_AFINDEX + 1)
#define HAL_RT_MAX_PEER_ENTRY             16

enum _rt_proto {
    // Direct routes.
    RT_CONNECTED = 1,
    // Kernel routes
    RT_KERNEL,
    // Static mroutes.
    RT_MSTATIC,
    // Static routes.
    RT_STATIC,
    // OSPF routes.
    RT_OSPF,
    // IS-IS routes.
    RT_ISIS,
    // BGP routes.
    RT_MBGP,
    // BGP routes.
    RT_BGP,
    // RIP routes.
    RT_RIP,
    // MPLS routes.
    RT_MPLS,
    // MAX value for range check. MUST BE LAST.
    RT_PROTO_MAX,
};

/* ARP/ND status */
#define RT_NUD_INCOMPLETE 0x01
#define RT_NUD_REACHABLE  0x02
#define RT_NUD_STALE      0x04
#define RT_NUD_DELAY      0x08
#define RT_NUD_PROBE      0x10
#define RT_NUD_FAILED     0x20
#define RT_NUD_NOARP      0x40
#define RT_NUD_PERMANENT  0x80
#define RT_NUD_NONE          0x00

typedef enum _rt_proto rt_proto;

typedef hal_ip_addr_t t_fib_ip_addr;

typedef struct _t_fib_audit_host_key {
    hal_vrf_id_t  vrf_id;
    t_fib_ip_addr ip_addr;
} t_fib_audit_host_key;

typedef struct _t_fib_audit_cfg {
    uint32_t interval;   /* Time interval in minutes */
} t_fib_audit_cfg;

typedef struct _t_fib_audit_route_key {
    hal_vrf_id_t      vrf_id;
    t_fib_ip_addr     prefix;
    uint8_t           prefix_len;
} t_fib_audit_route_key;

typedef struct _t_fib_route_summary {
    uint32_t         a_curr_count [HAL_RT_V6_PREFIX_LEN + 1];
} t_fib_route_summary;

typedef enum {
    HAL_RT_STATUS_ECMP,
    HAL_RT_STATUS_NON_ECMP,
    HAL_RT_STATUS_ECMP_INVALID
} t_fib_ecmp_status;

typedef struct _t_fib_link_node {
    std_dll  glue;
    void    *self;
} t_fib_link_node;

typedef struct _t_fib_config {
    uint32_t         max_num_npu;
    uint32_t         ecmp_max_paths;
    uint32_t         hw_ecmp_max_paths;
    bool             ecmp_path_fall_back;
    uint8_t          ecmp_hash_sel;
} t_fib_config;

typedef struct _t_fib_tnl_key {
    hal_ifindex_t if_index;
    hal_vrf_id_t  vrf_id;
} t_fib_tnl_key;

typedef struct _t_fib_tnl_dest {
    std_rt_head    rt_head;
    t_fib_tnl_key  key;
    t_fib_ip_addr  dest_addr;
} t_fib_tnl_dest;

typedef struct _t_fib_dr_msg_info {
    hal_vrf_id_t   vrf_id;
    t_fib_ip_addr  prefix;
    uint8_t        prefix_len;
    rt_proto       proto;
} t_fib_dr_msg_info;

typedef struct _t_fib_nh_msg_info {
    hal_vrf_id_t   vrf_id;
    t_fib_ip_addr  ip_addr;
    hal_ifindex_t  if_index;
} t_fib_nh_msg_info;

typedef struct _t_fib_arp_msg_info {
    hal_vrf_id_t   vrf_id;
    t_fib_ip_addr  ip_addr;
    hal_ifindex_t  if_index;
    uint8_t        mac_addr [HAL_RT_MAC_ADDR_LEN];
    uint32_t       out_if_index;
    uint8_t        is_l2_fh;
    uint8_t        status;
} t_fib_arp_msg_info;

typedef enum {
    FIB_AUDIT_NOT_STARTED,
    FIB_AUDIT_STARTED,
} t_fib_audit_status;

typedef struct _t_fib_audit {
    t_fib_audit_cfg     curr_cfg;
    t_fib_audit_cfg     next_cfg;
    t_fib_audit_status  status;
    uint8_t             enabled;
    uint8_t             af_index;
    uint8_t             to_be_stopped;
    uint8_t            is_first;
    uint8_t             hw_pass_over;
    uint8_t             sw_pass_over;
    uint32_t            num_audits_completed;
    uint64_t            last_audit_start_time;
    uint64_t            last_audit_end_time;
    uint64_t            last_audit_wake_up_time;
} t_fib_audit;

typedef struct _t_fib_vrf_info {
    hal_vrf_id_t        vrf_id;
    uint8_t             vrf_name [MAX_LEN_VRF_NAME + 1];
    uint8_t             af_index;
    bool                is_vrf_created;
    std_rt_table       *dr_tree;  /* Each node in the tree is of type t_fib_dR */
    std_rt_table       *nh_tree;  /* Each node in the tree is of type t_fib_nH */
    std_rt_table       *mp_md5_tree;  /* Each node in the tree is of type t_fib_mp_obj */
    std_radical_ref_t   dr_radical_marker;
    std_radical_ref_t   nh_radical_marker;
    uint32_t            num_dr_processed_by_walker;
    uint32_t            num_nh_processed_by_walker;
    bool                clear_ip_fib_on;
    bool                clear_ip_route_on;
    bool                clear_arp_on;
    bool                dr_clear_on;
    bool                nh_clear_on;
    std_radix_version_t dr_clear_max_radix_ver;
    std_radix_version_t nh_clear_max_radix_ver;
    bool                dr_ha_on;
    bool                nh_ha_on;
    bool                is_catch_all_disabled;
    std_radix_version_t dr_ha_max_radix_ver;
    std_radix_version_t nh_ha_max_radix_ver;
    t_fib_route_summary route_summary;
} t_fib_vrf_info;

typedef struct _t_fib_vrf_cntrs {
    uint32_t  num_route_add;
    uint32_t  num_route_del;
    uint32_t  num_vrf_add;
    uint32_t  num_vrf_del;
    uint32_t  num_route_clear;
    uint32_t  num_nbr_add;
    uint32_t  num_nbr_del;
    uint32_t  num_nbr_resolving;
    uint32_t  num_nbr_un_rslvd;
    uint32_t  num_nbr_clear;
    uint32_t  num_unknown_msg;
    uint32_t  num_fib_host_entries;
    uint32_t  num_fib_route_entries;
    uint32_t  num_cam_host_entries;
    uint32_t  num_cam_route_entries;
} t_fib_vrf_cntrs;

typedef struct _t_peer_routing_config {
    ndi_vrf_id_t     obj_id; /* NDI handle */
    uint8_t          peer_mac_addr [HAL_RT_MAC_ADDR_LEN];
    bool             status; /* true - enabled, false - disabled */
}t_peer_routing_config;

typedef struct _t_fib_vrf {
    hal_vrf_id_t     vrf_id;
    ndi_vrf_id_t     vrf_obj_id;
    t_fib_vrf_info   info [FIB_MAX_AFINDEX];
    t_fib_vrf_cntrs  cntrs [FIB_MAX_AFINDEX];
    t_peer_routing_config peer_routing_config[HAL_RT_MAX_PEER_ENTRY];
} t_fib_vrf;

typedef enum _t_fib_cmp_result {
    FIB_CMP_RESULT_EQUAL = 1,
    FIB_CMP_RESULT_NOT_EQUAL = 2,
} t_fib_cmp_result;

#define FIB_RDX_MAX_NAME_LEN           64
#define FIB_DEFAULT_ECMP_HASH          0
#define RT_PER_TLV_MAX_LEN             (2 * (sizeof(unsigned long)))
#define FIB_RDX_INTF_KEY_LEN           (8 * (sizeof (t_fib_intf_key)))

/* Common Data Structures */
#define FIB_DEFAULT_VRF                0
#define FIB_MIN_VRF                    0
#define FIB_MAX_VRF                    1
#define FIB_IS_VRF_ID_VALID(_vrf_id)   ((_vrf_id) < FIB_MAX_VRF)

#define FIB_MASK_V6_BYTES(_p_ip_addr1, _p_ip_addr2, _p_mask, _index)           \
        ((((_p_ip_addr1)->u.v6_addr[(_index)] &                                \
           ((_p_mask)->u.v6_addr[(_index)])) ==                                \
          ((_p_ip_addr2)->u.v6_addr[(_index)] &                                \
           ((_p_mask)->u.v6_addr[(_index)]))))

#define FIB_IS_IP_ADDR_IN_PREFIX(_p_prefix, _p_mask, _p_ip_addr)               \
        (((_p_prefix)->af_index == HAL_RT_V4_AFINDEX) ?                        \
         (FIB_IS_V4_ADDR_IN_PREFIX ((_p_prefix), (_p_mask), (_p_ip_addr))) :   \
         (FIB_IS_V6_ADDR_IN_PREFIX ((_p_prefix), (_p_mask), (_p_ip_addr))))

#define FIB_IS_V4_ADDR_IN_PREFIX(_p_prefix, _p_mask, _p_ip_addr)               \
        ((((_p_prefix)->u.v4_addr) & ((_p_mask)->u.v4_addr)) ==                \
         (((_p_ip_addr)->u.v4_addr) & ((_p_mask)->u.v4_addr)))

#define FIB_IS_V6_ADDR_IN_PREFIX(_p_prefix, _p_mask, _p_ip_addr)               \
        ((FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 0))  &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 1))  &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 2))  &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 3))  &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 4))  &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 5))  &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 6))  &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 7))  &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 8))  &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 9))  &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 10)) &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 11)) &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 12)) &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 13)) &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 14)) &&            \
         (FIB_MASK_V6_BYTES(_p_prefix, _p_ip_addr, _p_mask, 15)))


/* Macros to get/traverse the DLLs nodes in HAL Routing data structures */

#define FIB_DLL_GET_FIRST(_p_dll_head) std_dll_getfirst(_p_dll_head)

#define FIB_DLL_GET_NEXT(_p_dll_head, _p_dll)  \
        (((_p_dll) != NULL) ? std_dll_getnext((_p_dll_head), (_p_dll)) : NULL)

#define FIB_IS_AFINDEX_V6(_af_index)                                        \
        (((_af_index) == HAL_RT_V6_AFINDEX))

#define FIB_AFINDEX_TO_PREFIX_LEN(_af_index)                                \
        (((_af_index) == HAL_RT_V4_AFINDEX) ?                                  \
         HAL_RT_V4_PREFIX_LEN : HAL_RT_V6_PREFIX_LEN)

#define HAL_RT_ADDR_FAM_TO_AFINDEX(_family)                                 \
        (((_family) == AF_INET) ?                                           \
         HAL_RT_V4_AFINDEX : HAL_RT_V6_AFINDEX)

#define FIB_PORT_TYPE_VAL_TO_STR(_port_type, _if_index)                          \
        (((_port_type) == FIB_FH_PORT_TYPE_CPU) ? "Cpu" :                       \
        (((_port_type) == FIB_FH_PORT_TYPE_BLK_HOLE) ? "Black_hole" :            \
         (((_port_type) == FIB_FH_PORT_TYPE_ARP) ? FIB_IFINDEX_TO_STR(_if_index):\
          "Invalid")))

#define FIB_IS_PREFIX_LEN_VALID(_af_index, _prefix_len)                      \
        (((_prefix_len)) <= FIB_AFINDEX_TO_PREFIX_LEN ((_af_index)))

#define FIB_GET_VRF_INFO(_vrf_id, _af_index)                                 \
        (hal_rt_access_fib_vrf_info(_vrf_id, _af_index))

#define FIB_IS_VRF_CREATED(_vrf_id, _af_index)                               \
        (((hal_rt_access_fib_vrf_info(_vrf_id, _af_index))->is_vrf_created) == true)

#define FIB_GET_ROUTE_SUMMARY(_vrf_id, _af_index)                            \
        (&((hal_rt_access_fib_vrf_info(_vrf_id, _af_index))->route_summary))

#define FIB_INCR_CNTRS_ROUTE_ADD(_vrf_id, _af_index)                     \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_route_add)++)

#define FIB_INCR_CNTRS_ROUTE_DEL(_vrf_id, _af_index)                     \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_route_del)++)

#define FIB_INCR_CNTRS_VRF_ADD(_vrf_id, _af_index)                       \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_vrf_add)++)

#define FIB_INCR_CNTRS_VRF_DEL(_vrf_id, _af_index)                       \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_vrf_del)++)

#define FIB_INCR_CNTRS_ROUTE_CLR(_vrf_id, _af_index)                     \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_route_clear)++)

#define FIB_INCR_CNTRS_UNKNOWN_MSG(_vrf_id, _af_index)                   \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_unknown_msg)++)

#define FIB_INCR_CNTRS_NBR_ADD(_vrf_id, _af_index)                          \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_nbr_add)++)

#define FIB_INCR_CNTRS_NBR_DEL(_vrf_id, _af_index)                          \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_nbr_del)++)

#define FIB_INCR_CNTRS_NBR_RESOLVING(_vrf_id, _af_index)                    \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_nbr_resolving)++)

#define FIB_INCR_CNTRS_NBR_UNRSLVD(_vrf_id, _af_index)                      \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_nbr_un_rslvd)++)

#define FIB_INCR_CNTRS_NBR_VRF_DEL(_vrf_id, _af_index)                      \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_nbr_vrf_del)++)

#define FIB_INCR_CNTRS_NBR_CLR(_vrf_id, _af_index)                          \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_nbr_clear)++)

#define FIB_INCR_CNTRS_FIB_HOST_ENTRIES(_vrf_id, _af_index)                  \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_fib_host_entries)++)

#define FIB_INCR_CNTRS_FIB_ROUTE_ENTRIES(_vrf_id, _af_index)                 \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_fib_route_entries)++)

#define FIB_INCR_CNTRS_CAM_HOST_ENTRIES(_vrf_id, _af_index)                  \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_cam_host_entries)++)

#define FIB_INCR_CNTRS_CAM_ROUTE_ENTRIES(_vrf_id, _af_index)                 \
        (((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_cam_route_entries)++)

#define FIB_DECR_CNTRS_FIB_HOST_ENTRIES(_vrf_id, _af_index)                  \
        if ((FIB_GET_CNTRS_FIB_HOST_ENTRIES ((_vrf_id), (_af_index))) > 0)   \
        {                                                                  \
            ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_fib_host_entries)--;   \
        }

#define FIB_DECR_CNTRS_FIB_ROUTE_ENTRIES(_vrf_id, _af_index)                 \
        if ((FIB_GET_CNTRS_FIB_ROUTE_ENTRIES ((_vrf_id), (_af_index))) > 0)  \
        {                                                                  \
            ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_fib_route_entries)--;  \
        }

#define FIB_DECR_CNTRS_CAM_HOST_ENTRIES(_vrf_id, _af_index)                  \
        if ((FIB_GET_CNTRS_CAM_HOST_ENTRIES ((_vrf_id), (_af_index))) > 0)   \
        {                                                                  \
            ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_cam_host_entries)--;   \
        }

#define FIB_DECR_CNTRS_CAM_ROUTE_ENTRIES(_vrf_id, _af_index)                 \
        if ((FIB_GET_CNTRS_CAM_ROUTE_ENTRIES ((_vrf_id), (_af_index))) > 0)  \
        {                                                                  \
            ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_cam_route_entries)--;  \
        }

#define FIB_GET_CNTRS_ROUTE_ADD(_vrf_id, _af_index)                      \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_route_add)

#define FIB_GET_CNTRS_ROUTE_DEL(_vrf_id, _af_index)                      \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_route_del)

#define FIB_GET_CNTRS_VRF_ADD(_vrf_id, _af_index)                        \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_vrf_add)

#define FIB_GET_CNTRS_VRF_DEL(_vrf_id, _af_index)                        \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_vrf_del)

#define FIB_GET_CNTRS_ROUTE_CLR(_vrf_id, _af_index)                      \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_route_clear)

#define FIB_GET_CNTRS_UNKNOWN_MSG(_vrf_id, _af_index)                    \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_unknown_msg)

#define FIB_GET_CNTRS_NBR_ADD(_vrf_id, _af_index)                           \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_nbr_add)

#define FIB_GET_CNTRS_NBR_DEL(_vrf_id, _af_index)                           \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_nbr_del)

#define FIB_GET_CNTRS_NBR_UNRSLVD(_vrf_id, _af_index)                       \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_nbr_un_rslvd)

#define FIB_GET_CNTRS_NBR_CLR(_vrf_id, _af_index)                           \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_nbr_clear)

#define FIB_GET_CNTRS_FIB_HOST_ENTRIES(_vrf_id, _af_index)                   \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_fib_host_entries)

#define FIB_GET_CNTRS_FIB_ROUTE_ENTRIES(_vrf_id, _af_index)                  \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_fib_route_entries)

#define FIB_GET_CNTRS_CAM_HOST_ENTRIES(_vrf_id, _af_index)                   \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_cam_host_entries)

#define FIB_GET_CNTRS_CAM_ROUTE_ENTRIES(_vrf_id, _af_index)                  \
        ((hal_rt_access_fib_vrf_cntrs(_vrf_id, _af_index))->num_cam_route_entries)

/*
 * Function Prototypes
 */

int hal_rt_vrf_init (void);

int hal_rt_vrf_de_init (void);

int hal_rt_task_init (void);

void hal_rt_task_exit (void);

int hal_rt_default_dr_init (void);

const t_fib_config * hal_rt_access_fib_config(void);

t_fib_vrf * hal_rt_access_fib_vrf(uint32_t vrf_id);

t_fib_vrf_info * hal_rt_access_fib_vrf_info(uint32_t vrf_id, uint8_t af_index);

t_fib_vrf_cntrs * hal_rt_access_fib_vrf_cntrs(uint32_t vrf_id, uint8_t af_index);

std_rt_table * hal_rt_access_fib_vrf_dr_tree(uint32_t vrf_id, uint8_t af_index);

std_rt_table * hal_rt_access_fib_vrf_nh_tree(uint32_t vrf_id, uint8_t af_index);

std_rt_table * hal_rt_access_intf_tree(void);

void nas_l3_lock();

void nas_l3_unlock();

int hal_rt_process_peer_routing_config (uint32_t vrf_id, t_peer_routing_config *p_status);

#endif /* __HAL_RT_MAIN_H__ */
