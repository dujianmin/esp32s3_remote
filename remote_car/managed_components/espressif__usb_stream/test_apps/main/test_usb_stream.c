/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "usb_stream.h"
#include "unity.h"
#include "esp_random.h"

static const char *TAG = "usb_stream_test";

/* USB PIN fixed in esp32-s2, can not use io matrix */
#define TEST_MEMORY_LEAK_THRESHOLD (-400)
#define DEMO_XFER_BUFFER_SIZE (55 * 1024)

static void *_malloc(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

static void _free(void *ptr)
{
    heap_caps_free(ptr);
}

/* *******************************************************************************************
 * This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames. */
static void frame_cb(uvc_frame_t *frame, void *ptr)
{
    ESP_LOGI(TAG, "uvc callback! frame_format = %d, seq = %"PRIu32", width = %"PRIu32", height = %"PRIu32", length = %u, ptr = %d",
            frame->frame_format, frame->sequence, frame->width, frame->height, frame->data_bytes, (int) ptr);

    switch (frame->frame_format) {
        case UVC_FRAME_FORMAT_MJPEG:
            break;
        default:
            break;
    }
}

static void stream_state_changed_cb(usb_stream_state_t event, void *arg)
{
    switch (event) {
    case STREAM_CONNECTED: {
        TaskHandle_t task_handle = (TaskHandle_t)arg;
        if (task_handle) xTaskNotifyGive(task_handle);
        break;
    }
    case STREAM_DISCONNECTED:
        ESP_LOGI(TAG, "Device disconnected");
        break;
    default:
        ESP_LOGE(TAG, "Unknown event");
        break;
    }
}


TEST_CASE("test uvc any resolution", "[devkit][uvc]")
{
    esp_log_level_set("*", ESP_LOG_DEBUG);
    /* malloc double buffer for usb payload, xfer_buffer_size >= frame_buffer_size*/
    uint8_t *xfer_buffer_a = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(xfer_buffer_a != NULL);
    uint8_t *xfer_buffer_b = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(xfer_buffer_b != NULL);

    /* malloc frame buffer for a jpeg frame*/
    uint8_t *frame_buffer = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(frame_buffer != NULL);

    uvc_config_t uvc_config = {
        .frame_width = FRAME_RESOLUTION_ANY,
        .frame_height = FRAME_RESOLUTION_ANY,
        .frame_interval = FPS2INTERVAL(15),
        .xfer_buffer_size = DEMO_XFER_BUFFER_SIZE,
        .xfer_buffer_a = xfer_buffer_a,
        .xfer_buffer_b = xfer_buffer_b,
        .frame_buffer_size = DEMO_XFER_BUFFER_SIZE,
        .frame_buffer = frame_buffer,
        .frame_cb = frame_cb,
        .frame_cb_arg = NULL,
    };
    size_t test_count = 3;
    for (size_t i = 0; i < test_count; i++) {
        TEST_ASSERT_EQUAL(ESP_OK, uvc_streaming_config(&uvc_config));
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_start());
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_connect_wait(portMAX_DELAY));
        /* add delay for some camera will low response speed to pass */
        vTaskDelay(1);
        /* test streaming suspend */
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UVC, CTRL_SUSPEND, NULL));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        /* test streaming resume */
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UVC, CTRL_RESUME, NULL));
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        /* test streaming stop */
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_stop());
    }

    _free(xfer_buffer_a);
    _free(xfer_buffer_b);
    _free(frame_buffer);
    vTaskDelay(pdMS_TO_TICKS(100));
}

TEST_CASE("test uvc specified resolution", "[devkit][uvc]")
{
#define FRAME_WIDTH 480
#define FRAME_HEIGHT 320
    esp_log_level_set("*", ESP_LOG_DEBUG);
    /* malloc double buffer for usb payload, xfer_buffer_size >= frame_buffer_size*/
    uint8_t *xfer_buffer_a = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(xfer_buffer_a != NULL);
    uint8_t *xfer_buffer_b = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(xfer_buffer_b != NULL);

    /* malloc frame buffer for a jpeg frame*/
    uint8_t *frame_buffer = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(frame_buffer != NULL);

    uvc_config_t uvc_config = {
        .frame_width = FRAME_WIDTH,
        .frame_height = FRAME_HEIGHT,
        .frame_interval = FPS2INTERVAL(15),
        .xfer_buffer_size = DEMO_XFER_BUFFER_SIZE,
        .xfer_buffer_a = xfer_buffer_a,
        .xfer_buffer_b = xfer_buffer_b,
        .frame_buffer_size = DEMO_XFER_BUFFER_SIZE,
        .frame_buffer = frame_buffer,
        .frame_cb = frame_cb,
        .frame_cb_arg = NULL,
    };
    size_t test_count = 3;
    for (size_t i = 0; i < test_count; i++) {
        TEST_ASSERT_EQUAL(ESP_OK, uvc_streaming_config(&uvc_config));
        TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
        TEST_ASSERT_NOT_NULL(task_handle);
        //register callback to get the notification when the device is connected
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_state_register(&stream_state_changed_cb, (void *)task_handle));
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_start());
        //wait for the device connected
        TEST_ASSERT_NOT_EQUAL(0, ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000)));
        //get the resolution list and current index of connected camera
        size_t frame_size = 0;
        size_t frame_index = 0;
        TEST_ASSERT_EQUAL(ESP_OK, uvc_frame_size_list_get(NULL, &frame_size, &frame_index));
        TEST_ASSERT_NOT_EQUAL(0, frame_size);
        ESP_LOGI(TAG, "UVC: get frame list size = %u, current = %u", frame_size, frame_index);
        uvc_frame_size_t *uvc_frame_list = (uvc_frame_size_t *)malloc(frame_size * sizeof(uvc_frame_size_t));
        TEST_ASSERT_EQUAL(ESP_OK, uvc_frame_size_list_get(uvc_frame_list, NULL, NULL));
        TEST_ASSERT_EQUAL(FRAME_WIDTH, uvc_frame_list[frame_index].width);
        TEST_ASSERT_EQUAL(FRAME_HEIGHT, uvc_frame_list[frame_index].height);
        /* test streaming stop */
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_stop());
        free(uvc_frame_list);
    }

    _free(xfer_buffer_a);
    _free(xfer_buffer_b);
    _free(frame_buffer);
    vTaskDelay(pdMS_TO_TICKS(100));
}

TEST_CASE("test uvc change resolution", "[devkit][uvc]")
{
    esp_log_level_set("*", ESP_LOG_DEBUG);
    /* malloc double buffer for usb payload, xfer_buffer_size >= frame_buffer_size*/
    uint8_t *xfer_buffer_a = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(xfer_buffer_a != NULL);
    uint8_t *xfer_buffer_b = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(xfer_buffer_b != NULL);

    /* malloc frame buffer for a jpeg frame*/
    uint8_t *frame_buffer = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(frame_buffer != NULL);

    uvc_config_t uvc_config = {
        .frame_width = FRAME_RESOLUTION_ANY,
        .frame_height = FRAME_RESOLUTION_ANY,
        .frame_interval = FPS2INTERVAL(15),
        .xfer_buffer_size = DEMO_XFER_BUFFER_SIZE,
        .xfer_buffer_a = xfer_buffer_a,
        .xfer_buffer_b = xfer_buffer_b,
        .frame_buffer_size = DEMO_XFER_BUFFER_SIZE,
        .frame_buffer = frame_buffer,
        .frame_cb = frame_cb,
        .frame_cb_arg = NULL,
        // suspend uvc streaming after usb_streaming_start
        .flags = FLAG_UVC_SUSPEND_AFTER_START,
    };
    size_t test_count = 3;
    for (size_t i = 0; i < test_count; i++) {
        TEST_ASSERT_EQUAL(ESP_OK, uvc_streaming_config(&uvc_config));
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_start());
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_connect_wait(portMAX_DELAY));
        size_t frame_size = 0;
        size_t frame_index = 0;
        //Check the resolution list of connected camera
        TEST_ASSERT_EQUAL(ESP_OK, uvc_frame_size_list_get(NULL, &frame_size, &frame_index));
        TEST_ASSERT_NOT_EQUAL(0, frame_size);
        ESP_LOGI(TAG, "UVC: get frame list size = %u, current = %u", frame_size, frame_index);
        uvc_frame_size_t *uvc_frame_list = (uvc_frame_size_t *)malloc(frame_size * sizeof(uvc_frame_size_t));
        TEST_ASSERT_EQUAL(ESP_OK, uvc_frame_size_list_get(uvc_frame_list, NULL, NULL));
        for (size_t i = 0; i < frame_size; i++) {
            //Change the resolution one after another
            ESP_LOGI(TAG, "\tframe[%u] = %ux%u", i, uvc_frame_list[i].width, uvc_frame_list[i].height);
            uvc_frame_size_reset(uvc_frame_list[i].width, uvc_frame_list[i].height, FPS2INTERVAL(15));
            //Resume uvc streaming to get the frames
            TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UVC, CTRL_RESUME, NULL));
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            //Suspend uvc streaming to change the resolution
            TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UVC, CTRL_SUSPEND, NULL));
        }
        /* test streaming stop */
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_stop());
        free(uvc_frame_list);
    }

    _free(xfer_buffer_a);
    _free(xfer_buffer_b);
    _free(frame_buffer);
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void mic_frame_cb(mic_frame_t *frame, void *ptr)
{
    // We should using higher baudrate here, to reduce the blocking time here
    ESP_LOGV(TAG, "mic callback! bit_resolution = %u, samples_frequence = %"PRIu32", data_bytes = %"PRIu32,
            frame->bit_resolution, frame->samples_frequence, frame->data_bytes);
    // We should never block in mic callback!
    if (ptr) uac_spk_streaming_write(frame->data, frame->data_bytes, 0);
}

TEST_CASE("test uac mic", "[devkit][uac][mic]")
{
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    uac_config_t uac_config = {
        .mic_ch_num = UAC_CH_ANY,
        .mic_bit_resolution = UAC_BITS_ANY,
        .mic_samples_frequence = UAC_FREQUENCY_ANY,
        .mic_buf_size = 6400,
        .mic_cb = mic_frame_cb,
        .mic_cb_arg = NULL,
        .flags = FLAG_UAC_MIC_SUSPEND_AFTER_START,
    };
    size_t loop_count = 5;
    for (size_t i = 0; i < loop_count; i++) {
        /* code */
        TEST_ASSERT_EQUAL(ESP_OK, uac_streaming_config(&uac_config));
        /* Start camera IN stream with pre-configs, uvc driver will create multi-tasks internal
        to handle usb data from different pipes, and user's callback will be called after new frame ready. */
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_start());
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_connect_wait(portMAX_DELAY));
        //Check the resolution list of connected camera
        size_t frame_size = 0;
        size_t frame_index = 0;
        TEST_ASSERT_EQUAL(ESP_OK, uac_frame_size_list_get(STREAM_UAC_MIC, NULL, &frame_size, &frame_index));
        TEST_ASSERT_NOT_EQUAL(0, frame_size);
        ESP_LOGI(TAG, "uac: get frame list size = %u, current = %u", frame_size, frame_index);
        uac_frame_size_t *uac_frame_list = (uac_frame_size_t *)malloc(frame_size * sizeof(uac_frame_size_t));
        TEST_ASSERT_EQUAL(ESP_OK, uac_frame_size_list_get(STREAM_UAC_MIC, uac_frame_list, NULL, NULL));
        ESP_LOGI(TAG, "current frame: ch_num %u, bit %u, frequency %"PRIu32, uac_frame_list[frame_index].ch_num, uac_frame_list[frame_index].bit_resolution, uac_frame_list[frame_index].samples_frequence);
        free(uac_frame_list);
        size_t test_count = 100;
        size_t buffer_size = 96 * 20;
        size_t read_size = 0;
        uint8_t *buffer = malloc(buffer_size);
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_MIC, CTRL_RESUME, NULL));
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_MIC, CTRL_UAC_MUTE, (void *)0));
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_MIC, CTRL_UAC_VOLUME, (void *)90));
        vTaskDelay(20 / portTICK_PERIOD_MS);
        for (size_t i = 0; i < test_count; i++) {
            /* pre-config UAC driver with params from known USB Camera Descriptors*/
            TEST_ASSERT_EQUAL(ESP_OK, uac_mic_streaming_read(buffer, buffer_size, &read_size, portMAX_DELAY));
            TEST_ASSERT_NOT_EQUAL(0, read_size);
            ESP_LOGI(TAG, "mic rcv len: %u\n", read_size);
            vTaskDelay(20 / portTICK_PERIOD_MS);
            test_count--;
            if (test_count % 20 == 0) {
                TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_MIC, CTRL_UAC_MUTE, (void *)1));
                TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_MIC, CTRL_SUSPEND, NULL));
                vTaskDelay(20 / portTICK_PERIOD_MS);
                TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_MIC, CTRL_RESUME, NULL));
                TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_MIC, CTRL_UAC_MUTE, (void *)0));
                TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_MIC, CTRL_UAC_VOLUME, (void *)test_count));
            }
        }
        free(buffer);
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_MIC, CTRL_SUSPEND, NULL));
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_stop());
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}

TEST_CASE("test uac spk", "[devkit][uac][spk]")
{
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    uac_config_t uac_config = {
        .spk_ch_num = UAC_CH_ANY,
        .spk_bit_resolution = UAC_BITS_ANY,
        .spk_samples_frequence = UAC_FREQUENCY_ANY,
        .spk_buf_size = 6400,
        .flags = FLAG_UAC_SPK_SUSPEND_AFTER_START,
    };
    size_t loop_count = 3;
    for (size_t i = 0; i < loop_count; i++) {
        loop_count--;
        /* code */
        TEST_ASSERT_EQUAL(ESP_OK, uac_streaming_config(&uac_config));
        /* Start camera IN stream with pre-configs, uvc driver will create multi-tasks internal
        to handle usb data from different pipes, and user's callback will be called after new frame ready. */
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_start());
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_connect_wait(portMAX_DELAY));
        //Check the resolution list of connected camera
        size_t frame_size = 0;
        size_t frame_index = 0;
        TEST_ASSERT_EQUAL(ESP_OK, uac_frame_size_list_get(STREAM_UAC_SPK, NULL, &frame_size, &frame_index));
        TEST_ASSERT_NOT_EQUAL(0, frame_size);
        ESP_LOGI(TAG, "uac: get frame list size = %u, current = %u", frame_size, frame_index);
        uac_frame_size_t *uac_frame_list = (uac_frame_size_t *)malloc(frame_size * sizeof(uac_frame_size_t));
        TEST_ASSERT_EQUAL(ESP_OK, uac_frame_size_list_get(STREAM_UAC_SPK, uac_frame_list, NULL, NULL));
        ESP_LOGI(TAG, "current frame: ch_num %u, bit %u, frequency %"PRIu32, uac_frame_list[frame_index].ch_num, uac_frame_list[frame_index].bit_resolution, uac_frame_list[frame_index].samples_frequence);
        extern const uint8_t wave_array_32000_16_1[];
        extern const uint32_t s_buffer_size;
        int freq_offsite_step = 32000 / uac_frame_list[frame_index].samples_frequence;
        int downsampling_bits = 16 - uac_frame_list[frame_index].bit_resolution;
        const int buffer_ms = 200;
        const int buffer_size = buffer_ms * (uac_frame_list[frame_index].bit_resolution / 8) * (uac_frame_list[frame_index].samples_frequence / 1000);
        // if 8bit spk, declare uint8_t *d_buffer
        size_t offset_size = buffer_size / (uac_frame_list[frame_index].bit_resolution / 8);
        size_t test_count = 3;
        for (size_t i = 0; i < test_count; i++) {
            // if 8bit spk, declare uint8_t *d_buffer
            uint16_t *s_buffer = (uint16_t *)wave_array_32000_16_1;
            uint16_t *d_buffer = calloc(1, buffer_size);
            TEST_ASSERT_NOT_NULL(d_buffer);
            size_t spk_count = 3;
            while(spk_count) {
                TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_SPK, CTRL_RESUME, NULL));
                TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_SPK, CTRL_UAC_VOLUME, (void *)(90/spk_count)));
                TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_SPK, CTRL_UAC_MUTE, (void *)0));
                while (1) {
                    if ((uint32_t)(s_buffer + offset_size) >= (uint32_t)(wave_array_32000_16_1 + s_buffer_size)) {
                        s_buffer = (uint16_t *)wave_array_32000_16_1;
                        break;
                    } else {
                        // fill to usb buffer
                        for (size_t i = 0; i < offset_size; i++) {
                            d_buffer[i] = *(s_buffer + i * freq_offsite_step) >> downsampling_bits;
                        }
                        // write to usb speaker
                        uac_spk_streaming_write(d_buffer, buffer_size, portMAX_DELAY);
                        s_buffer += offset_size * freq_offsite_step;
                    }
                }
                spk_count--;
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_SPK, CTRL_UAC_VOLUME, (void *)(0/spk_count)));
                TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_SPK, CTRL_UAC_MUTE, (void *)1));
                TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_SPK, CTRL_SUSPEND, NULL));
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            free(d_buffer);
            test_count--;
        }
        free(uac_frame_list);
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_stop());
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}

TEST_CASE("test uac mic spk loop", "[devkit][uac][mic][spk]")
{
    esp_log_level_set("*", ESP_LOG_DEBUG);
    uac_config_t uac_config = {
        .mic_bit_resolution = UAC_BITS_ANY,
        .mic_samples_frequence = UAC_FREQUENCY_ANY,
        .spk_bit_resolution = UAC_BITS_ANY,
        .spk_samples_frequence = UAC_FREQUENCY_ANY,
        .spk_buf_size = 16000,
        .mic_cb = &mic_frame_cb,
        .mic_cb_arg = (void*)1,
    };
    size_t test_count = 3;
    for (size_t i = 0; i < test_count; i++) {
        /* pre-config UAC driver with params from known USB Camera Descriptors*/
        TEST_ASSERT_EQUAL(ESP_OK, uac_streaming_config(&uac_config));
        /* Start camera IN stream with pre-configs, uvc driver will create multi-tasks internal
        to handle usb data from different pipes, and user's callback will be called after new frame ready. */
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_start());
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_connect_wait(portMAX_DELAY));
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_SPK, CTRL_UAC_VOLUME, (void *)(90/(i+1))));
        size_t test_count2 = 3;
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        while (--test_count2) {
            TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_MIC, CTRL_SUSPEND, NULL));
            ESP_LOGI(TAG, "mic suspend");
            vTaskDelay(500 / portTICK_PERIOD_MS);
            TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_SPK, CTRL_SUSPEND, NULL));
            ESP_LOGI(TAG, "speaker suspend");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_SPK, CTRL_RESUME, NULL));
            ESP_LOGI(TAG, "speaker resume");
            vTaskDelay(500 / portTICK_PERIOD_MS);
            TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_control(STREAM_UAC_MIC, CTRL_RESUME, NULL));
            ESP_LOGI(TAG, "mic resume");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_stop());
        test_count--;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}

TEST_CASE("test uvc+uac", "[devkit][uvc][uac][mic][spk]")
{
    esp_log_level_set("*", ESP_LOG_DEBUG);
    /* malloc double buffer for usb payload, xfer_buffer_size >= frame_buffer_size*/
    uint8_t *xfer_buffer_a = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(xfer_buffer_a != NULL);
    uint8_t *xfer_buffer_b = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(xfer_buffer_b != NULL);

    /* malloc frame buffer for a jpeg frame*/
    uint8_t *frame_buffer = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(frame_buffer != NULL);

    uvc_config_t uvc_config = {
        .frame_width = FRAME_RESOLUTION_ANY,
        .frame_height = FRAME_RESOLUTION_ANY,
        .frame_interval = FPS2INTERVAL(15),
        .xfer_buffer_size = DEMO_XFER_BUFFER_SIZE,
        .xfer_buffer_a = xfer_buffer_a,
        .xfer_buffer_b = xfer_buffer_b,
        .frame_buffer_size = DEMO_XFER_BUFFER_SIZE,
        .frame_buffer = frame_buffer,
        .frame_cb = frame_cb,
        .frame_cb_arg = NULL,
    };

    uac_config_t uac_config = {
        .mic_bit_resolution = UAC_BITS_ANY,
        .mic_samples_frequence = UAC_FREQUENCY_ANY,
        .spk_bit_resolution = UAC_BITS_ANY,
        .spk_samples_frequence = UAC_FREQUENCY_ANY,
        .spk_buf_size = 16000,
        .mic_cb = &mic_frame_cb,
        .mic_cb_arg = (void*)1,
    };

    size_t test_count = 3;
    for (size_t i = 0; i < test_count; i++) {
        /* pre-config UVC driver with params from known USB Camera Descriptors*/
        TEST_ASSERT_EQUAL(ESP_OK, uvc_streaming_config(&uvc_config));
        /* pre-config UAC driver with params from known USB Camera Descriptors*/
        TEST_ASSERT_EQUAL(ESP_OK, uac_streaming_config(&uac_config));
        /* Start camera IN stream with pre-configs, uvc driver will create multi-tasks internal
        to handle usb data from different pipes, and user's callback will be called after new frame ready. */
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_start());
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_connect_wait(portMAX_DELAY));
        vTaskDelay(50 / portTICK_PERIOD_MS);
        /* test streaming suspend */
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UVC, CTRL_SUSPEND, NULL));
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UAC_SPK, CTRL_SUSPEND, NULL));
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UAC_MIC, CTRL_SUSPEND, NULL));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        /* test streaming resume */
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UVC, CTRL_RESUME, NULL));
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UAC_SPK, CTRL_RESUME, NULL));
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UAC_MIC, CTRL_RESUME, NULL));
        vTaskDelay(15000 / portTICK_PERIOD_MS);
        /* test streaming stop */
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_stop());
    }

    _free(xfer_buffer_a);
    _free(xfer_buffer_b);
    _free(frame_buffer);
    vTaskDelay(pdMS_TO_TICKS(100));
}

/* Power Selector */
#define BOARD_IO_HOST_BOOST_EN 13
#define BOARD_IO_DEV_VBUS_EN 12
#define BOARD_IO_USB_SEL 18
#define BOARD_IO_IDEV_LIMIT_EN 17

#define BOARD_IO_PIN_SEL_OUTPUT ((1ULL<<BOARD_IO_HOST_BOOST_EN)\
                                |(1ULL<<BOARD_IO_DEV_VBUS_EN)\
                                |(1ULL<<BOARD_IO_USB_SEL)\
                                |(1ULL<<BOARD_IO_IDEV_LIMIT_EN))

typedef enum {
    USB_DEVICE_MODE,
    USB_HOST_MODE,
} usb_mode_t;

esp_err_t iot_board_usb_device_set_power(bool on_off, bool from_battery)
{
    if (from_battery) {
        gpio_set_level(BOARD_IO_HOST_BOOST_EN, on_off); //BOOST_EN
        gpio_set_level(BOARD_IO_DEV_VBUS_EN, !on_off);//DEV_VBUS_EN
        gpio_set_level(BOARD_IO_IDEV_LIMIT_EN, on_off);//DEV_VBUS_EN
    } else {
        gpio_set_level(BOARD_IO_HOST_BOOST_EN, !on_off); //BOOST_EN
        gpio_set_level(BOARD_IO_DEV_VBUS_EN, on_off);//DEV_VBUS_EN
        gpio_set_level(BOARD_IO_IDEV_LIMIT_EN, on_off);//DEV_VBUS_EN
    }
    return ESP_OK;
}

esp_err_t iot_board_usb_set_mode(usb_mode_t mode)
{
    switch (mode)
    {
        case USB_DEVICE_MODE:
            gpio_set_level(BOARD_IO_USB_SEL, false); //USB_SEL
            break;
        case USB_HOST_MODE:
            gpio_set_level(BOARD_IO_USB_SEL, true); //USB_SEL
            break;
        default:
            assert(0);
            break;
    }
    return ESP_OK;
}

usb_mode_t iot_board_usb_get_mode(void)
{
    int value = gpio_get_level(BOARD_IO_USB_SEL);
    if (value) {
        return USB_HOST_MODE;
    }
    return USB_DEVICE_MODE;
}

static esp_err_t board_gpio_init(void)
{
    esp_err_t ret;
    gpio_config_t io_conf = {0};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = BOARD_IO_PIN_SEL_OUTPUT;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    ret = gpio_config(&io_conf);
    return ret;
}

TEST_CASE("30 min long time uvc+uac with suddenly disconnect", "[otg][uvc][uac][mic][spk][dis]")
{
    esp_log_level_set("*", ESP_LOG_DEBUG);
    /* malloc double buffer for usb payload, xfer_buffer_size >= frame_buffer_size*/
    uint8_t *xfer_buffer_a = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(xfer_buffer_a != NULL);
    uint8_t *xfer_buffer_b = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(xfer_buffer_b != NULL);

    /* malloc frame buffer for a jpeg frame*/
    uint8_t *frame_buffer = (uint8_t *)_malloc(DEMO_XFER_BUFFER_SIZE);
    TEST_ASSERT(frame_buffer != NULL);
    TEST_ASSERT_EQUAL(ESP_OK, board_gpio_init());
    TEST_ASSERT_EQUAL(ESP_OK, iot_board_usb_set_mode(USB_HOST_MODE));
    TEST_ASSERT_EQUAL(ESP_OK, iot_board_usb_device_set_power(true, false));
    uvc_config_t uvc_config = {
        .frame_width = FRAME_RESOLUTION_ANY,
        .frame_height = FRAME_RESOLUTION_ANY,
        .frame_interval = FPS2INTERVAL(15),
        .xfer_buffer_size = DEMO_XFER_BUFFER_SIZE,
        .xfer_buffer_a = xfer_buffer_a,
        .xfer_buffer_b = xfer_buffer_b,
        .frame_buffer_size = DEMO_XFER_BUFFER_SIZE,
        .frame_buffer = frame_buffer,
        .frame_cb = frame_cb,
        .frame_cb_arg = NULL,
    };

    uac_config_t uac_config = {
        .mic_bit_resolution = UAC_BITS_ANY,
        .mic_samples_frequence = UAC_FREQUENCY_ANY,
        .spk_bit_resolution = UAC_BITS_ANY,
        .spk_samples_frequence = UAC_FREQUENCY_ANY,
        .spk_buf_size = 16000,
        .mic_cb = &mic_frame_cb,
        .mic_cb_arg = (void*)1,
    };

    size_t test_count = 10;
    for (size_t i = 0; i < test_count; i++) {
        /* pre-config UVC driver with params from known USB Camera Descriptors*/
        TEST_ASSERT_EQUAL(ESP_OK, uvc_streaming_config(&uvc_config));
        /* pre-config UAC driver with params from known USB Camera Descriptors*/
        TEST_ASSERT_EQUAL(ESP_OK, uac_streaming_config(&uac_config));
        /* Start camera IN stream with pre-configs, uvc driver will create multi-tasks internal
        to handle usb data from different pipes, and user's callback will be called after new frame ready. */
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_start());
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_connect_wait(portMAX_DELAY));
        vTaskDelay(50 / portTICK_PERIOD_MS);

        size_t dis_count = 3;
        while (--dis_count) {
            uint32_t delay = esp_random() % 60000;
            ESP_LOGW(TAG, "will disconnect after %"PRIu32" ms", delay);
            vTaskDelay(delay / portTICK_PERIOD_MS);
            TEST_ASSERT_EQUAL(ESP_OK, iot_board_usb_device_set_power(false, false));
            delay = esp_random() % 600;
            vTaskDelay(delay / portTICK_PERIOD_MS);
            TEST_ASSERT_EQUAL(ESP_OK, iot_board_usb_device_set_power(true, false));
        }

        /* test streaming suspend */
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UVC, CTRL_SUSPEND, NULL));
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UAC_SPK, CTRL_SUSPEND, NULL));
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UAC_MIC, CTRL_SUSPEND, NULL));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        /* test streaming resume */
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UVC, CTRL_RESUME, NULL));
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UAC_SPK, CTRL_RESUME, NULL));
        TEST_ASSERT_NOT_EQUAL(ESP_FAIL, usb_streaming_control(STREAM_UAC_MIC, CTRL_RESUME, NULL));
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        /* test streaming stop */
        TEST_ASSERT_EQUAL(ESP_OK, usb_streaming_stop());
    }
    TEST_ASSERT_EQUAL(ESP_OK, iot_board_usb_set_mode(USB_DEVICE_MODE));
    TEST_ASSERT_EQUAL(ESP_OK, iot_board_usb_device_set_power(false, false));
    _free(xfer_buffer_a);
    _free(xfer_buffer_b);
    _free(frame_buffer);
    vTaskDelay(pdMS_TO_TICKS(100));
}

static size_t before_free_8bit;
static size_t before_free_32bit;

static void check_leak(size_t before_free, size_t after_free, const char *type)
{
    ssize_t delta = after_free - before_free;
    printf("MALLOC_CAP_%s: Before %u bytes free, After %u bytes free (delta %d)\n", type, before_free, after_free, delta);
    TEST_ASSERT_MESSAGE(delta >= TEST_MEMORY_LEAK_THRESHOLD, "memory leak");
}

void setUp(void)
{
    before_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    before_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
}

void tearDown(void)
{
    size_t after_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t after_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    check_leak(before_free_8bit, after_free_8bit, "8BIT");
    check_leak(before_free_32bit, after_free_32bit, "32BIT");
}

void app_main(void)
{
    printf("USB STREAM TEST \n");
    unity_run_menu();
}