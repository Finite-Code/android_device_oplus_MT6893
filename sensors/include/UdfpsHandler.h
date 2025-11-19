/*
 * Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include <poll.h>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <cmath>
#include <cstdio>
#include <fcntl.h>
#include <map>
#include <memory>
#include <sstream>
#include "V2_1/SubHal.h" 
#include <hardware/sensors.h>
#include <utils/SystemClock.h>
#include <log/log.h>
#include <android/hardware/sensors/2.1/types.h>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace subhal {
namespace implementation {

using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SensorFlagBits;
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::RateLevel;
using ::android::hardware::sensors::V1_0::SharedMemInfo;
using ::android::hardware::sensors::V2_1::Event;
using ::android::hardware::sensors::V2_1::SensorInfo;
using ::android::hardware::sensors::V2_1::SensorType;
using ::android::hardware::sensors::V2_1::ISensors;
using ::android::hardware::sensors::V2_1::implementation::IHalProxyCallback;
using ::android::hardware::sensors::V2_1::implementation::ISensorsSubHal;
using ::android::hardware::sensors::V2_0::implementation::ScopedWakelock;

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual const SensorInfo& getSensorInfo() const = 0;
    virtual void activate(bool enable) = 0;
};

class ISensorsEventCallback {
public:
    virtual ~ISensorsEventCallback() = default;
    virtual void postEvents(const std::vector<Event>& events, bool wakeup) = 0;
};

class UdfpsSensor {
public:
    UdfpsSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
        : mCallback(callback), mIsEnabled(false) {
        mSensorInfo.sensorHandle = sensorHandle;
        mSensorInfo.name = "UDFPS Sensor";
        mSensorInfo.vendor = "The LineageOS Project";
        mSensorInfo.version = 1;
        mSensorInfo.type = static_cast<SensorType>(static_cast<int32_t>(SensorType::DEVICE_PRIVATE_BASE) + 1);
        mSensorInfo.typeAsString = "org.lineageos.sensor.udfps";
        mSensorInfo.maxRange = 2048.0f;
        mSensorInfo.resolution = 1.0f;
        mSensorInfo.power = 0;
        mSensorInfo.fifoReservedEventCount = 0;
        mSensorInfo.fifoMaxEventCount = 0;
        mSensorInfo.minDelay = -1;
        mSensorInfo.maxDelay = 0;
        mSensorInfo.flags = SensorFlagBits::WAKE_UP | SensorFlagBits::ONE_SHOT_MODE;

        int rc = pipe(mWaitPipeFd);
        if (rc < 0) {
            mWaitPipeFd[0] = -1;
            mWaitPipeFd[1] = -1;
            ALOGE("failed to open wait pipe: %d", rc);
        }

        mPollFd = open("/sys/kernel/oplus_display/fp_state", O_RDONLY);
        if (mPollFd < 0) {
            ALOGE("failed to open poll fd: %d", mPollFd);
        }

        if (mWaitPipeFd[0] < 0 || mWaitPipeFd[1] < 0 || mPollFd < 0) {
            mStopThread = true;
            return;
        }

        mPolls[0] = { .fd = mWaitPipeFd[0], .events = POLLIN };
        mPolls[1] = { .fd = mPollFd, .events = POLLERR | POLLPRI };
    }

    ~UdfpsSensor() {
        mStopThread = true;
        interruptPoll();
        if (mThread.joinable()) mThread.join();
        if (mPollFd >= 0) close(mPollFd);
        if (mWaitPipeFd[0] >= 0) close(mWaitPipeFd[0]);
        if (mWaitPipeFd[1] >= 0) close(mWaitPipeFd[1]);
    }

    const SensorInfo& getSensorInfo() const { return mSensorInfo; }

    void activate(bool enable) {
        std::lock_guard<std::mutex> lock(mRunMutex);
        if (mIsEnabled != enable) {
            mIsEnabled = enable;
            if (enable && !mThread.joinable()) {
                 mStopThread = false;
                 mThread = std::thread(&UdfpsSensor::run, this);
            }
            interruptPoll();
        }
    }

    void interruptPoll() {
        if (mWaitPipeFd[1] < 0) return;
        char c = '1';
        write(mWaitPipeFd[1], &c, sizeof(c));
    }

    std::vector<Event> readEvents() {
        std::vector<Event> events;
        Event event;
        event.sensorHandle = mSensorInfo.sensorHandle;
        event.sensorType = mSensorInfo.type;
        event.timestamp = ::android::elapsedRealtimeNano();
        event.u.data[0] = mScreenX;
        event.u.data[1] = mScreenY;
        events.push_back(event);
        return events;
    }

    bool readFpState(int fd, int& screenX, int& screenY) {
        char buffer[512];
        int state = 0;
        int rc;

        rc = lseek(fd, 0, SEEK_SET);
        if (rc) {
            ALOGE("failed to seek: %d", rc);
            return false;
        }

        rc = read(fd, &buffer, sizeof(buffer));
        if (rc < 0) {
            ALOGE("failed to read state: %d", rc);
            return false;
        }

        buffer[sizeof(buffer) - 1] = '\0';

        rc = sscanf(buffer, "%d,%d,%d", &screenX, &screenY, &state);
        if (rc < 0) {
            ALOGE("failed to parse fp state: %d", rc);
            return false;
        }

        return state > 0;
    }

    void run() {
        while (!mStopThread) {
            if (!mIsEnabled) {
                 std::unique_lock<std::mutex> lock(mRunMutex);
                 lock.unlock();
                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
                 continue;
            }

            int rc = poll(mPolls, 2, -1);
            if (rc < 0) {
                ALOGE("failed to poll: %d", rc);
                mStopThread = true;
                continue;
            }

            if ((mPolls[1].revents & (POLLERR | POLLPRI)) == (POLLERR | POLLPRI)) {
                int x = 0, y = 0;
                if (readFpState(mPollFd, x, y)) {
                    mScreenX = x;
                    mScreenY = y;
                    
                    // Post events
                    mIsEnabled = false;
                    mCallback->postEvents(readEvents(), true /* wakeUp */);
                }
            } else if (mPolls[0].revents == mPolls[0].events) {
                char buf;
                read(mWaitPipeFd[0], &buf, sizeof(buf));
            }
        }
    }

private:
    SensorInfo mSensorInfo;
    ISensorsEventCallback* mCallback;
    bool mIsEnabled;
    std::mutex mRunMutex;
    std::thread mThread;
    bool mStopThread = false;
    
    int mWaitPipeFd[2];
    int mPollFd;
    struct pollfd mPolls[2];

    // Data storage
    int mScreenX = 0;
    int mScreenY = 0;
};

class InternalUdfpsSubHal : public ISensorsSubHal, public ISensorsEventCallback {
public:
    InternalUdfpsSubHal() : mCallback(nullptr), mNextHandle(1) { 
        std::shared_ptr<UdfpsSensor> sensor =
        std::make_shared<UdfpsSensor>(mNextHandle++ /* sensorHandle */, this /* callback */);
        mSensors[sensor->getSensorInfo().sensorHandle] = sensor;
    }

    ~InternalUdfpsSubHal() override = default;

    Return<void> getSensorsList_2_1(ISensors::getSensorsList_2_1_cb _hidl_cb) override {
        std::vector<SensorInfo> sensors;
        for (const auto& pair : mSensors) {
            sensors.push_back(pair.second->getSensorInfo());
        }
        _hidl_cb(sensors);
        return Void();
    }

    Return<Result> injectSensorData_2_1(const Event& event) override {
        (void)event;
        return Result::INVALID_OPERATION;
    }

    Return<Result> initialize(const sp<IHalProxyCallback>& halProxyCallback) override {
        mCallback = halProxyCallback;
        setOperationMode(OperationMode::NORMAL); 
        return Result::OK;
    }

    Return<Result> setOperationMode(OperationMode mode) override {
        mCurrentOperationMode = mode;
        return Result::OK; 
    }

    Return<Result> activate(int32_t sensorHandle, bool enabled) override {
        if (mSensors.count(sensorHandle)) {
            mSensors[sensorHandle]->activate(enabled);
            return Result::OK;
        }
        return Result::BAD_VALUE;
    }

    Return<Result> batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                         int64_t maxReportLatencyNs) override {
        (void)samplingPeriodNs;
        (void)maxReportLatencyNs;
        return (mSensors.count(sensorHandle)) ? Result::OK : Result::BAD_VALUE;
    }

    Return<Result> flush(int32_t sensorHandle) override { 
        if (mSensors.count(sensorHandle)) {
            Event flushEvent{};
            flushEvent.sensorHandle = sensorHandle;
            flushEvent.sensorType = SensorType::META_DATA;
            flushEvent.timestamp = ::android::elapsedRealtimeNano();
            flushEvent.u.meta.what = V1_0::MetaDataEventType::META_DATA_FLUSH_COMPLETE;
            postEvents({flushEvent}, false /* wakeUp */);
            return Result::OK;
        }
        return Result::BAD_VALUE;
    }

    Return<void> registerDirectChannel(const SharedMemInfo& mem,
                                       ISensors::registerDirectChannel_cb _hidl_cb) override {
        (void)mem;
        _hidl_cb(Result::INVALID_OPERATION, -1);
        return Void();
    }

    Return<Result> unregisterDirectChannel(int32_t channelHandle) override {
        (void)channelHandle;
        return Result::INVALID_OPERATION;
    }

    Return<void> configDirectReport(int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
                                    ISensors::configDirectReport_cb _hidl_cb) override {
        (void)sensorHandle;
        (void)channelHandle;
        (void)rate;
        _hidl_cb(Result::INVALID_OPERATION, -1);
        return Void();
    }

    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& args) override {
        if (fd.getNativeHandle() == nullptr || fd->numFds < 1) {
            return Void();
        }
        if (args.size() != 0) {
		    return Void();
        }
        return Return<void>();
    }

    const std::string getName() override { return "InternalUdfps"; }

    OperationMode getOperationMode() const { return mCurrentOperationMode; }

    void postEvents(const std::vector<Event>& events, bool wakeup) override {
        if (mCallback != nullptr) {
            ScopedWakelock wakelock = mCallback->createScopedWakelock(wakeup);
            mCallback->postEvents(events, std::move(wakelock));
        }
    }

protected:
    std::map<int32_t, std::shared_ptr<UdfpsSensor>> mSensors;

    sp<IHalProxyCallback> mCallback;

private:
    OperationMode mCurrentOperationMode = OperationMode::NORMAL;

    int32_t mNextHandle;
};

extern "C" ISensorsSubHal* HIDL_FETCH_ISensorsSubHal(const char* name) {
    if (name == nullptr || strcmp(name, "InternalUdfps") == 0) {
        return new InternalUdfpsSubHal();
    }
    return nullptr;
}

} // namespace implementation
} // namespace subhal
} // namespace V2_1
} // namespace sensors
} // namespace hardware
} // namespace android