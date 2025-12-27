#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

import common

def FullOTA_InstallBegin(info):
  info.script.AppendExtra("ifelse(is_mounted(\"/apex\"), unmount(\"/apex\"));")
  return

def FullOTA_InstallEnd(info):
  OTA_InstallEnd(info, False)

def IncrementalOTA_InstallEnd(info):
  OTA_InstallEnd(info, True)

def AddImageOnly(info, basename, incremental, firmware):
  if incremental:
    input_zip = info.source_zip
  else:
    input_zip = info.input_zip
  if firmware:
    data = input_zip.read("RADIO/" + basename)
  else:
    data = input_zip.read("IMAGES/" + basename)
  common.ZipWriteStr(info.output_zip, basename, data)

def AddImage(info, basename, dest, incremental):
  AddImageOnly(info, basename, incremental, False)
  info.script.Print("Patching {} image unconditionally...".format(dest.split('/')[-1]))
  info.script.AppendExtra('package_extract_file("%s", "%s");' % (basename, dest))

def OTA_InstallEnd(info, incremental):
  AddImage(info, "dtbo.img", "/dev/block/by-name/dtbo", incremental)
  AddImage(info, "vbmeta.img", "/dev/block/by-name/vbmeta", incremental)
  AddImage(info, "vbmeta_system.img", "/dev/block/by-name/vbmeta_system", incremental)
  AddImage(info, "vbmeta_vendor.img", "/dev/block/by-name/vbmeta_vendor", incremental)
