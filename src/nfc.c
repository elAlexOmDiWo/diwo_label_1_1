
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

#include <nrf52.h>

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

int set_nfc_id( uint8_t* addr ) {
//  NRF_NFCT_Type* nfct = (NRF_NFCT_Type *)0x40005000;  
  
  uint32_t nfc_id = 0;

  uint32_t* preg = (uint32_t * )0x40005590;
  nfc_id = addr[3];
  nfc_id <<= 8;
  nfc_id |= addr[2];
  nfc_id <<= 8;
  nfc_id |= addr[1];
  nfc_id <<= 8;
  nfc_id |= addr[0];  
  
  *preg++ = nfc_id;
//  nfct->NFCID1_LAST = nfc_id;

  nfc_id = 0;
  nfc_id = addr[5];
  nfc_id <<= 8;
  nfc_id |= addr[4];  

  *preg++ = nfc_id;
//  nfct->NFCID1_2ND_LAST = nfc_id;  
  
  return 0;
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
//  
//  NRF_NFCT_Type* nfct = (NRF_NFCT_Type *)0x40005000;  
//  
//  uint32_t nfc_id = 0;  
//  
//  nfc_id = (addr.a.val[0] << 4) | (addr.a.val[1] << 2) | addr.a.val[2];
//  nfct->NFCID1_2ND_LAST = nfc_id;
//    
//  nfc_id = (addr.a.val[3] << 6) | (addr.a.val[4] << 4) | (addr.a.val[5] << 2) | 0;
//  nfct->NFCID1_LAST = nfc_id;  

  set_nfc_id( addr.a.val );
  
//#define UID_ST        0x88
//  
//  uint8_t uid[10];// = { 0x00, addr.a.val[5], addr.a.val[4], addr.a.val[3],  }
//  uid[0] = 0x00;
//  uid[1] = 0x01;
//  uid[2] = 0x02;
//  uid[3] = UID_ST ^ uid[0] ^ uid[1] ^ uid[2];
//  
//  uid[4] = 0x03;
//  uid[5] = 0x04;
//  uid[6] = 0x05;
//  uid[7] = 0x06;  
//
//  uid[8] = uid[3] ^ uid[4] ^ uid[5] ^ uid[6];
//  uid[9] = 0x01;
//  
//  uint8_t temp_uid[10];
//  nrfx_err_t nrf_err = NRFX_SUCCESS;
  
 
//  NRF_NFCT_Type* nfct = (NRF_NFCT_Type *)0x40005000;  
//  
//  uint32_t nfc_id = 0;
  
//  nfc_id = ( uid[0] << 4 ) | ( uid[1] << 2 ) | uid[2];
//  nfct->NFCID1_2ND_LAST = nfc_id;
//  
//  nfc_id = (uid[3] << 6) | (uid[4] << 4) | (uid[5] << 2) | uid[6];
//  nfct->NFCID1_LAST = nfc_id;  
  
//  nrf_err = nfc_platform_nfcid1_default_bytes_get( temp_uid, sizeof( temp_uid ) );  
//  if( NRFX_SUCCESS != nrf_err ) {
//    printk( "Error read UID\n" );
//  }

//  nfc_id = (uid[0] << 4) | (uid[1] << 2) | uid[2];
//  nfct->NFCID1_2ND_LAST = nfc_id;
//  
//  nfc_id = (uid[3] << 6) | (uid[4] << 4) | (uid[5] << 2) | uid[6];
//  nfct->NFCID1_LAST = nfc_id;  
  
//  err = nfc_t2t_internal_set( uid, sizeof( uid ));
//  if( 0 != err ) {
//    printk( "Error write UID\n" );
//  }

//  nrf_err = nfc_platform_nfcid1_default_bytes_get( temp_uid, sizeof( temp_uid ) );   
//  if( NRFX_SUCCESS != nrf_err ) {
//    printk( "Error read UID\n" );
//  }
  
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
