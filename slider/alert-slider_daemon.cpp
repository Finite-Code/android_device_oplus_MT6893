/*
 * Copyright (C) 2021 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

int read_tristate() {
    int fd = open("/proc/tristatekey/tri_state", O_RDONLY);
    char p[16];
    int ret = read(fd, p, sizeof(p) - 1);
    p[ret] = 0;
    return atoi(p);
}

void vibrationHandler(const char* type) {
    // define leds
    constexpr const char* duration = "/sys/class/leds/vibrator/duration";
    constexpr const char* active = "/sys/class/leds/vibrator/activate";
    constexpr const char* motor_old = "/sys/class/leds/vibrator/motor_old";

    // store initial gain value
    char gain_val[8] = {0};
    int gain_fd = open("/sys/class/leds/vibrator/gain", O_RDONLY);
    if (gain_fd != -1) {
        ssize_t bytes_read = read(gain_fd, gain_val, sizeof(gain_val) - 1);
        if (bytes_read > 0) gain_val[bytes_read] = '\0';
        close(gain_fd);
    }

    // handle leds
    auto writeToFile = [](const char* path, const char* val) {
        int fd = open(path, O_WRONLY);
        if (fd >= 0) {
            write(fd, val, strlen(val));
            close(fd);
        }
    };

    // trigger long vibrator once
    if (strcmp(type, "long") == 0) {
        writeToFile(duration, "400");
        writeToFile(active, "1");
        usleep(400 * 1000);
        writeToFile(active, "0");
        writeToFile(duration, "0");
    }

    // trigger haptic kick twice
    else if (strcmp(type, "short") == 0) {
        writeToFile(motor_old, "1");
        usleep(200 * 1000);
        writeToFile(motor_old, "1");
        usleep(200 * 1000);
        writeToFile(motor_old, "0");
    }

    // reset leds
    writeToFile("/sys/class/leds/vibrator/gain", gain_val);
    writeToFile("/sys/class/leds/vibrator/waveform_index", "0x0a");
}

int main() {
    int fd = -1;
    for (int i = 0; i < 255; i++) {
        char path[256];
        snprintf(path, 256, "/dev/input/event%d", i);
        fd = open(path, O_RDWR);
        if (fd == -1) continue;
        char name[256];
        ioctl(fd, EVIOCGNAME(256), name);
        printf("Got input name %s\n", name);
        if (strcmp(name, "oplus,hall_tri_state_key") == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    if (fd == -1) return 0;

    ioctl(fd, EVIOCGRAB, 1);

    struct input_event ev;
    while (read(fd, &ev, sizeof(ev)) != 0) {
        if (!(ev.code == 61 && ev.value == 0)) continue;
        int state = read_tristate();
        printf("State %d\n", state);
        if (state == 1) {
            system("service call audio 49 i32 0 s16 android");
        } else if (state == 2) {
            system("service call audio 49 i32 1 s16 android");
            vibrationHandler("short");
        } else if (state == 3) {
            system("service call audio 49 i32 2 s16 android");
            vibrationHandler("long");
        }
    }
}
