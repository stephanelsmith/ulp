


### In `esp32_common.cmake`
#### add after `idf_component_register`
```
if(DEFINED ulp_embedded_sources)
    list(APPEND MICROPY_DEF_CORE ULP_EMBEDDED_APP=1)
    set(ulp_app_name 
        "ulp_embedded"
    )
    set(ulp_depentants 
        ${ulp_depentants} 
        "esp32_ulp.c"
    )
    message("embedded ULP App sources: " ${ulp_embedded_sources} ",  deps: " ${ulp_depentants})
    ulp_embed_binary(${ulp_app_name} ${ulp_embedded_sources} ${ulp_depentants})
endif()
```

### In `esp32_ulp.c`
#### add && !CONFIG_ULP_COPROC_TYPE_RISCV to the top-level #if guard
#### replace
```
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
```
####  with
```
#if (CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3) && !CONFIG_ULP_COPROC_TYPE_RISCV
```


### In `modesp32.c`
#### same guard on the module table entry that references esp32_ulp_type
#### replace
```
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
```
####  with
```
#if (CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3) && !CONFIG_ULP_COPROC_TYPE_RISCV
```


### In `modesp32.h`
#### wrapped the extern const mp_obj_type_t esp32_ulp_type declaration in #if !CONFIG_ULP_COPROC_TYPE_RISCV
```
 extern const mp_obj_type_t esp32_partition_type;
 extern const mp_obj_type_t esp32_rmt_type;
+#if !CONFIG_ULP_COPROC_TYPE_RISCV
 extern const mp_obj_type_t esp32_ulp_type;
+#endif
 extern const mp_obj_type_t esp32_ldo_type;
```
