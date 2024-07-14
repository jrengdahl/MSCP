#ifndef QBUS_HPP
#define QBUS_HPP

void QDMAbegin();
void QDMAend();
void Qaddr(uint32_t addr, int write, int byte=0);
uint16_t Qread();
void Qwrite(uint16_t data);
void QWriteWord(uint32_t addr, uint16_t data);
uint16_t QReadWord(uint32_t addr);


#endif // QBUS_HPP
