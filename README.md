# sonic-nas-l3
This repo contains the Layer 3 (L3) portion of the network abstraction service (NAS). This handles routes equal cost multi-path (ECMP) and non-ECMP, and neighbor programming for IPv4 and IPv6 addressing. 

## Build
See [sonic-nas-manifest](https://github.com/Azure/sonic-nas-manifest) for more information on common build tools.

### Build requirements
* `sonic-base-model`
* `sonic-common`
* `sonic-nas-common`
* `sonic-object-library`
* `sonic-logging`
* `sonic-nas-ndi`
* `sonic-nas-ndi-api`
* `sonic-nas-linux`

### Dependent packages
* `libsonic-logging-dev`
* `libsonic-logging1` 
* `libsonic-model1`
* `libsonic-model-dev`
* `libsonic-common1`
* `libsonic-common-dev`
* `libsonic-object-library1`
* `libsonic-object-library-dev`
* `sonic-sai-api-dev`
* `libsonic-nas-common1`
* `libsonic-nas-common-dev`
* `sonic-ndi-api-dev`
* `libsonic-nas-ndi1`
* `libsonic-nas-ndi-dev` 
* `libsonic-nas-linux1`
* `libsonic-nas-linux-dev`
* `libsonic-sai-common1` 
* `libsonic-sai-common-utils1`

BUILD CMD: sonic_build --dpkg libsonic-logging-dev libsonic-logging-dev libsonic-logging1 libsonic-model1 libsonic-model-dev libsonic-common1 libsonic-common-dev libsonic-object-library1 libsonic-object-library-dev sonic-sai-api-dev libsonic-nas-common1 libsonic-nas-common-dev sonic-ndi-api-dev libsonic-nas-ndi1 libsonic-nas-ndi-dev libsonic-nas-linux1 libsonic-nas-linux-dev --apt libsonic-sai-common1 libsonic-sai-common-utils1 -- clean binary

(c) Dell 2016
