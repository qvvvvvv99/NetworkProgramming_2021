#include "linkedlist.h"

extern ClntList *List_Head;	//리스트의 첫노드
extern HANDLE Sys_Manager_Sema;	//관리자 관련 세마포어 핸들

//접속자 노드 추가
ClntList* Add_ClntList()
{
	ClntList *New = NULL;		//새로 할당받을 노드
	ClntList *Node = List_Head;		//마지막 노드 찾기용

	
	New = (ClntList *)malloc(sizeof(ClntList));	//노드 메모리 할당

	New->NextNode = NULL;
	New->PreNode = NULL;

	//세마포어 락
	WaitForSingleObject(Sys_Manager_Sema, INFINITE);

	if (Node == NULL)	//첫 노드이면 헤드로 지정
	{
		List_Head = New;

		//세마포어 언락
		ReleaseSemaphore(Sys_Manager_Sema, 1, NULL);

		return New;
	}
		

	while (Node->NextNode != NULL)	//리스트 마지막 노드 찾기
		Node = Node->NextNode;

	//마지막 노드에 새 노드 연결
	Node->NextNode = New;
	New->PreNode = Node;

	//세마포어 언락
	ReleaseSemaphore(Sys_Manager_Sema, 1, NULL);

	return New;
}


//접속자 종료시 노드 제거
void Delete_ClntNode(ClntList *Node)
{
	//임계구역
	//세마포어 락
	WaitForSingleObject(Sys_Manager_Sema, INFINITE);

	ClntList *p = Node->PreNode;	//이전 노드
	ClntList *n = Node->NextNode;	//이후 노드

	/* 삭제 노드 전후간 연결 */
	if (p != NULL)	//이전 노드 있을시
	{
		if (n != NULL)	//다음 노드가 있을시
		{
			p->NextNode = n;
			n->PreNode = p;
		}
		else	//다음 노드가 없을시
		{
			p->NextNode = NULL;
		}
	}

	else	//이전 노드가 없을시
	{
		if (n != NULL)		//다음 노드가 있을시
		{
			List_Head = n;
			n->PreNode = NULL;
		}
		else		//다음 노드가 없을시
		{
			List_Head = NULL;
		}
	}

	closesocket(Node->ClntSock.hClntSock);
	free(Node);

	//세마포어 언락
	ReleaseSemaphore(Sys_Manager_Sema, 1, NULL);
	//임계구역
}

//모든 접속자 노드 제거
void Delete_AllNode()
{
	ClntList *Node = NULL;	//삭제 노드 저장

	//세마포어 락
	WaitForSingleObject(Sys_Manager_Sema, INFINITE);

	while (List_Head != NULL)
	{
		Node = List_Head;
		List_Head = List_Head->NextNode;

		closesocket(Node->ClntSock.hClntSock);
		free(Node);
	}
	//세마포어 언락
	ReleaseSemaphore(Sys_Manager_Sema, 1, NULL);
}

//접속자 노드 정렬
void sort_list()
{
	ClntList *Node1 = List_Head;	//현재 노드(메인 루프)
	ClntList *Node2 = NULL;	//현재 노드(서브 루프)
	ClntList *p = NULL;		//다음 노드
	ClntList *r = NULL;		//이전 노드
	ClntList *tmp = NULL;
	Acc_Pass	tmp1;
	Clnt_sock	tmp2;
	HANDLE		tmp3;

	//세마포어 락
	WaitForSingleObject(Sys_Manager_Sema, INFINITE);

	//선택정렬
	while (Node1 != NULL)
	{
		Node2 = Node1->NextNode;
		tmp = Node1;
		while (Node2 != NULL)
		{
			//오른차순 비교(가장 작은 값을 tmp에 보관)
			if (atoi(Node2->Acc_Pass.Account) < atoi(tmp->Acc_Pass.Account))
				tmp = Node2;

			Node2 = Node2->NextNode;
		}
		//스왑
		tmp1 = tmp->Acc_Pass;
		tmp2 = tmp->ClntSock;
		tmp3 = tmp->hThread;
		tmp->Acc_Pass = Node1->Acc_Pass;
		tmp->ClntSock = Node1->ClntSock;
		tmp->hThread = Node1->hThread;
		Node1->Acc_Pass = tmp1;
		Node1->ClntSock = tmp2;
		Node1->hThread = tmp3;

		Node1 = Node1->NextNode;
	}

	//세마포어 언락
	ReleaseSemaphore(Sys_Manager_Sema, 1, NULL);
}
