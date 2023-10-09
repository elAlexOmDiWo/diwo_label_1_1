
/** \file   power.c
*   \author Alex
*   \date   26.09.2023
*   
* Модуль управления питанием
*/

#include <zephyr/logging/log.h>

#define LOG_LEVEL_POWER LOG_LEVEL_DBG

LOG_MODULE_REGISTER(power, LOG_LEVEL_POWER);

#include "power.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>

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

static void set_tx_power(uint8_t handle_type, uint16_t handle, int8_t tx_pwr_lvl) {
  struct bt_hci_cp_vs_write_tx_power_level *cp;
  struct bt_hci_rp_vs_write_tx_power_level *rp;
  struct net_buf *buf, *rsp = NULL;
  int err;

  buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL, sizeof(*cp));
  if (!buf) {
    printk("Unable to allocate command buffer\n");
    return;
  }

  cp = net_buf_add(buf, sizeof(*cp));
  cp->handle = sys_cpu_to_le16(handle);
  cp->handle_type = handle_type;
  cp->tx_power_level = tx_pwr_lvl;

  err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
                             buf, &rsp);
  if (err) {
    uint8_t reason = rsp ? ((struct bt_hci_rp_vs_write_tx_power_level *)
                              rsp->data)
                             ->status
                         : 0;
    printk("Set Tx power err: %d reason 0x%02x\n", err, reason);
    return;
  }

  rp = (void *)rsp->data;
  printk("Actual Tx Power: %d\n", rp->selected_tx_power);

  net_buf_unref(rsp);
}

int set_power(int8_t tx_pwr_lvl) {
  int ret;
  struct bt_conn *default_conn;
  uint16_t default_conn_handle;

  struct bt_conn *conn = get_connect();

  if (NULL == conn)
    return -1;

  default_conn = bt_conn_ref(conn);
  
  ret = bt_hci_get_conn_handle(default_conn, &default_conn_handle);
  if (ret) {
    printk("No connection handle (err %d)\n", ret);
  }
  else {
    set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, default_conn_handle, BT_HCI_VS_LL_TX_POWER_LEVEL_NO_PREF);
  }
}

int get_power_by_index(int index) {
  const static uint8_t pwr_array[] = { 4, 3, 0, -4, -8, -12, -16, -20, -40 };

  if (index >= sizeof(pwr_array))
    return 127;
  return pwr_array[index];
}

void set_power_tx(tx_power_e power) {
  if ((power < txp4p) || (power > txp40m )) {
    LOG_DBG("Power OUT of ");
    return;
  }
  NRF_RADIO->TXPOWER = get_power_by_index( power );
}
