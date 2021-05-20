
#include "define.h"
#include "join.h"
#include "processing.h"
#include "storage.h"
#include "linkedlist.h"
#include "manager.h"


DWORD WINAPI thread_main(void *Node);			//사용자 스레드 메인함수
DWORD WINAPI thread_input(void *null);			//서버 스레드 키보드 입력 함수
DWORD WINAPI thread_count(void *null);			//사용자 스레드 메인함수

void ErrorHandling(char *Message);				//에러메세지 출력


char not_account[] = "계좌번호가 없습니다.";
char not_password[] = "비밀번호가 틀립니다.";

int select_input;	//키보드 입력값 저장 //0 입력시 프로그램 종료

Clnt_sock Tmp_Sock;
SOCKET hServSock;		//서버 소켓

ClntList *List_Head = NULL;	//리스트 헤드

HANDLE Sys_Record_Sema;	//시스템 기록관련 세마포어 핸들
HANDLE Sys_Manager_Sema;	//관리자 관련 세마포어 핸들

int main(void)
{

	//TCHAR account[MAX_STR];
	//TCHAR password[MAX_STR];

	select_input = 1;	//select 초기화

	/*관련 폴더 생성*/
	system("mkdir 계좌번호");
	system("mkdir 이체내역");
	system("mkdir 시스템기록");

	/* 소켓 관련 선언 */

	SOCKADDR_IN servAddr;	//서버 주소정보
	int clntAddrSize;		//주소 사이즈
	WSADATA wsaData;		//라이브러리정보

	HANDLE hThread;			//스레드 핸들
	HANDLE hThread_input;
	HANDLE hThread_count;

	
	ClntList *Node = NULL;


	CONSOLE_CURSOR_INFO cursorInfo;

	//콘솔 커서 숨기기
	GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

	////////////////////////////////////////////////////
	//세마포어 핸들 생성
	Sys_Record_Sema = CreateSemaphore(NULL, 1, 1, NULL);
	Sys_Manager_Sema = CreateSemaphore(NULL, 1, 1, NULL);
	////////////////////////////////////////////////////


	//라이브러리 등록
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSA 오류");

	cout << "WSA 라이브러리 등록" << endl;

	//소켓 생성
	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hServSock == INVALID_SOCKET)
		ErrorHandling("SOCKET 오류");

	cout << "SOCKET 생성" << endl;

	//주소정보 초기화
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(PORTNUM);

	cout << "주소정보 초기화" << endl;

	//바인딩
	if (bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
		ErrorHandling("bind 오류");

	cout << "바인딩 완료" << endl;

	//연결 준비
	if (listen(hServSock, 5) == SOCKET_ERROR)
		ErrorHandling("listen 오류");

	cout << "연결 대기중" << endl;


	//서버 사용자 키보드 입력 스레드(서버 개인작업용) //Node => Head
	hThread_input = CreateThread(NULL, 0, thread_input, NULL, 0, NULL);

	//접속자 정보 출력 스레드
	hThread_count = CreateThread(NULL, 0, thread_count, NULL, 0, NULL);
	
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	//통신 루프 시작

	while (select_input != 0)
	{
		clntAddrSize = sizeof(Tmp_Sock.clntAddr);

		//클라이언트 접속 대기
		Tmp_Sock.hClntSock = accept(hServSock, (SOCKADDR*) &(Tmp_Sock.clntAddr), &clntAddrSize);
		if (Tmp_Sock.hClntSock == -1)
			ErrorHandling("accept 오류");
		else
		{
			Node = Add_ClntList();	//새로운 접속자 노드 할당
			Node->ClntSock = Tmp_Sock;	//소켓정보 저장

			//cout << inet_ntoa(Tmp_Sock.clntAddr.sin_addr) << " 연결 성공!!" << endl;
			system_record(2, ACCEPT_SOCKET, inet_ntoa(Tmp_Sock.clntAddr.sin_addr));

			hThread = CreateThread(NULL, 0, thread_main, (void *) Node, 0, NULL);	//스레드 할당
			Node->hThread = hThread;	//스레드값 저장
		}
	}

	//통신 루프 종료
	/////////////////////////////////////////
	/////////////////////////////////////////
	/////////////////////////////////////////

	cout << "종료 중입니다. 기다려 주세요." << endl;

	WaitForSingleObject(hThread_input, INFINITE);
	WaitForSingleObject(hThread_count, INFINITE);



	Delete_AllNode();

	//Sleep(5000);	//종료시 1초간 대기


/* 임시 저장 삭제해야할것들
	length = recv(hClntSock, a.Account, MAX_STR, 0);
	cout << a.Account << endl;


	length = recv(hClntSock, a.Password, MAX_STR, 0);
	cout << a.Password << endl;

	//회원가입, 계좌가 존재하면 실패
	char flag;
	if (Registration(a.Account, a.Password))
		flag = '1';
	else
		flag = '2';

	send(hClntSock, &flag, sizeof(char), 0);		//성공 코드 전송

	cout << """""" << flag << endl;
	cin >> message;

*/

	return 0;
}

DWORD WINAPI thread_main(void *Node)
{
	ClntList *ClntNode = (ClntList *) Node;	//접속자정보 노드

	SOCKET hClntSock = ClntNode->ClntSock.hClntSock;	//클라이언트 소켓 저장
	SOCKADDR_IN clntAddr = ClntNode->ClntSock.clntAddr;	//클라이언트 주소 저장

	int length;			//수신량 기록
	char recv_code;		//수신코드 저장
	char send_code;		//송신코드 저장
	char IP_Addrs[16];	//아이피 주소 저장
	strcpy(IP_Addrs,inet_ntoa(clntAddr.sin_addr));
	Acc_Pass Clnt;		//계좌 비밀번호 정보 기록
	
	Clnt.Account[0] = NULL;

	int trans_amount = 0;			//이체 금액 보관
	char target_account[MAX_STR];	//이체 계좌 보관

	//cout << IP_Addrs << " " << "스레드 생성완료" << endl;

	while (select_input != 0)
	{
		recv_code = CLEAR_CODE;		//recv_code 초기화
		recv(hClntSock, &recv_code, sizeof(char), 0);		//요구 코드 수신

		switch (recv_code)
		{
		case REQUEST_JOIN:		//회원가입

			//회원가입시 출력문 없어도 됨..
			//cout << IP_Addrs << " " << "회원가입 " << endl;

			//데이터 길이 수신 후 계좌번호 수신
			recv(hClntSock, (char *)&length, sizeof(unsigned int), 0);
			recv(hClntSock, Clnt.Account, length, 0);

			//데이터 길이 수신 후 비밀번호 수신
			recv(hClntSock, (char *)&length, sizeof(unsigned int), 0);
			recv(hClntSock, Clnt.Password, length, 0);

			//회원가입 함수 호출
			if (Registration(Clnt.Account, Clnt.Password))
			{
				//회원가입 승인 코드 전송
				send_code = ACCEPT_JOIN;
				//cout << IP_Addrs << " " << Clnt.Account << " 회원가입 승인!!" << endl;
				system_record(2, ACCEPT_JOIN, IP_Addrs);
			}
			else
			{	//회원가입 거절 코드 전송
				send_code = REJECT_JOIN;
				//cout << IP_Addrs << " " << Clnt.Account << " 회원가입 거절!!" << endl;
				system_record(2, REJECT_JOIN, IP_Addrs);
			}
			send(hClntSock, &send_code, sizeof(char), 0);

			break;

			////////////////////////////////////////////////////
			////////////////////////////////////////////////////
			////////////////////////////////////////////////////
		case REQUEST_LOGIN:		//로그인

			//로그인시 출력문 없어도됨..
			//cout << IP_Addrs << " " << "로그인 " << endl;

			//데이터 길이 수신 후 계좌번호 수신
			recv(hClntSock, (char *)&length, sizeof(unsigned int), 0);
			recv(hClntSock, Clnt.Account, length, 0);

			//데이터 길이 수신 후 비밀번호 수신
			recv(hClntSock, (char *)&length, sizeof(unsigned int), 0);
			recv(hClntSock, Clnt.Password, length, 0);

			//로그인 승인/거절 판별
			switch (Login_Check(Clnt.Account, Clnt.Password))
			{
			case 0:		//비밀번호 틀림

				//로그인 거절코드 전송
				send_code = REJECT_LOGIN;
				send(hClntSock, &send_code, sizeof(char), 0);

				length = strlen(not_password);
				send(hClntSock, (char *) &(++length), sizeof(unsigned int), 0);
				send(hClntSock, not_password, length, 0);

				//cout << IP_Addrs << " " << "로그인 실패 비밀번호 틀림" << endl;
				system_record(3, REJECT_LOGIN, IP_Addrs, "비밀번호 틀림");

				break;
			case 1:		//계좌번호 없음

				//로그인 거절코드 전송
				send_code = REJECT_LOGIN;
				send(hClntSock, &send_code, sizeof(char), 0);

				length = strlen(not_account);
				send(hClntSock, (char *) &(++length), sizeof(unsigned int), 0);
				send(hClntSock, not_account, length, 0);

				//cout << IP_Addrs << " " << "로그인 실패 계좌번호 없음 " << endl;
				system_record(3, REJECT_LOGIN, IP_Addrs, "계좌번호 없음");

				break;
			case 2:		//로그인 성공

				//로그인 승인코드 전송
				send_code = ACCEPT_LOGIN;
				send(hClntSock, &send_code, sizeof(char), 0);


				//접속자 계좌,비밀번호 저장
				strcpy(ClntNode->Acc_Pass.Account, Clnt.Account);
				strcpy(ClntNode->Acc_Pass.Password, Clnt.Password);

				//cout << IP_Addrs << " " << Clnt.Account << " "  << "로그인 승인" << endl;
				system_record(3, ACCEPT_LOGIN, IP_Addrs, Clnt.Account);

				break;
			}

			break;


			////////////////////////////////////////////////////
			////////////////////////////////////////////////////
			////////////////////////////////////////////////////
		case REQUEST_ACCOUNT :	//계좌내역 조회 요청

			//cout << Clnt.Account << "이체 내역 조회" << endl;

			//이체내역 조회, 0이면 내역없음
			if (Proc_History(Clnt.Account, &Clnt.Record) == 0)
			{
				send_code = RESULT_ACCOUNT_N;		//실패코드 전송
				send(hClntSock, &send_code, sizeof(char), 0);
				break;
			}
			else
			{
				send_code = RESULT_ACCOUNT_Y;		//성공코드 전송
				send(hClntSock, &send_code, sizeof(char), 0);
			}
			length = strlen(Clnt.Record);	//레코드 길이 전송
			send(hClntSock, (char *)&(++length), sizeof(unsigned int), 0);

			send(hClntSock, Clnt.Record, length, 0);	//내역 전송

			free(Clnt.Record);
			break;


			////////////////////////////////////////////////////
			////////////////////////////////////////////////////
			////////////////////////////////////////////////////
		case REQUEST_BALANCE :	//잔액조회 요청

			//잔액 추출하기
			Clnt.balance = Proc_Balance(Clnt.Account);

			ClntNode->Acc_Pass.balance = Clnt.balance;	//잔액정보 저장

			//잔액 결과 송신///없어도 될거같음..
//			send_code = RESULT_BALANCE;
//			send(hClntSock, &send_code, sizeof(char), 0);

			send(hClntSock, (char *) &(Clnt.balance), sizeof(int), 0);


			break;


			////////////////////////////////////////////////////
			////////////////////////////////////////////////////
			////////////////////////////////////////////////////
		case REQUEST_TRANSFER :	//계좌이체 요청

			//이체대상 계좌번호 수신
			recv(hClntSock, (char *)&length, sizeof(unsigned int), 0);
			recv(hClntSock, (char *)target_account, length, 0);

			//이체금액 수신
			recv(hClntSock, (char *)&trans_amount, sizeof(int), 0);

			//계좌 이체 실패시(계좌없음)
			if (Proc_Transfer(Clnt.Account, target_account, trans_amount) == 1)
			{
				//cout << Clnt.Account << "에서" << target_account << "으로 이체실패 계좌없음" << endl;
				system_record(3, RESULT_TRANSFER_N, Clnt.Account, target_account);

				send_code = RESULT_TRANSFER_N;
				send(hClntSock, (char *)&send_code, sizeof(char), 0);

				break;
			}
			
			//계좌이체 성공
			send_code = RESULT_TRANSFER_Y;
			send(hClntSock, (char *)&send_code, sizeof(char), 0);

			//cout << Clnt.Account << "에서" << target_account << "으로"
			//	<< trans_amount << "원 계좌이체 성공" << endl;

			//잔액 변경
			Clnt.balance = Clnt.balance - trans_amount;

			//이체내역 기록
			system_record(3, RESULT_TRANSFER_Y, Clnt.Account, target_account);
			transfer_record(Clnt.Account, target_account, trans_amount, Clnt.balance);

			break;



		case QUIT_MESSAGE:
			//종료 메세지 수신시 소켓 반환하고 종료
			//cout << IP_Addrs << " " << Clnt.Account << " 연결 종료!!" << endl;
			system_record(2, QUIT_MESSAGE, Clnt.Account);

			Delete_ClntNode(ClntNode);	//클라이언트 노드 반환

			return 0;

			break;
		}
	}
	

	return 0;
}

void ErrorHandling(char *Message)
{
	cout << Message << endl;
}

//서버사용자 키보드 입력 '0'입력시 종료
DWORD WINAPI thread_input(void *null)
{
	ClntList *ListHead = List_Head;

	while(select_input != 0)	//0 입력시 종료
	{
		scanf("%d", &select_input);
		if (select_input == 1)
		{
			//접속자 노드 정렬
			sort_list();
			//접속자 보기
			User_info();
			select_input = 9;
		}
	}


	//소켓 해제를 통하여 메인함수 accept 탈출함.
	closesocket(Tmp_Sock.hClntSock);
	closesocket(hServSock);
	WSACleanup();

	return 0;
}

//접속자 표시 스레드
DWORD WINAPI thread_count(void *null)
{
	int cnt = 0;
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO Info;


	while (select_input != 0)
	{
		cnt = count_use();

		GetConsoleScreenBufferInfo(hCon, &Info);	//현재 커서 위치 저장

		gotoxy(100, Info.dwCursorPosition.Y);	//커서 이동
		cout << "접속자 수: " << cnt;

		gotoxy(Info.dwCursorPosition.X, Info.dwCursorPosition.Y);	//저장된 위치로 복귀

		Sleep(5000);	//5초마다 표시
	}

	return 0;
}
