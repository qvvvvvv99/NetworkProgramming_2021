#include "storage.h"


extern HANDLE Sys_Record_Sema;	//시스템 기록관련 세마포어 핸들

//시스템 정보 기록
void system_record(int n, ...)
{
	va_list arg;	//가변인자 변수

	time_t sys_localtime;
	struct tm *systime;	//로컬 시스템 시간 기록
	int year;		//년 저장
	int month;		//월 저장
	int day;		//일 저장
	int hour;		//시 저장
	int minute;		//분 저장
	int sec;		//초 저장

	FILE *fp;

	char path[100] = ".\\시스템기록\\";	//경로입력
	char message[100] = { NULL };		//메세지 변수
	char *account;						//주체 계좌번호 지정
	char *target;						//상대 계좌번호 지정
	char code;							//코드별 메세지 저장
	
	char Cyear[5] = { NULL };	//년 문자열
	char Cmonth[3] = { NULL };	//월 문자열
	char Cday[3] = { NULL };	//일 문자열
	char Cdays[15] = { NULL };	//년월일 문자열


	/////////////////////////// 시간 설정 시작
	sys_localtime = time(NULL);
	systime = localtime(&sys_localtime);

	year = systime->tm_year+1900;	//1900년 기준이므로 더함
	month = systime->tm_mon+1;		//0~11 표현하여 더함
	day = systime->tm_mday;
	hour = systime->tm_hour;
	minute = systime->tm_min;
	sec = systime->tm_sec;

	//문자열 변환
	itoa(year, Cyear, 10);
	itoa(month, Cmonth, 10);
	itoa(day, Cday, 10);

	//문자열 연결
	strcat(Cdays, Cyear);
	strcat(Cdays, Cmonth);
	strcat(Cdays, Cday);
	strcat(Cdays, ".txt");	//확장자 지정

	//////////////////////// 시간 설정 끝


	va_start(arg, n);	//가변인자 갯수 설정

	code = va_arg(arg, char);		//첫번째 인자, 해당 코드 추출
	account = va_arg(arg, char*);	//두번째 인자, 주체 계좌 추출
	
	strcat(path, Cdays);	//파일 경로 지정
	fp = fopen(path, "a");

	//코드별 메세지 기록
	switch (code)
	{
	case ACCEPT_SOCKET:
		strcpy(message, "연결");
		break;
	case REQUEST_JOIN:
		strcpy(message, "회원가입 요청");
		break;
	case REQUEST_LOGIN:
		strcpy(message, "로그인 요청");
		break;
	case REQUEST_ACCOUNT:
		strcpy(message, "이체내역 요청");
		break;
	case REQUEST_BALANCE:
		strcpy(message, "잔액 요청");
		break;
	case REQUEST_TRANSFER:
		strcpy(message, "이체 요청");
		break;
	case ACCEPT_JOIN:
		strcpy(message, "회원가입 승인");
		break;
	case REJECT_JOIN:
		strcpy(message, "회원가입 거절");
		break;
	case ACCEPT_LOGIN:
		strcpy(message, "로그인 승인");
		break;
	case REJECT_LOGIN:
		strcpy(message, "로그인 거절");
		break;
	case RESULT_ACCOUNT_Y:
		strcpy(message, "이체내역 전송");
		break;
	case RESULT_ACCOUNT_N:
		strcpy(message, "이체내역 전송 실패");
		break;
	case RESULT_BALANCE:
		strcpy(message, "잔액 전송");
		break;
	case RESULT_TRANSFER_Y:
		strcpy(message, "이체 성공");
		break;
	case RESULT_TRANSFER_N:
		strcpy(message, "이체 실패");
		break;
	case QUIT_MESSAGE:
		strcpy(message, "로그아웃");
		break;
	}

	//세마포어 락
	WaitForSingleObject(Sys_Record_Sema, INFINITE);
	
	//코드별 시스템 파일기록 및 출력
	switch (n)
	{
	case 2:
		fprintf(fp, "%d/%d/%d %d:%d:%d\t%s %s\n", year, month, day, hour, minute, sec, account, message);
		fprintf(stdout, "%d/%d/%d %d:%d:%d\t%s %s\n", year, month, day, hour, minute, sec, account, message);
		break;
	case 3:
		target = va_arg(arg, char*);
		fprintf(fp, "%d/%d/%d %d:%d:%d\t%s %s %s\n", year, month, day, hour, minute, sec, account, target, message);
		fprintf(stdout, "%d/%d/%d %d:%d:%d\t%s %s %s\n", year, month, day, hour, minute, sec, account, target, message);
		break;
	}

	//세마포어 언락
	ReleaseSemaphore(Sys_Record_Sema, 1, NULL);

	fclose(fp);
}

//이제 내역 기록
void transfer_record(const char *dest,const char *src, int amount, int balance)
{
	time_t sys_localtime;
	struct tm *systime;	//로컬 시스템 시간 기록
	int year;		//년 저장
	int month;		//월 저장
	int day;		//일 저장
	int hour;		//시 저장
	int minute;		//분 저장
	int sec;		//초 저장

	FILE *fp_dest;	//입금자 파일
	FILE *fp_src;	//입금대상 파일
	FILE *fp_target;//입금대상 계좌 잔액조회 파일
	
	HANDLE Semaphore_dest;	//목적 세마포어 핸들 생성
	HANDLE Semaphore_src;	//자료 세마포어 핸들 생성

	char path_dest[100] = ".\\이체내역\\";	//입금자 경로입력
	char path_src[100] = ".\\이체내역\\";	//이체대상 경로입력
	char path_target[100] = ".\\계좌번호\\";//이체대상 계좌번호(잔액조회)

	int target_balance;
	int tmp_;

	//세마포어 생성(이체내역 파일 2개)
	Semaphore_dest = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, dest);
	Semaphore_src = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, src);

	if (Semaphore_dest == NULL)
		Semaphore_dest = CreateSemaphore(NULL, 1, 1, dest);
	if (Semaphore_src == NULL)
		Semaphore_src = CreateSemaphore(NULL, 1, 1, src);

	//cout << (LPCTSTR)dest << ' ' << (LPCTSTR)src << endl;
	/////////////////////////// 시간 설정 시작
	sys_localtime = time(NULL);
	systime = localtime(&sys_localtime);

	year = systime->tm_year + 1900;	//1900년 기준이므로 더함
	month = systime->tm_mon + 1;		//0~11 표현하여 더함
	day = systime->tm_mday;
	hour = systime->tm_hour;
	minute = systime->tm_min;
	sec = systime->tm_sec;
	//////////////////////// 시간 설정 끝

	strcat(path_dest, dest);	//경로지정(파일이름: 계좌번호)
	strcat(path_src, src);
	strcat(path_target, src);

	strcat(path_dest, ".txt");	//파일 경로 지정
	strcat(path_src, ".txt");
	strcat(path_target, ".txt");


	//세마포어 락
	WaitForSingleObject(Semaphore_dest, INFINITE);
	WaitForSingleObject(Semaphore_src, INFINITE);

	//파일 열기
	fp_dest = fopen(path_dest, "a");
	fp_src = fopen(path_src, "a");
	fp_target = fopen(path_target, "r");

	//계좌잔액 추출
	fscanf(fp_target, "%d %d %d", &tmp_, &tmp_, &target_balance);

	//입금자 기록
	fprintf(fp_dest, "%d/%d/%d %2d:%2d:%2d\n┗%s에서 %s으로 %8d원 출금\n//잔액 %10d원//\n", year, month, day, hour, minute, sec, dest, src, amount, balance);
	
	//이체대상 기록
	fprintf(fp_src, "%d/%d/%d %2d:%2d:%2d\n┗%s에서 %s으로 %8d원 입금\n//잔액 %10d원//\n", year, month, day, hour, minute, sec, dest, src, amount, target_balance);


	//////////////////
	//테스트용
/*	for (int i = 0; i < 10; i++)
	{
		Sleep(1000);
		cout << i << ' ' << Semaphore_dest << ' ' << Semaphore_src <<endl;
	}*/
	///////////////////////

	//세마포어 언락
	ReleaseSemaphore(Semaphore_dest, 1, NULL);
	ReleaseSemaphore(Semaphore_src, 1, NULL); 
	
	fclose(fp_dest);
	fclose(fp_src);
	fclose(fp_target);
}
