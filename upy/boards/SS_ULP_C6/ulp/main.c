
#include <stdint.h>
#include "ulp_lp_core_gpio.h"
#include "ulp_lp_core_utils.h"

#define LED_GPIO_NUM 3

unsigned int var_counter = 0;
unsigned int var_count = 1;

void main(){
      if(var_count){
          var_counter++;
      }
      ulp_lp_core_gpio_set_level(LED_GPIO_NUM, 1);
      ulp_lp_core_gpio_set_level(LED_GPIO_NUM, 0);
}
