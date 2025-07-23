#!/bin/bash

# Patch wpa_supplicant
cd external/wpa_supplicant_8
git fetch https://github.com/liwhy1/android_external_wpa_supplicant_8 bab5ea1f459ad1f4068dea5702839ce02373b067
git cherry-pick bab5ea1f459ad1f4068dea5702839ce02373b067
cd -