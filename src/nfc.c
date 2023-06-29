
/** \file   nfc.c
    \author Alex
*/

#include "nfc.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <nfc_t2t_lib.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>

#include <zephyr/bluetooth/bluetooth.h>

#define MAX_REC_COUNT 2
#define NDEF_MSG_BUF_SIZE 128

static const uint8_t en_payload[] = {
  'D',
  'i',
  'W',
  'o',
  ' ',
  'L',
  'a',
  'b',
  'e',
  'l',
  ' ',
  'N',
  'F',
  'C',
  '0',
  '.',
  '1',
};
static const uint8_t en_code[] = {'e', 'n'};

static uint8_t addr_text[] = {
  '0',
  '0',
  '0',
  '0',
  '0',
  '0',
  '0',
  '0',
  '0',
  '0',
  '0',
  '0',
  0
};

static uint8_t ndef_msg_buf[NDEF_MSG_BUF_SIZE];

static void nfc_callback( void *context, nfc_t2t_event_t event, const uint8_t *data, size_t data_length ) {
  ARG_UNUSED(context);
  ARG_UNUSED(data);
  ARG_UNUSED(data_length);

  switch (event) {
    case NFC_T2T_EVENT_FIELD_ON: {
      break;
    }
    case NFC_T2T_EVENT_FIELD_OFF: {
      break;
    }
    default: {
      break;
    }
  }
}

static int welcome_msg_encode(uint8_t *buffer, uint32_t *len) {
  int err;

  NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_en_text_rec,
                                UTF_8,
                                en_code,
                                sizeof(en_code),
                                en_payload,
                                sizeof(en_payload));

  NFC_NDEF_TEXT_RECORD_DESC_DEF(ble_addr_text_rec,
                                UTF_8,
                                en_code,
                                sizeof(en_code),                                
                                addr_text,
                                sizeof(addr_text));

  NFC_NDEF_MSG_DEF(nfc_text_msg, MAX_REC_COUNT);
  
  err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg), &NFC_NDEF_TEXT_RECORD_DESC(nfc_en_text_rec));
  if (err < 0) {
    printk("Cannot add first record!\n");
    return err;
  }

  err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg), &NFC_NDEF_TEXT_RECORD_DESC(ble_addr_text_rec));
  if (err < 0) {
    printk("Cannot add second record!\n");
    return err;
  }

  err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_text_msg), buffer, len);
  if (err < 0) {
    printk("Cannot encode message!\n");
  }

  return err;
}

int init_nfc(void) {
  uint32_t len = sizeof(ndef_msg_buf);

  int count = 1;
  bt_addr_le_t addr;
  bt_id_get( &addr, &count );

  snprintk( addr_text, sizeof( addr_text ), "%02x%02x%02x%02x%02x%02x",
             addr.a.val[5],
             addr.a.val[4],
             addr.a.val[3],
             addr.a.val[2],
             addr.a.val[1],
             addr.a.val[0]);

  if (nfc_t2t_setup(nfc_callback, NULL) < 0) {
    printk("Cannot setup NFC T2T library!\n");
    goto fail;
  }

  if (welcome_msg_encode(ndef_msg_buf, &len) < 0) {
    printk("Cannot encode message!\n");
    goto fail;
  }

  if (nfc_t2t_payload_set(ndef_msg_buf, len) < 0) {
    printk("Cannot set payload!\n");
    goto fail;
  }

  if (nfc_t2t_emulation_start() < 0) {
    printk("Cannot start emulation!\n");
    goto fail;
  }
//  printk("NFC configuration done\n");

  return 0;

fail:
  return -EIO;
}
