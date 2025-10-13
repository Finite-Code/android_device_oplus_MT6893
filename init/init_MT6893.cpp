//
// Copyright (C) 2024-2025 The LineageOS Project
//
// SPDX-License-Identifier: Apache-2.0
//

#include <fcntl.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include "vendor_init.h"
#include "property_service.h"

using android::base::ReadFileToString;
using android::base::Trim;
using std::string;

std::vector<string> ro_props_default_source_order = {
    "",
    "bootimage.",
    "odm.",
    "product.",
    "system.",
    "system_ext.",
    "vendor.",
};

void property_override(const char* prop, const char* value, bool add = true) {
    prop_info* pi = (prop_info*)__system_property_find(prop);
    if (pi)
        __system_property_update(pi, value, strlen(value));
    else if (add)
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

void set_ro_build_prop(const string& prop, const string& value, bool product = true) {
    string prop_name;
    for (const auto& source : ro_props_default_source_order) {
        if (product)
            prop_name = "ro.product." + source + prop;
        else
            prop_name = "ro." + source + "build." + prop;

        property_override(prop_name.c_str(), value.c_str());
    }
}

void vendor_load_properties() {
    const char* prj_file = "/proc/oplusVersion/prjName";
    string prj_name;
    string device;
    string model;
    string fingerprint;

    if (ReadFileToString(prj_file, &prj_name)) {
        prj_name = Trim(prj_name);

        if (prj_name == "20662") {
            device = "cupida";
            model = "Realme X7 Max 5G";
            fingerprint = "realme/RMX3031/RMX3031L1:13/TP1A.220905.001/R.ead5d5-5fba:user/release-keys";
        }
        else if (prj_name == "20827" || prj_name == "20831") {
            device = "denniz";
            model = "OnePlus Nord 2 5G";
            fingerprint = "OnePlus/DN2103EEA/OP515BL1:13/TP1A.220905.001/R.108b2c1-1:user/release-keys";
        }
        else {
            device = "mt6893";
            model = "mt6893";
            fingerprint = "lineage/lineage_mt6893/mt6893:13/TP1A.220905.001/eng.user.20230710.184518:userdebug/test-keys";
            LOG(WARNING) << "Unknown prjName: " << prj_name;
        }
    }
    else {
        LOG(ERROR) << "Unable to read prjName from " << prj_file;
        return;
    }

    // Apply build fingerprint globally
    if (!fingerprint.empty()) {
        property_override("ro.build.fingerprint", fingerprint.c_str());
        for (const auto& source : ro_props_default_source_order) {
            string prop = "ro." + source + "build.fingerprint";
            property_override(prop.c_str(), fingerprint.c_str());
        }
    }

    // Set device and model props
    set_ro_build_prop("device", device);
    set_ro_build_prop("model", model);
    set_ro_build_prop("name", model);
    set_ro_build_prop("product", model, false);

    // Optional: global ro.device override
    property_override("ro.vendor.device", device.c_str());

    // Set gamma conversion prop for denniz
    if (device == "denniz") {
        property_override("sys.brightness.disable_gamma_conversion", "0");
    }
}
