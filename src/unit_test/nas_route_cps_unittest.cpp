
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
 * nas_rt_cps_api_unittest.cpp
 *
 *  Created on: May 20, 2015
 *      Author: Satish Mynam
 */



#include "std_mac_utils.h"
#include "std_ip_utils.h"

#include "dell-base-routing.h"
#include "nas_rt_api.h"
#include "ds_common_types.h"
#include "cps_class_map.h"
#include "cps_api_object.h"
#include "cps_api_operation.h"
#include "cps_class_map.h"
#include "cps_api_object_key.h"



#include <gtest/gtest.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

cps_api_object_list_t list_of_objects;

TEST(std_nas_route_test, nas_route_add) {

    cps_api_object_t obj = cps_api_object_create();

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
           BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);

    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
       //     cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    /*
     * Check mandatory route attributes
     *  BASE_ROUTE_OBJ_ENTRY_AF,     BASE_ROUTE_OBJ_ENTRY_VRF_ID);
     * BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,   BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN;
     */

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_VRF_ID,0);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);

    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.6",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_attr_id_t ids[3];
    const int ids_len = sizeof(ids)/sizeof(*ids);
    ids[0] = BASE_ROUTE_OBJ_ENTRY_NH_LIST;
    ids[1] = 0;
    ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR;

    /*
     * Set Loopback0 NH
     */
    inet_aton("127.0.0.1",&a);
    ip=a.s_addr;
    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
                    &ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_NH_COUNT,1);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_create(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);

}

TEST(std_nas_route_test, nas_route_set) {

    cps_api_object_t obj = cps_api_object_create();
    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
      //      cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
              BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);

    /*
     * Check mandatory route attributes
     *  BASE_ROUTE_OBJ_ENTRY_AF,     BASE_ROUTE_OBJ_ENTRY_VRF_ID);
     * BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,   BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN;
     * For NH: BASE_ROUTE_OBJ_ENTRY_NH_COUNT, BASE_ROUTE_OBJ_ENTRY_NH_LIST,
     * BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR
     */

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_VRF_ID,0);

    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.6",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_attr_id_t ids[3];
    const int ids_len = sizeof(ids)/sizeof(*ids);
    ids[0] = BASE_ROUTE_OBJ_ENTRY_NH_LIST;
    ids[1] = 0;
    ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR;

    inet_aton("127.0.0.2",&a);
    ip=a.s_addr;
    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
                    &ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_NH_COUNT,1);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_set(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);

}


TEST(std_nas_route_test, nas_neighbor_add) {

    cps_api_object_t obj = cps_api_object_create();
    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
    //        cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
              BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_AF,AF_INET);

    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.6",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,&ip,sizeof(ip));
    int port_index = if_nametoindex("e101-001-0");
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_IFINDEX, port_index);

    hal_mac_addr_t mac_addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    cps_api_object_attr_add(obj, BASE_ROUTE_OBJ_NBR_MAC_ADDR, &mac_addr, HAL_MAC_ADDR_LEN);


    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_create(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);

}

TEST(std_nas_route_test, nas_neighbor_set) {

    cps_api_object_t obj = cps_api_object_create();
    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
     //       cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
              BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_AF,AF_INET);

    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.6",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,&ip,sizeof(ip));
    int port_index = if_nametoindex("e101-001-0");
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_IFINDEX, port_index);

    hal_mac_addr_t mac_addr = {0x00, 0x00, 0x00, 0xaa, 0xbb, 0xcc};

    cps_api_object_attr_add(obj, BASE_ROUTE_OBJ_NBR_MAC_ADDR, &mac_addr, HAL_MAC_ADDR_LEN);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_set(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);
}

TEST(std_nas_route_test, nas_neighbor_delete) {

    cps_api_object_t obj = cps_api_object_create();
    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
     //       cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
              BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_AF,AF_INET);

    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.6",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,&ip,sizeof(ip));
    int port_index = if_nametoindex("e101-001-0");
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_IFINDEX, port_index);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_delete(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);
}

TEST(std_nas_route_test, nas_neighbor_add_static) {

    cps_api_object_t obj = cps_api_object_create();
    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
    //        cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
              BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_AF,AF_INET);

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_TYPE,BASE_ROUTE_RT_TYPE_STATIC);
    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.7",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,&ip,sizeof(ip));
    int port_index = if_nametoindex("e101-001-0");
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_IFINDEX, port_index);

    hal_mac_addr_t mac_addr = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};

    cps_api_object_attr_add(obj, BASE_ROUTE_OBJ_NBR_MAC_ADDR, &mac_addr, HAL_MAC_ADDR_LEN);


    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_create(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);

}

TEST(std_nas_route_test, nas_neighbor_set_static) {

    cps_api_object_t obj = cps_api_object_create();
    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
     //       cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
              BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_AF,AF_INET);

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_TYPE,BASE_ROUTE_RT_TYPE_STATIC);
    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.7",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,&ip,sizeof(ip));
    int port_index = if_nametoindex("e101-001-0");
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_IFINDEX, port_index);

    hal_mac_addr_t mac_addr = {0x00, 0x00, 0x11, 0xaa, 0xbb, 0xcc};

    cps_api_object_attr_add(obj, BASE_ROUTE_OBJ_NBR_MAC_ADDR, &mac_addr, HAL_MAC_ADDR_LEN);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_set(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);
}

TEST(std_nas_route_test, nas_neighbor_delete_static) {

    cps_api_object_t obj = cps_api_object_create();
    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
     //       cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
              BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_AF,AF_INET);

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_TYPE,BASE_ROUTE_RT_TYPE_STATIC);
    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.7",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,&ip,sizeof(ip));
    int port_index = if_nametoindex("e101-001-0");
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_IFINDEX, port_index);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_delete(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);
}


TEST(std_nas_route_test, nas_route_delete) {

    cps_api_object_t obj = cps_api_object_create();
    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
    //        cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
              BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_VRF_ID,0);

    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.6",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_delete(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);

}

TEST(std_nas_route_test, nas_route_mp_add) {

    cps_api_object_t obj = cps_api_object_create();

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
           BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);

    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
       //     cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    /*
     * Check mandatory route attributes
     *  BASE_ROUTE_OBJ_ENTRY_AF,     BASE_ROUTE_OBJ_ENTRY_VRF_ID);
     * BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,   BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN;
     */

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_VRF_ID,0);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);

    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.6",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_attr_id_t ids[3];
    const int ids_len = sizeof(ids)/sizeof(*ids);
    ids[0] = BASE_ROUTE_OBJ_ENTRY_NH_LIST;
    ids[1] = 0;
    ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR;

    /*
     * Set Loopback0 NH
     */
    inet_aton("4.4.4.2",&a);
    ip=a.s_addr;
    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
                    &ip,sizeof(ip));

    ids[1] = 1;
    inet_aton("1.1.1.2",&a);
    ip=a.s_addr;
    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
            &ip,sizeof(ip));

    ids[1] = 2;
    inet_aton("2.2.2.2",&a);
    ip=a.s_addr;
    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
            &ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_NH_COUNT,3);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_create(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);
    printf("___________________________________________\n");
    system("ip route show 6.6.6.6");
    printf("___________________________________________\n");
}

TEST(std_nas_route_test, nas_route_mp_set) {

    cps_api_object_t obj = cps_api_object_create();
    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
      //      cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
              BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);

    /*
     * Check mandatory route attributes
     *  BASE_ROUTE_OBJ_ENTRY_AF,     BASE_ROUTE_OBJ_ENTRY_VRF_ID);
     * BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,   BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN;
     * For NH: BASE_ROUTE_OBJ_ENTRY_NH_COUNT, BASE_ROUTE_OBJ_ENTRY_NH_LIST,
     * BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR
     */

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_VRF_ID,0);

    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.6",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_attr_id_t ids[3];
    const int ids_len = sizeof(ids)/sizeof(*ids);
    ids[0] = BASE_ROUTE_OBJ_ENTRY_NH_LIST;
    ids[1] = 0;
    ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR;

    inet_aton("4.4.4.3",&a);
    ip=a.s_addr;
    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
                    &ip,sizeof(ip));
    ids[1] = 1;
    inet_aton("1.1.1.3",&a);
    ip=a.s_addr;
    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
            &ip,sizeof(ip));

    ids[1] = 2;
    inet_aton("2.2.2.3",&a);
    ip=a.s_addr;
    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
            &ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_NH_COUNT,3);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_set(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);

    printf("___________________________________________\n");
    system("ip route show 6.6.6.6");
    printf("___________________________________________\n");

}


TEST(std_nas_route_test, nas_route_mp_delete) {

    cps_api_object_t obj = cps_api_object_create();
    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
    //        cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
              BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_VRF_ID,0);

    uint32_t ip;
    struct in_addr a;
    inet_aton("6.6.6.6",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_delete(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);
    printf("___________________________________________\n");
    system("ip route show 6.6.6.6");
    printf("___________________________________________\n");
}

//Scale tests for route add/delete
TEST(std_nas_route_test, nas_route_add_scale) {
    char ip_addr[256];
    int i=1, j=1 ,count=0;
    uint32_t ip;
    struct in_addr a;


   for(i=1; i<101; i++) {
       for (j=1; j<251; j++) {

    cps_api_object_t obj = cps_api_object_create();

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
           BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);

    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
       //     cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    /*
     * Check mandatory route attributes
     *  BASE_ROUTE_OBJ_ENTRY_AF,     BASE_ROUTE_OBJ_ENTRY_VRF_ID);
     * BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,   BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN;
     */

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_VRF_ID,0);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);


    snprintf(ip_addr,256, "11.10.%d.%d",i,j);
   // printf ("Add Route:%s, NH:1.1.1.4\n", ip_addr);

    inet_aton(ip_addr,&a);
    //inet_aton("6.6.6.6",&a);
    ip=a.s_addr;
    //printf ("Route:%d\n", ip);
    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_attr_id_t ids[3];
    const int ids_len = sizeof(ids)/sizeof(*ids);
    ids[0] = BASE_ROUTE_OBJ_ENTRY_NH_LIST;
    ids[1] = 0;
    ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR;

    /*
     * Set  NH
     */
    inet_aton("1.1.1.4",&a);
    ip=a.s_addr;
    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
                    &ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_NH_COUNT,1);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_create(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);
    count++;

       }
   }
printf("Sent %d Routes\n", count);
}

TEST(std_nas_route_test, nas_route_delete_scale) {

    char ip_addr[256];
    int i=1, j=1;
    uint32_t ip;
    struct in_addr a;



    for(i=1; i<65; i++) {
        for (j=1; j<251; j++) {

    cps_api_object_t obj = cps_api_object_create();
    //cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
    //        cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
              BASE_ROUTE_OBJ_OBJ,cps_api_qualifier_TARGET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_VRF_ID,0);


    snprintf(ip_addr,256, "11.10.%d.%d",i,j);
    //printf ("Delete Route:%s \n", ip_addr);

    inet_aton(ip_addr,&a);
    //inet_aton("6.6.6.6",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_delete(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);
        }
    }

}

void nas_route_dump_arp_object_content(cps_api_object_t obj){
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    char str[INET_ADDRSTRLEN];


    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {

        switch (cps_api_object_attr_id(it.attr)) {

        case BASE_ROUTE_OBJ_NBR_ADDRESS:
            std::cout<<"IP Address "<<inet_ntop(AF_INET,cps_api_object_attr_data_bin(it.attr),str,INET_ADDRSTRLEN)<<std::endl;
            break;

        case BASE_ROUTE_OBJ_NBR_MAC_ADDR:
        {
              char mt[6];
              char mstring[20];
              memcpy(mt, cps_api_object_attr_data_bin(it.attr), 6);
              sprintf(mstring, "%x:%x:%x:%x:%x:%x", mt[0], mt[1], mt[2], mt[3], mt[4], mt[5]);
              std::cout<<"MAC "<<mstring<<std::endl;
        }
            break;

        case BASE_ROUTE_OBJ_NBR_VRF_ID:
            std::cout<<"VRF Id "<<cps_api_object_attr_data_u32(it.attr)<<std::endl;
            break;

        case BASE_ROUTE_OBJ_NBR_IFINDEX:
            std::cout<<"Ifindex "<<cps_api_object_attr_data_u32(it.attr)<<std::endl;
            break;

        case BASE_ROUTE_OBJ_NBR_FLAGS:
            std::cout<<"Flags "<<cps_api_object_attr_data_u32(it.attr)<<std::endl;
            break;

        case BASE_ROUTE_OBJ_NBR_STATE:
            std::cout<<"State "<<cps_api_object_attr_data_u32(it.attr)<<std::endl;
            break;

        case BASE_ROUTE_OBJ_NBR_TYPE:
            std::cout<<"Type "<<cps_api_object_attr_data_u32(it.attr)<<std::endl;
            break;

        default:
            break;
        }
    }
}

TEST(std_nas_route_test, nas_route_arp_get) {
    cps_api_get_params_t gp;
    cps_api_get_request_init(&gp);

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_ROUTE_OBJ_NBR,
                                        cps_api_qualifier_TARGET);
    unsigned short af = 2;
    cps_api_set_key_data(obj,BASE_ROUTE_OBJ_NBR_AF,cps_api_object_ATTR_T_U16,
                                 &af,sizeof(af));

    if (cps_api_get(&gp)==cps_api_ret_code_OK) {
        size_t mx = cps_api_object_list_size(gp.list);

        for ( size_t ix = 0 ; ix < mx ; ++ix ) {
            obj = cps_api_object_list_get(gp.list,ix);
            std::cout<<"ARP ENTRY "<<std::endl;
            std::cout<<"================================="<<std::endl;
            nas_route_dump_arp_object_content(obj);
            std::cout<<std::endl;
        }
    }

    cps_api_get_request_close(&gp);

}

TEST(std_nas_route_test, nas_peer_routing_config_enable) {
    cps_api_object_t obj = cps_api_object_create();

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_ROUTE_PEER_ROUTING_CONFIG_OBJ,cps_api_qualifier_TARGET);

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_PEER_ROUTING_CONFIG_VRF_ID,0);

    hal_mac_addr_t mac_addr = {0x01, 0x12, 0x13, 0x14, 0x15, 0x16};

    cps_api_object_attr_add(obj, BASE_ROUTE_PEER_ROUTING_CONFIG_PEER_MAC_ADDR, &mac_addr, HAL_MAC_ADDR_LEN);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_create(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);
}

TEST(std_nas_route_test, nas_peer_routing_config_set) {
    cps_api_object_t obj = cps_api_object_create();

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_ROUTE_PEER_ROUTING_CONFIG_OBJ,cps_api_qualifier_TARGET);

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_PEER_ROUTING_CONFIG_VRF_ID,0);

    hal_mac_addr_t mac_addr = {0x01, 0x22, 0x23, 0x24, 0x25, 0x26};

    cps_api_object_attr_add(obj, BASE_ROUTE_PEER_ROUTING_CONFIG_PEER_MAC_ADDR, &mac_addr, HAL_MAC_ADDR_LEN);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_set(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);
}


TEST(std_nas_route_test, nas_peer_routing_config_disable) {
    cps_api_object_t obj = cps_api_object_create();

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_ROUTE_PEER_ROUTING_CONFIG_OBJ,cps_api_qualifier_TARGET);

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_PEER_ROUTING_CONFIG_VRF_ID,0);

    hal_mac_addr_t mac_addr = {0x01, 0x12, 0x13, 0x14, 0x15, 0x16};

    cps_api_object_attr_add(obj, BASE_ROUTE_PEER_ROUTING_CONFIG_PEER_MAC_ADDR, &mac_addr, HAL_MAC_ADDR_LEN);

    /*
     * CPS transaction
     */
    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_delete(&tr,obj);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);

}

void nas_route_dump_peer_routing_object_content(cps_api_object_t obj){
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);

    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {

        switch (cps_api_object_attr_id(it.attr)) {

        case BASE_ROUTE_PEER_ROUTING_CONFIG_VRF_ID:
            std::cout<<"VRF Id "<<cps_api_object_attr_data_u32(it.attr)<<std::endl;
            break;

        case BASE_ROUTE_PEER_ROUTING_CONFIG_PEER_MAC_ADDR:
        {
              char mt[6];
              char mstring[20];
              memcpy(mt, cps_api_object_attr_data_bin(it.attr), 6);
              sprintf(mstring, "%x:%x:%x:%x:%x:%x", mt[0], mt[1], mt[2], mt[3], mt[4], mt[5]);
              std::cout<<"MAC "<<mstring<<std::endl;
        }
            break;

        default:
            break;
        }
    }
}


TEST(std_nas_route_test, nas_peer_routing_config_get) {
    cps_api_get_params_t gp;
    cps_api_get_request_init(&gp);

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_ROUTE_PEER_ROUTING_CONFIG_OBJ,
                                        cps_api_qualifier_TARGET);

    if (cps_api_get(&gp)==cps_api_ret_code_OK) {
        size_t mx = cps_api_object_list_size(gp.list);

        for ( size_t ix = 0 ; ix < mx ; ++ix ) {
            obj = cps_api_object_list_get(gp.list,ix);
            std::cout<<"Peer Routing Status "<<std::endl;
            std::cout<<"================================="<<std::endl;
            nas_route_dump_peer_routing_object_content(obj);
            std::cout<<std::endl;
        }
    }

    cps_api_get_request_close(&gp);

}

void nas_route_dump_route_object_content(cps_api_object_t obj) {

    char str[INET6_ADDRSTRLEN];
    char if_name[IFNAMSIZ];
    uint32_t addr_len = 0, af_data = 0;
    uint32_t nhc = 0, nh_itr = 0;

    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);

    cps_api_object_attr_t af       = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_AF);
    af_data = cps_api_object_attr_data_u32(af) ;

    addr_len = ((af_data == AF_INET) ? HAL_INET4_LEN : HAL_INET6_LEN);

    cps_api_object_attr_t prefix   = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX);
    cps_api_object_attr_t pref_len = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN);
    cps_api_object_attr_t nh_count = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_NH_COUNT);
    std::cout<<"AF "<<((af_data == AF_INET) ? "IPv4" : "IPv6")<<","<<
        inet_ntop(af_data, cps_api_object_attr_data_bin(prefix), str,addr_len)<<"/"<<
        cps_api_object_attr_data_u32(pref_len)<<std::endl;
    if (nh_count != CPS_API_ATTR_NULL) {
        nhc = cps_api_object_attr_data_u32(nh_count);
        std::cout<<"NHC "<<nhc<<std::endl;
    }

    for (nh_itr = 0; nh_itr < nhc; nh_itr++)
    {
        cps_api_attr_id_t ids[3] = { BASE_ROUTE_OBJ_ENTRY_NH_LIST,
            0, BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR};
        const int ids_len = sizeof(ids)/sizeof(*ids);
        ids[1] = nh_itr;

        cps_api_object_attr_t attr = cps_api_object_e_get(obj,ids,ids_len);
        if (attr != CPS_API_ATTR_NULL)
            std::cout<<"NextHop "<<inet_ntop(af_data,cps_api_object_attr_data_bin(attr),str,addr_len)<<std::endl;

        ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_IFINDEX;
        attr = cps_api_object_e_get(obj,ids,ids_len);
        if (attr != CPS_API_ATTR_NULL)
            if_indextoname((int)cps_api_object_attr_data_u32(attr), if_name);
        std::cout<<"IfIndex "<<if_name<<"("<<cps_api_object_attr_data_u32(attr)<<")"<<std::endl;

        ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_WEIGHT;
        attr = cps_api_object_e_get(obj,ids,ids_len);
        if (attr != CPS_API_ATTR_NULL)
            std::cout<<"Weight "<<cps_api_object_attr_data_u32(attr)<<std::endl;

        ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_RESOLVED;
        attr = cps_api_object_e_get(obj,ids,ids_len);
        if (attr != CPS_API_ATTR_NULL)
            std::cout<<"Is Next Hop Resolved "<<cps_api_object_attr_data_u32(attr)<<std::endl;
    }
}


TEST(std_nas_route_test, nas_route_ipv4_get) {
    cps_api_get_params_t gp;
    cps_api_get_request_init(&gp);

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_ROUTE_OBJ_ENTRY,
                                        cps_api_qualifier_TARGET);
    unsigned int af = AF_INET;
    cps_api_set_key_data(obj,BASE_ROUTE_OBJ_ENTRY_AF,cps_api_object_ATTR_T_U32,
                                 &af,sizeof(af));

    if (cps_api_get(&gp)==cps_api_ret_code_OK) {
        size_t mx = cps_api_object_list_size(gp.list);

        std::cout<<"IPv4 FIB Entries"<<std::endl;
        std::cout<<"================================="<<std::endl;
        for ( size_t ix = 0 ; ix < mx ; ++ix ) {
            obj = cps_api_object_list_get(gp.list,ix);
            nas_route_dump_route_object_content(obj);
            std::cout<<std::endl;
        }
    }
    cps_api_get_request_close(&gp);
}

TEST(std_nas_route_test, nas_route_ipv6_get) {
    cps_api_get_params_t gp;
    cps_api_get_request_init(&gp);

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_ROUTE_OBJ_ENTRY,
                                        cps_api_qualifier_TARGET);
    unsigned int af = AF_INET6;
    cps_api_set_key_data(obj,BASE_ROUTE_OBJ_ENTRY_AF,cps_api_object_ATTR_T_U32,
                                 &af,sizeof(af));

    if (cps_api_get(&gp)==cps_api_ret_code_OK) {
        size_t mx = cps_api_object_list_size(gp.list);

        std::cout<<"IPv6 FIB Entries"<<std::endl;
        std::cout<<"================================="<<std::endl;
        for ( size_t ix = 0 ; ix < mx ; ++ix ) {
            obj = cps_api_object_list_get(gp.list,ix);
            nas_route_dump_route_object_content(obj);
            std::cout<<std::endl;
        }
    }
    cps_api_get_request_close(&gp);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);


  printf("___________________________________________\n");
  system("ip address add 6.6.6.1/24  dev e00-10");
  system("ifconfig e00-10");
  printf("___________________________________________\n");
  printf("Run the NAS CPS routing test cases the below order: "
          "route add, route set, nbr add, nbr set, nbr deleet, route delete\n");
  return RUN_ALL_TESTS();
}
