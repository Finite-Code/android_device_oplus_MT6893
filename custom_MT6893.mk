#
# SPDX-FileCopyrightText: Android Open Source Project
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit from device makefile
$(call inherit-product, device/oplus/MT6893/device.mk)

# Inherit some common Lineage stuff.
# $(call inherit-product, vendor/custom/config/common_full_phone.mk)

# Device identifier. This must come after all inclusions.
PRODUCT_NAME := custom_MT6893
PRODUCT_DEVICE := MT6893
PRODUCT_BRAND := Oplus
PRODUCT_MODEL := MT6893
PRODUCT_MANUFACTURER := Oplus

# Build info
PRODUCT_BUILD_PROP_OVERRIDES := BuildDesc=$(call normalize-path-list, "sys_mssi_64_cn_armv82-user-13-TP1A.220905.001-1677828988354-release-keys")
PRODUCT_GMS_CLIENTID_BASE := android-oplus
