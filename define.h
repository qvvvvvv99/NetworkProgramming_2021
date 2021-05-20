#ifndef DEFINE_H
#define DEFINE_H

#include <iostream>
#include <winsock2.h>
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <time.h>
using namespace std;



#define MAX_STR 20
///////////////////////
///////////////////////

/* 통신 코드 정의 */
#define CLEAR_CODE			0		//코드변수 초기화
#define ACCEPT_SOCKET		1		//소켓연결 완료
#define REQUEST_JOIN		11		//회원가입 요청
#define REQUEST_LOGIN		12		//로그인 요청
#define REQUEST_ACCOUNT		21		//계좌내역 요청
#define REQUEST_BALANCE		22		//잔액조회 요청
#define REQUEST_TRANSFER	31		//계좌이체 요청
#define ACCEPT_JOIN			41		//회원가입 승인
#define REJECT_JOIN			42		//회원가입 거절
#define ACCEPT_LOGIN		51		//로그인 승인
#define REJECT_LOGIN		52		//로그인 거절
#define RESULT_ACCOUNT_Y	61		//계좌조회 결과(성공)
#define RESULT_ACCOUNT_N	62		//계좌조회 결과(실패)
#define RESULT_BALANCE		63		//잔액조회 결과
#define RESULT_TRANSFER_Y	71		//계좌이체 결과(성공)
#define RESULT_TRANSFER_N	72		//계좌이체 결과(실패)
#define QUIT_MESSAGE		99		//종료 메세지

#define PORTNUM		8088		//포트번호 정의

//접속자 계좌 정보 보관
typedef struct Acc_Pass
{
	char Password[MAX_STR];
	char Account[MAX_STR];
	int balance;
	char *Record;		//이체내역 보관
}Acc_Pass;

//스레드에 넘길 인자. 소켓과 주소정보
typedef struct Clnt_Sock
{
	SOCKET hClntSock;		//클라이언트 소켓
	SOCKADDR_IN clntAddr;	//클라이언트 주소정보
}Clnt_sock;

//접속자 정보 리스트
typedef struct ClntList
{

	Clnt_Sock ClntSock;	//사용자 소켓정보
	Acc_Pass Acc_Pass;	//사용자 계좌,비밀번호, 잔액
	HANDLE hThread;		//스레드 핸들

	struct ClntList *NextNode;	//다음 노드
	struct ClntList *PreNode;	//이전노드

}ClntList;

#endif
