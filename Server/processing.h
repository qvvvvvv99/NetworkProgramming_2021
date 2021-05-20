#ifndef PROCESSING_H
#define PROCESSING_H

#include "define.h"

//잔액정보 추출
int Proc_Balance(char * Account);

//계좌이체 처리
int Proc_Transfer(char *Account, char *target, int amount);

//이체내역 조회 처리(반환값 0이면 실패)
int Proc_History(char *Account, char **record);



#endif
