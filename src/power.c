
/** \file   power.c
*   \author Alex
*   \date   26.09.2023
*   
* Модуль управления питанием
*/

#include "power.h"

uint32_t get_reset_reason(void) {
  uint32_t reas = 0;

#if NRF_POWER_HAS_RESETREAS

  reas = nrf_power_resetreas_get(NRF_POWER);
  nrf_power_resetreas_clear(NRF_POWER, reas);
  if (reas & NRF_POWER_RESETREAS_NFC_MASK) {
    printk("Wake up by NFC field detect\n");
  }
  else if (reas & NRF_POWER_RESETREAS_RESETPIN_MASK) {
    printk("Reset by pin-reset\n");
  }
  else if (reas & NRF_POWER_RESETREAS_SREQ_MASK) {
    printk("Reset by soft-reset\n");
  }
  else if (reas) {
    printk("Reset by a different source (0x%08X)\n", reas);
  }
  else {
    printk("Power-on-reset\n");
  }

#else

  reas = nrf_reset_resetreas_get(NRF_RESET);
  nrf_reset_resetreas_clear(NRF_RESET, reas);
  if (reas & NRF_RESET_RESETREAS_NFC_MASK) {
    printk("Wake up by NFC field detect\n");
  }
  else if (reas & NRF_RESET_RESETREAS_RESETPIN_MASK) {
    printk("Reset by pin-reset\n");
  }
  else if (reas & NRF_RESET_RESETREAS_SREQ_MASK) {
    printk("Reset by soft-reset\n");
  }
  else if (reas) {
    printk("Reset by a different source (0x%08X)\n", reas);
  }
  else {
    printk("Power-on-reset\n");
  }

#endif
  return reas;
}
