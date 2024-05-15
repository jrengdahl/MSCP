#ifndef BOUNDARIES_H
#define BOUNDARIES_H

extern uint32_t _memory_start, _memory_end;
extern uint32_t _memory2_start, _memory2_end;

extern uint32_t _text_start, _text_end;
extern uint32_t _sdata, _edata;
extern uint32_t _sbss, _ebss;
extern uint32_t _heap_start, _heap_end;
extern uint32_t _stack_start, _stack_end;

#endif // BOUNDARIES_H
