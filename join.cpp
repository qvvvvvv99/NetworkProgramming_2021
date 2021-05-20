#include "join.h"

//계좌 등록
bool Registration(char *Account, char *Password)
{
	FILE *fp;
	char path[100] = { NULL };	//경로입력

	strcat(path, ".\\계좌번호\\");
	strcat(path, Account);
	strcat(path, ".txt");

	HANDLE Semaphore_dest;

	//세마포어 생성
	Semaphore_dest = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, Account);

	if (Semaphore_dest == NULL)
		Semaphore_dest = CreateSemaphore(NULL, 1, 1, Account);

	//세마포어 락
	WaitForSingleObject(Semaphore_dest, INFINITE);

	//파일이 있을시 복귀
	if ((fp = fopen(path, "r")) != NULL)
	{
		fclose(fp);
		//세마포어 언락
		ReleaseSemaphore(Semaphore_dest, 1, NULL);

		return false;
	}

	//파일 없을시 생성하여 초기화
	if ((fp = fopen(path, "w")) == NULL)
	{
		fclose(fp);
		//세마포어 언락
		ReleaseSemaphore(Semaphore_dest, 1, NULL);

		return false;
	}

	fprintf(fp, "%s %s %s", Account, Password, "1000000");

	//세마포어 언락
	ReleaseSemaphore(Semaphore_dest, 1, NULL);

	fclose(fp);

	return true;
}

//로그인 승인/거절
int Login_Check(char *Account, char *Password)
{
	FILE *fp;
	char path[100] = { NULL };		//경로입력
	char Acc[MAX_STR] = { NULL };	//계좌번호 추출 버퍼
	char Pass[MAX_STR] = { NULL };	//비밀번호 추출 버퍼

	HANDLE Semaphore_dest;

	strcat(path, ".\\계좌번호\\");
	strcat(path, Account);
	strcat(path, ".txt");

	//세마포어 생성
	Semaphore_dest = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, Account);

	if (Semaphore_dest == NULL)
		Semaphore_dest = CreateSemaphore(NULL, 1, 1, Account);

	//세마포어 락
	WaitForSingleObject(Semaphore_dest, INFINITE);

	//파일이 없을시 복귀
	if ((fp = fopen(path, "r")) == NULL)
	{
		//세마포어 언락
		ReleaseSemaphore(Semaphore_dest, 1, NULL);

		return 1;	//계좌번호가 존재하지 않음
	}
	fscanf(fp, "%s %s", Acc, Pass);		//계좌정보 추출

	//세마포어 언락
	ReleaseSemaphore(Semaphore_dest, 1, NULL);

	fclose(fp);

	//비밀번호가 맞으면.
	if (strcmp(Password, Pass) == 0)
		return 2;

	//비밀번호가 틀리면.
	else
		return 0;
}
