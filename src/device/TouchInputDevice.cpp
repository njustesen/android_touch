//
// Created by kunaldawn on 12/10/17.
//

#include "TouchInputDevice.h"

std::shared_ptr<android_touch::TouchInputDevice> android_touch::TouchInputDevice::getNewInstance() {
    std::vector<std::string> inputDeviceFileNames = FileUtils::getFiles(INPUT_DEVICE_ROOT);

    int maxInputDeviceScore = -1;
    std::string maxInputDevicePath;
    for (const auto &inputDeviceFileName : inputDeviceFileNames) {
        const auto inputDevicePath = INPUT_DEVICE_ROOT + inputDeviceFileName;
        int currentInputDeviceScore = 1000;
        int inputDeviceFileDescriptor;
        struct libevdev *inputEventDevice = NULL;

        if (!isCharacterDevice(inputDevicePath)) {
            libevdev_free(inputEventDevice);
            continue;
        }

        inputDeviceFileDescriptor = open(inputDevicePath.c_str(), O_RDWR);
        if (inputDeviceFileDescriptor < 0) {
            libevdev_free(inputEventDevice);
            continue;
        }

        if (libevdev_new_from_fd(inputDeviceFileDescriptor, &inputEventDevice)) {
            libevdev_free(inputEventDevice);
            continue;
        }

        if (!libevdev_has_event_code(inputEventDevice, EV_ABS, ABS_MT_POSITION_X)) {
            libevdev_free(inputEventDevice);
            continue;
        }

        if (libevdev_has_event_code(inputEventDevice, EV_ABS, ABS_MT_TOOL_TYPE)) {
            int toolMinimum = libevdev_get_abs_minimum(inputEventDevice, ABS_MT_TOOL_TYPE);
            int toolMaximum = libevdev_get_abs_maximum(inputEventDevice, ABS_MT_TOOL_TYPE);

            if (toolMinimum > MT_TOOL_FINGER || toolMaximum < MT_TOOL_FINGER) {
                libevdev_free(inputEventDevice);
                continue;
            }

            currentInputDeviceScore -= toolMaximum - MT_TOOL_FINGER;
        }

        if (libevdev_has_event_code(inputEventDevice, EV_ABS, ABS_MT_SLOT)) {
            currentInputDeviceScore += 100;
            currentInputDeviceScore += libevdev_get_abs_maximum(inputEventDevice, ABS_MT_SLOT);
        }

        std::string inputDeviceName = libevdev_get_name(inputEventDevice);
        if (inputDeviceName.find("key") != std::string::npos) {
            currentInputDeviceScore -= 1;
        }

        if (libevdev_has_property(inputEventDevice, INPUT_PROP_DIRECT)) {
            currentInputDeviceScore += 1000;
        }

        if (maxInputDeviceScore < currentInputDeviceScore) {
            maxInputDeviceScore = currentInputDeviceScore;
            maxInputDevicePath = inputDevicePath;
        }

        libevdev_free(inputEventDevice);
    }

    if (maxInputDeviceScore > 0) {
        return std::make_shared<TouchInputDevice>(maxInputDevicePath);
    }

    return nullptr;
}
