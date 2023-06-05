
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

#define NFC_FIELD_LED DK_LED1

/* Text message in English with its language code. */
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

static uint32_t counter = 0;

/* Text message in Norwegian with its language code. */
static uint8_t addr_text[] = {
  '0',
  '0',
  '0',
  '0',
  '-',
  '0',
  '0',
  '0',
  '0',
};
//static const uint8_t counter_lang[] = {'e', 'n'};

/* Buffer used to hold an NFC NDEF message. */
static uint8_t ndef_msg_buf[NDEF_MSG_BUF_SIZE];

static void nfc_callback(void *context,
                         nfc_t2t_event_t event,
                         const uint8_t *data,
                         size_t data_length) {
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

/**
 * @brief Function for encoding the NDEF text message.
 */
static int welcome_msg_encode(uint8_t *buffer, uint32_t *len) {
  int err;

  /* Create NFC NDEF text record description in English */
  NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_en_text_rec,
                                UTF_8,
                                en_code,
                                sizeof(en_code),
                                en_payload,
                                sizeof(en_payload));

  /* Create NFC NDEF text record description in Norwegian */
  NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_counter_text_rec,
                                UTF_8,
                                counter_text,
                                sizeof(counter_text),
                                counter_lang,
                                sizeof(counter_lang));

  //	/* Create NFC NDEF text record description in Polish */
  //	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_pl_text_rec,
  //				      UTF_8,
  //				      pl_code,
  //				      sizeof(pl_code),
  //				      pl_payload,
  //				      sizeof(pl_payload));

  /* Create NFC NDEF message description, capacity - MAX_REC_COUNT
	 * records
	 */
  NFC_NDEF_MSG_DEF(nfc_text_msg, MAX_REC_COUNT);

  /* Add text records to NDEF text message */
  err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg), &NFC_NDEF_TEXT_RECORD_DESC(nfc_en_text_rec));
  if (err < 0) {
    printk("Cannot add first record!\n");
    return err;
  }
  err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg), &NFC_NDEF_TEXT_RECORD_DESC(nfc_counter_text_rec));
  if (err < 0) {
    printk("Cannot add second record!\n");
    return err;
  }
  //	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg), &NFC_NDEF_TEXT_RECORD_DESC(nfc_pl_text_rec));
  //	if (err < 0) {
  //		printk("Cannot add third record!\n");
  //		return err;
  //	}

  err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_text_msg), buffer, len);
  if (err < 0) {
    printk("Cannot encode message!\n");
  }

  return err;
}

int init_nfc(void) {
  uint32_t len = sizeof(ndef_msg_buf);

  printk("Starting NFC Text Record example\n");

  int count = 1;
  bt_addr_le_t addr;
  bt_id_get( &addr, &count );

  snprintf("%-26s - %02x:%02x:%02x:%02x:%02x:%02x\r",
             "MAC ADDR",
             app_vars.addr.a.val[0],
             app_vars.addr.a.val[1],
             app_vars.addr.a.val[2],
             app_vars.addr.a.val[3],
             app_vars.addr.a.val[4],
             app_vars.addr.a.val[5]);  
  
  /* Set up NFC */
  if (nfc_t2t_setup(nfc_callback, NULL) < 0) {
    printk("Cannot setup NFC T2T library!\n");
    goto fail;
  }

  /* Encode welcome message */
  if (welcome_msg_encode(ndef_msg_buf, &len) < 0) {
    printk("Cannot encode message!\n");
    goto fail;
  }

  /* Set created message as the NFC payload */
  if (nfc_t2t_payload_set(ndef_msg_buf, len) < 0) {
    printk("Cannot set payload!\n");
    goto fail;
  }

  /* Start sensing NFC field */
  if (nfc_t2t_emulation_start() < 0) {
    printk("Cannot start emulation!\n");
    goto fail;
  }
  printk("NFC configuration done\n");

  return 0;

fail:
#if CONFIG_REBOOT
  sys_reboot(SYS_REBOOT_COLD);
#endif /* CONFIG_REBOOT */

  return -EIO;
}
