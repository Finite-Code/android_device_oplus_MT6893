#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)
from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)

namespace_imports = [
    'device/oplus/MT6893',
    'hardware/google/interfaces',
    'hardware/google/pixel',
    'hardware/mediatek',
    'hardware/mediatek/libmtkperf_client',
]

blob_fixups: blob_fixups_user_type = {
    'vendor/bin/hw/vendor.mediatek.hardware.pq@2.2-service': blob_fixup()
        .replace_needed('libutils.so', 'libutils_v32.so')
        .replace_needed('libhidlbase.so', 'libhidlbase_v32.so'),
    'vendor/lib*/hw/audio.primary.mt6893.so': blob_fixup()
        .replace_needed('libalsautils.so', 'libalsautils-v31.so')
        .replace_needed('libtinyalsa.so', 'libtinyalsa-v32.so'),
    ('vendor/bin/hw/android.hardware.media.c2@1.2-mediatek',
    'vendor/bin/hw/android.hardware.media.c2@1.2-mediatek-64b'): blob_fixup()
        .add_needed('libcodec2_hidl@1.0.so')
        .add_needed('libshim.so')
        .add_needed('libstagefright_foundation_v33.so'),
    'vendor/bin/mtk_agpsd': blob_fixup()
        .replace_needed('libcrypto.so', 'libcrypto-v32.so')
        .replace_needed('libssl.so', 'libssl-v32.so'),
    'vendor/lib64/libmtkcam_featurepolicy.so': blob_fixup()
        .sig_replace('34 E8 87 40 B9', '34 28 02 80 52'),
    'vendor/bin/hw/android.hardware.wifi@1.0-service-lazy': blob_fixup()
        .replace_needed('libwifi-hal.so', 'libwifi-hal-mtk.so'),
    'lib64/libem_support_jni.so': blob_fixup()
        .add_needed('libjni_shim.so'),
    'vendor/lib64/hw/sensors.mt6893.so': blob_fixup()
        .add_needed('libsensors_shim.so'),
    'vendor/bin/hw/mtkfusionrild': blob_fixup()
        .add_needed('libutils-v32.so'),
    'vendor/lib64/libaalservice.so': blob_fixup()
        .replace_needed('libsensorndkbridge.so', 'libsensorndkbridge-v30.so'),
    'vendor/lib/libcodec2_vndk-mtk.so': blob_fixup()
        .add_needed('libshim_ui.so'),
    ('vendor/bin/hw/android.hardware.neuralnetworks@1.3-service-mtk-neuron',
    'vendor/lib*/libnvram.so',
    'odm/bin/hw/vendor.oplus.hardware.charger@1.0-service',
    'vendor/lib64/libsysenv.so'): blob_fixup()
        .add_needed('libbase_shim.so'),
    ('vendor/lib64/hw/hwcomposer.mt6893.so', 
    'vendor/lib64/libutils_v32.so'): blob_fixup()
        .add_needed('libprocessgroup_shim.so'),
    'vendor/lib*/libmtkcam_stdutils.so': blob_fixup()
        .replace_needed('libutils.so', 'libutils_v32.so'),
    'vendor/bin/hw/camerahalserver': blob_fixup()
        .replace_needed('libutils.so', 'libutils_v32.so')
        .replace_needed('libbinder.so', 'libbinder_v32.so')
        .replace_needed('libhidlbase.so', 'libhidlbase_v32.so'),
    'vendor/lib64/hw/android.hardware.camera.provider@2.6-impl-mediatek.so': blob_fixup()
        .add_needed('libcamera_metadata_shim.so'),
    'odm/lib*/libui_oplus.so': blob_fixup()
        .replace_needed('android.hardware.graphics.common-V2-ndk_platform.so', 'android.hardware.graphics.common-V2-ndk.so'),
    'vendor/lib*/libmtkisp_metadata.so': blob_fixup()
        .replace_needed('libui.so', 'libui_oplus.so'),
    'vendor/lib64/libcam.utils.sensorprovider.so': blob_fixup()
        .replace_needed('libsensorndkbridge.so', 'libsensorndkbridge-v30.so'),
    'vendor/lib64/libsensor_custom.so': blob_fixup()
        .binary_regex_replace(b'android.sensor.wise_light', b'android.sensor.light\x00\x00\x00\x00\x00')
        .sig_replace('5B 00 01 00', '05 00 00 00'),
}  # fmt: skip

module = ExtractUtilsModule(
    'MT6893',
    'oplus',
    namespace_imports=namespace_imports,
    blob_fixups=blob_fixups,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
