#ifndef JOIN_H
#define JOIN_H

#include "define.h"

//회원가입(계좌 등록)
bool Registration(char *Account, char *Password);

//로그인 승인/거절
int Login_Check(char *Account, char *Password);


#endif
