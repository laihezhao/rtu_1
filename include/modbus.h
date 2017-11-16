#ifndef __MODBUS_H_
#define __MODBUS_H_

void modbus_get_uint16_sw(const float src, uint16_t *dest);
float modbus_get_float_sw(const uint16_t *sr);
void modbus_set_float_retry(float, uint16_t *);
float modbus_get_float_retry(const uint16_t *);
int modbus_read_registers_retry(modbus_t *, int addr, int nb, uint16_t *data);
int modbus_write_registers_retry(modbus_t* ctx, int addr, int nb, const uint16_t *src);
int modbus_read_input_bits_retry(modbus_t* ctx, int addr, int nb, uint8_t *data);
int modbus_read_bits_retry(modbus_t* ctx, int addr, int nb, uint8_t * dest);
int modbus_write_bit_retry(modbus_t* ctx, int addr, int status);

int get_modbus_settings(void);
void put_modbus_settings(void);

#endif //__MODBUS_H_
