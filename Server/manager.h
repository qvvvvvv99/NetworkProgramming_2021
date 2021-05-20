#ifndef MANAGER_H
#define MANAGER_H

#include "define.h"


//현재 접속자 수 보기
int count_use();

//콘솔 커서 위치 변경
void gotoxy(int x, int y);

//접속자 정보 보기
void User_info();

#endif
