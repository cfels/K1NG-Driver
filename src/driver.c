#include "../includes/include.h"
#include "driver.h"
#include "data.h"

void run_driver(int sens) {
    libusb_device_handle *handle = NULL;
    int r = libusb_init(NULL);
    if (r < 0) {
        fprintf(stderr, "libusb init failed: %s\n", libusb_error_name(r));
        return;
    }

    // wired/wireless support
    handle = libusb_open_device_with_vid_pid(NULL, TARGET_VID, TARGET_PID_WIRED);
    if (!handle) {
        handle = libusb_open_device_with_vid_pid(NULL, TARGET_VID, TARGET_PID_WIRELESS);
    }

    if (!handle) {
        fprintf(stderr, "mouse not found!!\n");
        libusb_exit(NULL);
        return;
    }

    printf("mouse found!!\n");

    if (libusb_kernel_driver_active(handle, 1) == 1) {
        libusb_detach_kernel_driver(handle, 1);
    }
    libusb_claim_interface(handle, 1);

    unsigned char payload[17];
    get_payload(sens, payload);

    int transferred = libusb_control_transfer(
        handle,
        0x21,
        0x09,
        0x0208,
        0x0001,
        payload,
        sizeof(payload),
        4000
    );

    if (transferred < 0) {
        fprintf(stderr, "byte transfer failed: %s\n", libusb_error_name(transferred));
    } else {
        printf("write succeed %d (%d bytes written)\n", sens, transferred);
    }

    libusb_release_interface(handle, 1);
    libusb_close(handle);
    libusb_exit(NULL);
}
