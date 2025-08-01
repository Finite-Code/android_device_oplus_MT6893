#!/bin/bash

# Patch wpa_supplicant
cd external/wpa_supplicant_8
git fetch https://github.com/liwhy1/android_external_wpa_supplicant_8 bab5ea1f459ad1f4068dea5702839ce02373b067
git cherry-pick bab5ea1f459ad1f4068dea5702839ce02373b067
cd -

# Patch keyguard
cd frameworks/base/packages/SystemUI/src/com/android/systemui/keyguard
git fetch https://github.com/liwhy1/android_frameworks_base 9104d26ac6b0e4456f571d26ec3b930e5946b899
git cherry-pick 9104d26ac6b0e4456f571d26ec3b930e5946b899
cd -
