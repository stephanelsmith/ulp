## Micro-ULP
A project to enable ULP for the esp32s3 and esp32c6 in Micropython.

### :wrench: EspIDF
```
git clone --depth 1 --branch v5.5.2 https://github.com/espressif/esp-idf.git esp-idf-v5.5.2
cd esp-idf-v5.5.2
git submodule update --init --recursive
./install.sh
source export.sh
```

### :snake: Micropython
```
git clone git@github.com:hudl/titan-micropython.git micropython-espidf5.5.2
cd micropython-espidf5.5.2
git submodule update --init --recursive
make -C mpy-cross
cd ports/esp32
```

[Updates required in micropython to enable ULP](docs/ulp.md)

### :hammer: Build
```
ln -sf ~/micro-upy/upy/boards/* boards/.
make BOARD=SS_ULP_S3 USER_C_MODULES=$IDF_PATH/../micro-ulp/upy/c_modules/esp32.cmake
make BOARD=SS_ULP_C6 USER_C_MODULES=$IDF_PATH/../micro-ulp/upy/c_modules/esp32.cmake
```

### :horse_racing: Run
```
# get ulp variables address here:
cat build-SS_ULP_S3/esp-idf/main/ulp_embedded/ulp_embedded.ld
cat build-SS_ULP_C6/esp-idf/main/ulp_embedded/ulp_embedded.ld 
```
```
import culp
u = culp.ULP()
u.set_wakeup_period(1000*1000) # 1s
u.run_embedded()

# get variables here
# cat build-SS_ULP_S3/esp-idf/main/ulp_embedded/ulp_embedded.ld
ulp_var_counter = 0x50000080  #s3
u.read(ulp_var_counter)
ulp_var_counter = 0x50000450  #c6
u.read(ulp_var_counter)
```



## :raised_hands: Acknowledgements
- [Micropython](https://github.com/micropython/micropython) project


## License
GNU General Public License v3.0
