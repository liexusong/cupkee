/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016 Lixing Ding <ding.lixing@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "hardware.h"

#define C_STACK_SIZE 8192

extern char _etext;  // devined in ld scripts
extern char end;     // defined in ld scripts

extern vector_table_t vector_table;
void *hw_memory_bgn = NULL;
void *hw_memory_end = NULL;

static void hw_memory_init(void)
{
    hw_memory_bgn = CUPKEE_ADDR_ALIGN(&end, 16);
    hw_memory_end = (char *)(vector_table.initial_sp_value) - C_STACK_SIZE;
}

static void hw_setup_systick(void)
{
    systick_set_frequency(SYSTEM_TICKS_PRE_SEC, 72000000);

    systick_interrupt_enable();
    systick_counter_enable();
}

/* systick interrupt handle routing  */
void sys_tick_handler(void)
{
    _cupkee_systicks++;
    cupkee_event_post_systick();
}

size_t hw_memory_left(void)
{
    return hw_memory_end - hw_memory_bgn;
}

size_t hw_malloc_all(void **p, size_t align)
{
    void *memory_align;
    int left;

    if (align) {
        memory_align = CUPKEE_ADDR_ALIGN(hw_memory_bgn, align);
    } else {
        memory_align = hw_memory_bgn;
    }

    left = hw_memory_end - memory_align;
    hw_memory_bgn = memory_align + left;

    if (p) {
        *p = memory_align;
    }

    return left;
}

void *hw_malloc(size_t size, size_t align)
{
    void *memory_align;
    size_t left;

    if (size == 0) {
        return NULL;
    }

    if (align) {
        memory_align = CUPKEE_ADDR_ALIGN(hw_memory_bgn, align);
    } else {
        memory_align = hw_memory_bgn;
    }

    left = hw_memory_end - memory_align;
    if (left < size) {
        return NULL;
    }

    hw_memory_bgn = memory_align + size;

    return memory_align;
}

void hw_enter_critical(uint32_t *state)
{
    *state = cm_mask_interrupts(1);
}

void hw_exit_critical(uint32_t state)
{
    cm_mask_interrupts(state);
}

void hw_info_get(hw_info_t *info)
{
    if (info) {
        info->sys_freq = 72000000;
        info->ram_sz = (char *)(vector_table.initial_sp_value) - (char *)0x20000000;
        //info->rom_sz = desig_get_flash_size() * 1024;
        info->rom_sz = &_etext - (char *)0x8000000;
        info->ram_base = (void *)0x20000000;
        info->rom_base = (void *)0x08000000;
    }
}

void hw_setup(void)
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

    hw_memory_init();

    /* initial device resouce */
    hw_setup_gpio();
    hw_setup_usart();
    hw_setup_adc();
    hw_setup_i2c();
    hw_setup_timer();
    hw_setup_usb();

    /* initial resource system depend on */
    hw_setup_storage();

    hw_setup_systick();
}

void hw_poll(void)
{
    hw_poll_usb();
}

void hw_halt(void)
{
    while (1)
        ;
}

const hw_driver_t *hw_device_request(int type, int instance)
{
    switch (type) {
    case DEVICE_TYPE_PIN:       return hw_request_pin(instance);
    case DEVICE_TYPE_ADC:       return hw_request_adc(instance);
    case DEVICE_TYPE_DAC:       return NULL;
    case DEVICE_TYPE_PWM:       return hw_request_pwm(instance);
    case DEVICE_TYPE_PULSE:     return hw_request_pulse(instance);
    case DEVICE_TYPE_TIMER:     return hw_request_timer(instance);
    case DEVICE_TYPE_COUNTER:   return hw_request_counter(instance);
    case DEVICE_TYPE_UART:      return hw_request_uart(instance);
    case DEVICE_TYPE_I2C:       return hw_request_i2c(instance);
    case DEVICE_TYPE_SPI:
    case DEVICE_TYPE_USART:     return NULL;
    case DEVICE_TYPE_USB_CDC:   return hw_request_cdc(instance);
    default:                    return NULL;
    }
}

int hw_device_instances(int type)
{
    switch (type) {
    case DEVICE_TYPE_PIN:       return HW_INSTANCES_PIN;
    case DEVICE_TYPE_ADC:       return HW_INSTANCES_ADC;
    case DEVICE_TYPE_DAC:       return 0;
    case DEVICE_TYPE_PWM:       return HW_INSTANCES_PWM;
    case DEVICE_TYPE_PULSE:     return HW_INSTANCES_PULSE;
    case DEVICE_TYPE_TIMER:     return HW_INSTANCES_TIMER;
    case DEVICE_TYPE_COUNTER:   return HW_INSTANCES_COUNTER;
    case DEVICE_TYPE_UART:      return HW_INSTANCES_UART;
    case DEVICE_TYPE_I2C:       return HW_INSTANCES_I2C;
    case DEVICE_TYPE_SPI:       return 0;
    case DEVICE_TYPE_USART:     return 0;
    case DEVICE_TYPE_USB_CDC:   return 1;
    default:                    return 0;
    }
}

