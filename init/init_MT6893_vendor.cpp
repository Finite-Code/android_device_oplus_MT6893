//
// Copyright (C) 2025 The LineageOS Project
//
// SPDX-License-Identifier: Apache-2.0
//

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <android-base/properties.h>
#include <string>
#include <iostream>

int main() {
    std::string device = android::base::GetProperty("ro.vendor.device", "");
    std::string target;

    if (device == "cupida") {
        target = "/odm/firmware/tfa98xx_t0.cnt";
    } else {
        target = "/odm/firmware/tfa98xx.cnt";
    }

    std::string firmwareDir = "/mnt/odm_fw/firmware";
    std::string symlinkPath = firmwareDir + "/tfa98xx.cnt";

    unlink(symlinkPath.c_str());  // Remove old symlink if exists
    if (symlinkat(target.c_str(), AT_FDCWD, symlinkPath.c_str()) != 0) {
        perror("symlinkat failed");
        return 1;
    }

    return 0;
}
