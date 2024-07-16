#ifndef QBUS_HPP
#define QBUS_HPP

void QbusInit();
void QDMAbegin();
void QDMAend();
uint16_t Qread(uint32_t addr);
void Qwrite(uint32_t addr, uint16_t data);

#endif // QBUS_HPP
