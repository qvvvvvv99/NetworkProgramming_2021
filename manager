#include "manager.h"

extern ClntList *List_Head;	//리스트의 첫노드
extern HANDLE Sys_Manager_Sema;	//관리자 관련 세마포어 핸들

//현재 접속자 수 보기
int count_use()
{
	ClntList *Node = List_Head;
	int cnt = 0;

	//세마포어 락
	WaitForSingleObject(Sys_Manager_Sema, INFINITE);

	while (Node != NULL)
	{
		cnt++;
		Node = Node->NextNode;
	}
	//세마포어 언락
	ReleaseSemaphore(Sys_Manager_Sema, 1, NULL);

	return cnt;
}

//콘솔 커서 위치 변경
void gotoxy(int x, int y)
{
	COORD A = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), A);
}

//접속자 정보 보기
void User_info()
{
	ClntList *Node = List_Head;

	printf("┌──────┬─────┐\n");
	printf("│  아 이 피  │ 계좌번호 │\n");
	printf("├──────┼─────┤\n");

	//세마포어 락
	WaitForSingleObject(Sys_Manager_Sema, INFINITE);

	while (Node != NULL)
	{
		printf("│%11s │", inet_ntoa(Node->ClntSock.clntAddr.sin_addr));
		printf("%10s│\n",Node->Acc_Pass.Account);

		Node = Node->NextNode;
	}

	//세마포어 언락
	ReleaseSemaphore(Sys_Manager_Sema, 1, NULL);

	printf("└──────┴─────┘\n");

}
