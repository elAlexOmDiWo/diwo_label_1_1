
#include "settings.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/drivers/gpio.h>

#include "main.h"

#define BT_DEVICE_NAME "Alex Test device"

#define DEVICE_NAME "Alex test"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

struct bt_uuid_128 setting_service_uuid =
    BT_UUID_INIT_128(0xf0, 0xde, 0xbc, 0x9a, 0xd8, 0x65, 0x11, 0xed, 0xb1, 0x6f,
                     0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66);

struct bt_uuid_128 app_settings_frequency_uuid =
    BT_UUID_INIT_128(0xf1, 0xde, 0xbc, 0x9a, 0xd8, 0x65, 0x11, 0xed, 0xb1, 0x6f,
                     0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66);

struct bt_uuid_128 app_settings_power_uuid =
    BT_UUID_INIT_128(0xf2, 0xde, 0xbc, 0x9a, 0xd8, 0x65, 0x11, 0xed, 0xb1, 0x6f,
                     0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66);

struct bt_uuid_128 app_settings_hit_threshold_uuid =
    BT_UUID_INIT_128(0xf3, 0xde, 0xbc, 0x9a, 0xd8, 0x65, 0x11, 0xed, 0xb1, 0x6f,
                     0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66);

struct bt_uuid_128 app_settings_fall_threshold_uuid =
    BT_UUID_INIT_128(0xf4, 0xde, 0xbc, 0x9a, 0xd8, 0x65, 0x11, 0xed, 0xb1, 0x6f,
                     0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34,
                  0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
};

// Имя видимое при сканированиии
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

ssize_t write_period(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (offset + len > sizeof(app_settings.adv_period )) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }
  app_settings.adv_period = *(int *)buf;
  return len;
}

ssize_t read_period(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset) {
  const char *value = attr->user_data;                 
  return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

ssize_t write_power(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (offset + len > sizeof(app_settings.power)) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }

  app_settings.power = *(int *)buf;
  return len;
}

ssize_t read_power(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset) {
  const char *value = attr->user_data;
  return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

ssize_t write_hit_threshold( struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (offset + len > sizeof(app_settings.hit_threshold)) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }
  app_settings.hit_threshold = *(int *)buf;
  return len;
}

ssize_t read_hit_threshold( struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset ) {
  const char *value = attr->user_data;
  return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

ssize_t write_fall_threshold( struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags ) {
  if (offset + len > sizeof(app_settings.fall_threshold)) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }
  app_settings.fall_threshold = *(int *)buf;
  return len;
}

ssize_t read_fall_threshold( struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset ) {
  const char *value = attr->user_data;
  return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

//static struct bt_gatt_attr app_settings_attrs[] = {
//    /* Vendor Primary Service Declaration */
//    BT_GATT_PRIMARY_SERVICE(&setting_service_uuid),
//    BT_GATT_CHARACTERISTIC( &app_settings_frequency_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
//        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE,
//        read_period, write_period, &app_settings.adv_period ),
//    BT_GATT_CHARACTERISTIC( &app_settings_power_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
//        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE,
//        read_power, write_power, &app_settings.power),
//    BT_GATT_CHARACTERISTIC(&app_settings_hit_threshold_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE, 
//        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE,
//        read_hit_threshold, write_hit_threshold, &app_settings.hit_threshold),
//    BT_GATT_CHARACTERISTIC(&app_settings_fall_threshold_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
//        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE,
//        read_fall_threshold, write_fall_threshold, &app_settings.fall_threshold),
//};

//static struct bt_gatt_service app_settings_svc = BT_GATT_SERVICE(app_settings_attrs);

int run_device(void) {

  struct bt_le_adv_param param;
  int err = 0;

//  SOC_FLASH_NRF_UICR

  //  if (0 != (err = init_board())) {
//    printk("Error Init board - %d\n", err);
//  }

//  if (0 != (err = bt_enable(NULL))) {
//    printk("Error BT Enable - %d\n", err);
//  }

  bt_set_name("DiWo Label");
  param.options = BT_LE_ADV_OPT_USE_NAME;

  if (0 != (err = (bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
                                   ARRAY_SIZE(sd))))) {
    printk("Error BT Start - %d\n", err);
    switch (err) {
      case ECONNREFUSED: {
        break;
      }
    }
  }

//  bt_gatt_service_register(&app_settings_svc);

  k_sleep(K_SECONDS( 100 ));
  
//  while (1) {
//    k_sleep(K_SECONDS(1));
//  }
  
  return 0;
}
