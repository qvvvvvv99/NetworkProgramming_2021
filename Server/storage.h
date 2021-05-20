#ifndef STORAGE_H
#define STORAGE_H

#include "define.h"

//시스템 정보 기록
void system_record(int n, ...);

//이제 내역 기록
void transfer_record(const char *dest, const char *src, int amount, int balance);

#endif
