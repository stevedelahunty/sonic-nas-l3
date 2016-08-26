#!/usr/bin/python
# -*- coding: utf-8 -*-

#
# Copyright (c) 2015 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#
from sos.plugins import Plugin, DebianPlugin
from subprocess import Popen, PIPE
import os, subprocess

class DN_nas_l3(Plugin, DebianPlugin):
    """ Collects nas-l3 debugging information     
    """

    plugin_name = os.path.splitext(os.path.basename(__file__))[0]
    profiles = ('networking', 'dn')

    def setup(self):
        self.add_cmd_output("/sbin/ip route")
        self.add_cmd_output("/sbin/ip -6 route")
        self.add_cmd_output("/sbin/ip neigh")
        
        proc = subprocess.Popen(['ps','-Af'],stdout=subprocess.PIPE)
        stdout_value = proc.communicate()
        if "hshsell" not in stdout_value[:]:
            self.add_cmd_output("/opt/dell/os10/bin/hshell -c 'l3 defip show'")
            self.add_cmd_output("/opt/dell/os10/bin/hshell -c 'l3 ip6route show'")
            self.add_cmd_output("/opt/dell/os10/bin/hshell -c 'l3 l3 show'")
            self.add_cmd_output("/opt/dell/os10/bin/hshell -c 'l3 egress show'")
            self.add_cmd_output("/opt/dell/os10/bin/hshell -c 'l3 multipath show'")
            self.add_cmd_output("/opt/dell/os10/bin/hshell -c 'l3 ip6host show'")
            
        else:
            self.add_alert("hshell is already running, hence unable to collect diagnostics")