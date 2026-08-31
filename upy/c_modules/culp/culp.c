
#include "py/obj.h"
#include "py/runtime.h"

#if CONFIG_ULP_COPROC_TYPE_RISCV
#include "ulp_riscv.h"
#elif CONFIG_ULP_COPROC_TYPE_LP_CORE
#include "ulp_lp_core.h"
static uint32_t lp_core_sleep_duration_us = 0;
#endif

#include "hal/adc_types.h"
#include "driver/rtc_io.h"
#include "ulp_adc.h"
#include "py/mphal.h"

extern const uint8_t ulp_main_bin_start[] asm ("_binary_ulp_embedded_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm ("_binary_ulp_embedded_bin_end");

typedef struct _ulp_obj_t {
    mp_obj_base_t base;
} ulp_obj_t;

static mp_obj_t ulp_make_new(const mp_obj_type_t *type,
                               size_t n_args, size_t n_kw,
                               const mp_obj_t *args) {
    ulp_obj_t *self = m_new_obj(ulp_obj_t);
    self->base.type = type;
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t esp32_ulp_set_wakeup_period(
    mp_obj_t self_in,
    mp_obj_t period_index_in,
    mp_obj_t period_us_in
    ) {
    mp_uint_t period_index = mp_obj_get_int(period_index_in);
    mp_uint_t period_us = mp_obj_get_int(period_us_in);
    int _errno;
#if CONFIG_ULP_COPROC_TYPE_RISCV
    _errno = ulp_set_wakeup_period(period_index, period_us);
#elif CONFIG_ULP_COPROC_TYPE_LP_CORE
    (void)period_index;
    lp_core_sleep_duration_us = period_us;
    _errno = ESP_OK;
#endif
    if (_errno != ESP_OK) {
        mp_raise_OSError(_errno);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(esp32_ulp_set_wakeup_period_obj, esp32_ulp_set_wakeup_period);

static mp_obj_t esp32_ulp_load_and_run_embedded(mp_obj_t self_in) {
    int _errno;
#if CONFIG_ULP_COPROC_TYPE_RISCV
    _errno = ulp_riscv_load_binary(ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start));
    if (_errno != ESP_OK) {
        mp_raise_OSError(_errno);
    }
    _errno = ulp_riscv_run();
    if (_errno != ESP_OK) {
        mp_raise_OSError(_errno);
    }
#elif CONFIG_ULP_COPROC_TYPE_LP_CORE
    _errno = ulp_lp_core_load_binary(ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start));
    if (_errno != ESP_OK) {
        mp_raise_OSError(_errno);
    }
    ulp_lp_core_cfg_t cfg = {
        .wakeup_source = lp_core_sleep_duration_us
            ? ULP_LP_CORE_WAKEUP_SOURCE_LP_TIMER
            : ULP_LP_CORE_WAKEUP_SOURCE_HP_CPU,
        .lp_timer_sleep_duration_us = lp_core_sleep_duration_us,
    };
    _errno = ulp_lp_core_run(&cfg);
    if (_errno != ESP_OK) {
        mp_raise_OSError(_errno);
    }
#endif
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(esp32_ulp_load_and_run_embedded_obj, esp32_ulp_load_and_run_embedded);

static mp_obj_t esp32_ulp_pause(mp_obj_t self_in) {
#if CONFIG_ULP_COPROC_TYPE_RISCV
    ulp_riscv_timer_stop();
    ulp_riscv_halt();
#elif CONFIG_ULP_COPROC_TYPE_LP_CORE
    ulp_lp_core_stop();
#endif
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(esp32_ulp_pause_obj, esp32_ulp_pause);

static mp_obj_t esp32_ulp_resume(mp_obj_t self_in) {
#if CONFIG_ULP_COPROC_TYPE_RISCV
    ulp_riscv_timer_resume();
#elif CONFIG_ULP_COPROC_TYPE_LP_CORE
    ulp_lp_core_cfg_t cfg = {
        .wakeup_source = lp_core_sleep_duration_us
            ? ULP_LP_CORE_WAKEUP_SOURCE_LP_TIMER
            : ULP_LP_CORE_WAKEUP_SOURCE_HP_CPU,
        .lp_timer_sleep_duration_us = lp_core_sleep_duration_us,
    };
    ulp_lp_core_run(&cfg);
#endif
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(esp32_ulp_resume_obj, esp32_ulp_resume);

static mp_obj_t esp32_ulp_read(mp_obj_t self_in, mp_obj_t address) {
    uint32_t addr = mp_obj_get_int(address);
    if (addr < (uintptr_t)RTC_SLOW_MEM) {
        addr += (uintptr_t)RTC_SLOW_MEM;
    }
    if (addr > ((uintptr_t)(RTC_SLOW_MEM) + CONFIG_ULP_COPROC_RESERVE_MEM)) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid address"));
    }
    uint32_t val = *(uint32_t *)addr;
    return mp_obj_new_int(val);
}
static MP_DEFINE_CONST_FUN_OBJ_2(esp32_ulp_read_obj, esp32_ulp_read);

static mp_obj_t esp32_ulp_write(mp_obj_t self_in, mp_obj_t address, mp_obj_t value) {
    uintptr_t addr = mp_obj_get_int(address);
    if (addr < (uintptr_t)RTC_SLOW_MEM) {
        addr += (uintptr_t)RTC_SLOW_MEM;
    }
    if (addr > ((uintptr_t)(RTC_SLOW_MEM) + CONFIG_ULP_COPROC_RESERVE_MEM)) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid address"));
    }
    *(uint32_t *)addr = mp_obj_get_int(value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(esp32_ulp_write_obj, esp32_ulp_write);

static mp_obj_t esp32_ulp_rtc_init(mp_obj_t self_in, mp_obj_t pin_in) {
    gpio_num_t gpio_id = mp_obj_get_int(pin_in);
    if (!rtc_gpio_is_valid_gpio(gpio_id)) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin"));
    }
    rtc_gpio_init(gpio_id);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(esp32_ulp_rtc_init_obj, esp32_ulp_rtc_init);

static mp_obj_t esp32_ulp_rtc_deinit(mp_obj_t self_in, mp_obj_t pin_in) {
    gpio_num_t gpio_id = mp_obj_get_int(pin_in);
    if (!rtc_gpio_is_valid_gpio(gpio_id)) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin"));
    }
    rtc_gpio_deinit(gpio_id);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(esp32_ulp_rtc_deinit_obj, esp32_ulp_rtc_deinit);

#if CONFIG_ULP_COPROC_TYPE_RISCV || SOC_LP_ADC_SUPPORTED
static mp_obj_t esp32_ulp_adc_init(mp_obj_t self_in, mp_obj_t channel_in) {
    int channel = mp_obj_get_int(channel_in);
    ulp_adc_cfg_t cfg = {
        .adc_n = ADC_UNIT_1,
        .channel = channel,
        .width = ADC_BITWIDTH_13,
        .atten = ADC_ATTEN_DB_11,
#if CONFIG_ULP_COPROC_TYPE_RISCV
        .ulp_mode = ADC_ULP_MODE_RISCV,
#else
        .ulp_mode = ADC_ULP_MODE_LP_CORE,
#endif
    };
    int _errno = ulp_adc_init(&cfg);
    if (_errno != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("ADC unit already in use or invalid channel"));
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(esp32_ulp_adc_init_obj, esp32_ulp_adc_init);
#endif

static const mp_rom_map_elem_t ulp_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_wakeup_period), MP_ROM_PTR(&esp32_ulp_set_wakeup_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_embedded), MP_ROM_PTR(&esp32_ulp_load_and_run_embedded_obj) },
    { MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&esp32_ulp_pause_obj) },
    { MP_ROM_QSTR(MP_QSTR_resume), MP_ROM_PTR(&esp32_ulp_resume_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&esp32_ulp_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&esp32_ulp_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtc_init), MP_ROM_PTR(&esp32_ulp_rtc_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtc_deinit), MP_ROM_PTR(&esp32_ulp_rtc_deinit_obj) },
    #if CONFIG_ULP_COPROC_TYPE_RISCV || SOC_LP_ADC_SUPPORTED
    { MP_ROM_QSTR(MP_QSTR_adc_init), MP_ROM_PTR(&esp32_ulp_adc_init_obj) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_RESERVE_MEM), MP_ROM_INT(CONFIG_ULP_COPROC_RESERVE_MEM) },
};
static MP_DEFINE_CONST_DICT(ulp_locals_dict, ulp_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    ulp_type,
    MP_QSTR_ULP,
    MP_TYPE_FLAG_NONE,
    make_new, ulp_make_new,
    locals_dict, &ulp_locals_dict
    );

static const mp_rom_map_elem_t cmodule_culp_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_mymodule) },
    { MP_ROM_QSTR(MP_QSTR_ULP), MP_ROM_PTR(&ulp_type) },
};
static MP_DEFINE_CONST_DICT(cmodule_culp_globals, cmodule_culp_globals_table);

const mp_obj_module_t cmodule_culp = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&cmodule_culp_globals,
};

MP_REGISTER_MODULE(MP_QSTR_culp, cmodule_culp);
