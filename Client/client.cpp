#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>



/* 윈도우 프로시저 선언 */
LRESULT CALLBACK WndProc_Login(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc_Main(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc_Join(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

//에러처리 팝업창
void ErrorHandling(TCHAR *Message);

//////////////////////
//////////////////////

#define MAX_STR 20				//문자열 버퍼 길이
#define SERVER_IP "127.0.0.1"	//서버 주소
//#define SERVER_IP "192.168.0.2"	//다중 사용 시험을 위한 실제 서버 주소
#define PORTNUM		8088		//포트번호 정의

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

/* 컨트롤 식별번호 */
#define ID_EDIT_LOGIN_ACCOUNT		100		//로그인>계좌입력
#define ID_EDIT_LOGIN_PASSWORD		101		//로그인>비밀번호
#define ID_BUTTON_LOGIN_CONNECT		1000	//로그인>접속버튼
#define ID_BUTTON_LOGIN_JOIN		1001	//로그인>회원가입버튼
#define ID_EDIT_MAIN_BALANCE		110		//메인>잔액
#define ID_EDIT_MAIN_RECORD			111		//메인>조회
#define ID_EDIT_MAIN_PASSWORD		112		//메인>비밀번호
#define ID_EDIT_MAIN_ACCOUNT		113		//메인>이체 계좌번호
#define ID_EDIT_MAIN_AMOUNT			114		//메인>이체 금액
#define ID_BUTTON_MAIN_TRANSFER		1100	//메인>이체버튼
#define ID_EDIT_JOIN_ACCOUNT		120		//회원가입>계좌번호
#define ID_EDIT_JOIN_PASSWORD		121		//회원가입>비밀번호
#define ID_BUTTON_JOIN_JOIN			1200	//회원가입>가입 버튼
#define ID_TABCONTROL_MAIN			0		//탭컨트롤
#define EDIT_STRING_LIMIT			15		//에디트 입력문자열 제한


//////////////////////////////////////////////
//////////////////////////////////////////////

/*	문자열 형변환 후 버퍼(유니코드->멀티바이트)	*/

//회원가입용 버퍼(멀티바이트)
typedef struct Join_Information
{
	char Password[MAX_STR];
	char Account[MAX_STR];
}Join_Information;


//계좌이체용 버퍼
typedef struct Transfer
{
	char Password[MAX_STR];
	char Account[MAX_STR];
	int Amount;
}Transfer;

//주요변수 모움(멀티바이트), 폼에서 쓰일 보관변수들 
typedef struct Information
{
	char Account[MAX_STR];			//계좌 번호
	char Password[MAX_STR];			//이체 비밀번호 비교용
	char Balance[MAX_STR];			//계좌 잔액
	char *Record;					//계좌조회 버퍼
}Information;

///////////////////////////////////////////////////
///////////////////////////////////////////////////

/* 소켓 관련 선언 */
WSADATA wsaData;		//라이브러리정보
SOCKET hClntSock;		//클라이언트 소켓
SOCKADDR_IN servAddr;	//서버 주소정보
int servAddrSize;		//주소 사이즈

/* 윈도우 클래스 이름 */
TCHAR MainClassName[] = TEXT("Main Form");		//메인폼
TCHAR LoginClassName[] = TEXT("Login Form");	//로그인폼
TCHAR JoinClassName[] = TEXT("Join Form");		//회원가입폼

TCHAR *Record = NULL;		//조회내역 수신 버퍼(수신 후 Information.Record로 변환)

/*	윈도우 핸들 선언 */
HINSTANCE g_hInst;
HWND hWndMain;		//메인폼 핸들
HWND hWndLogin;		//로그인폼 핸들
HWND hWndJoin;		//회원가입폼 핸들

/*	문자열 형변환 버퍼(유니코드)	*/
TCHAR Password[MAX_STR];		//비밀번호 형변환 버퍼 (char<->TCHAR)
TCHAR Account[MAX_STR];		//계좌번호 형변환 버퍼 (char<->TCHAR)
TCHAR Amount[MAX_STR];		//이체금액 형변환 버퍼 (char<->TCHAR)
TCHAR Balance[MAX_STR];			//계좌잔액 형변환 버퍼 (char<->TCHAR)

int int_balance;				//계좌잔액 형변환 버퍼 (int<->char)
char char_amount[MAX_STR];		//이체금액 형변환 버퍼 (int<->char)

int length;				//에디트박스 추출 문자열 길이 저장
char send_code;			//송신코드 저장
char recv_code;			//수신코드 저장
char messagebuf[1000];	//기타 메세지 수신 버퍼 ; 멀티바이트
TCHAR Tmessagebuf[1000];//기타 메세지 수신 버퍼 ; 유니코드

Information Clnt;

///////////////////////////////////////////////////
///////////////////////////////////////////////////

int WINAPI WinMain(	HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPSTR lpCmdLine,
					int nCmdShow)
{
	MSG msg;				//이벤트 메세지 보관

	WNDCLASS WC_Main;		//메인 윈도우 클래스
	WNDCLASS WC_Login;		//로그인 윈도우 클래스
	WNDCLASS WC_Join;		//회원가입 윈도우 클래스

	//////////////////////////////////////////////
	//////////////////////////////////////////////
	//////////////////////////////////////////////
	//라이브러리 등록
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling(TEXT("WSA 오류"));

	//소켓 생성
	hClntSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hClntSock == INVALID_SOCKET)
		ErrorHandling(TEXT("SOCKET 오류"));

	//주소정보 초기화
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(SERVER_IP);	//서버주소
	servAddr.sin_port = htons(PORTNUM);					//서버포트

	//소켓연결
	if (connect(hClntSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
		ErrorHandling(TEXT("connect 오류"));
	else
		MessageBox(NULL, TEXT("연결 되었습니다."), TEXT("연결!!"), MB_OK);
	//////////////////////////////////////////////
	//////////////////////////////////////////////
	//////////////////////////////////////////////
							
							
	/* 윈도우 구조체 */
	g_hInst = hInstance;
	
	//메인 윈도우 클래스
	WC_Main.hInstance = hInstance;
	WC_Main.lpszClassName = MainClassName;
	WC_Main.lpfnWndProc = WndProc_Main;
	WC_Main.style = CS_DBLCLKS;
	WC_Main.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WC_Main.hCursor = LoadCursor(NULL, IDC_ARROW);
	WC_Main.lpszMenuName = NULL;
	WC_Main.cbClsExtra = 0;
	WC_Main.cbWndExtra = 0;
	WC_Main.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

	/* 메인 클래스 등록. 실패 시 프로그램 종료 */
	if (!RegisterClass(&WC_Main))
		return 0;

	//로그인 윈도우 클래스
	WC_Login.hInstance = hInstance;
	WC_Login.lpszClassName = LoginClassName;
	WC_Login.lpfnWndProc = WndProc_Login;
	WC_Login.style = CS_DBLCLKS;
	WC_Login.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WC_Login.hCursor = LoadCursor(NULL, IDC_ARROW);
	WC_Login.lpszMenuName = NULL;
	WC_Login.cbClsExtra = 0;
	WC_Login.cbWndExtra = 0;
	WC_Login.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

	/* 로그인 클래스 등록. 실패 시 프로그램 종료 */
	if (!RegisterClass(&WC_Login))
		return 0;

	//회원가입 윈도우 클래스
	WC_Join.hInstance = hInstance;
	WC_Join.lpszClassName = JoinClassName;
	WC_Join.lpfnWndProc = WndProc_Join;
	WC_Join.style = CS_DBLCLKS;
	WC_Join.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WC_Join.hCursor = LoadCursor(NULL, IDC_ARROW);
	WC_Join.lpszMenuName = NULL;
	WC_Join.cbClsExtra = 0;
	WC_Join.cbWndExtra = 0;
	WC_Join.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

	/* 회원가입 클래스 등록. 실패 시 프로그램 종료 */
	if (!RegisterClass(&WC_Join))
		return 0;
	//////////////////////////////////////////////
	//////////////////////////////////////////////
	//////////////////////////////////////////////


	/* 첫화면 세팅(로그인 폼) */
	hWndLogin = CreateWindow(	LoginClassName,
								TEXT("로그인"),
								WS_POPUP | WS_BORDER | WS_CAPTION | WS_SYSMENU,
								600,
								300,
								350,
								150,
								NULL,
								(HMENU)0,
								g_hInst,
								NULL);
	ShowWindow(hWndLogin, SW_SHOW);		//로그인 폼 화면에 띄움

	/* 이벤트 메시지 루프 */
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);		//이벤트-메세지 분석
		DispatchMessage(&msg);		//메세지 전달
	}

	//종료 메세지 전달
	send_code = QUIT_MESSAGE;
	send(hClntSock, &send_code, sizeof(char), 0);

	//소켓 해제
	closesocket(hClntSock);
	WSACleanup();

	return 0;
}

HWND hLoginCtl_Edit_Account;		//계좌 입력 핸들
HWND hLoginCtl_Edit_Password;		//비밀번호 입력 핸들
HWND hLoginCtl_Button_Connect;		//접속 버튼 핸들
HWND hLoginCtl_Button_Join;			//회원가입 버튼 핸들

LRESULT CALLBACK WndProc_Login(	HWND hWnd,			//로그인 폼 코드
								UINT iMessage,
								WPARAM wParam,
								LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;

	switch (iMessage)
	{
	case WM_CREATE:

		/* 로그인폼 컨트롤 세팅 */

		hLoginCtl_Edit_Account = CreateWindow(TEXT("edit"), TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_BORDER, 10, 30,
			200, 20, hWnd, (HMENU)ID_EDIT_LOGIN_ACCOUNT,
			g_hInst, NULL);
		hLoginCtl_Edit_Password = CreateWindow(TEXT("edit"), TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 10, 80,
			200, 20, hWnd, (HMENU)ID_EDIT_LOGIN_PASSWORD,
			g_hInst, NULL);
		hLoginCtl_Button_Connect = CreateWindow(TEXT("button"), TEXT("접속"),
			WS_CHILD | WS_VISIBLE | WS_BORDER, 220, 10,
			100, 40, hWnd, (HMENU)ID_BUTTON_LOGIN_CONNECT,
			g_hInst, NULL);
		hLoginCtl_Button_Join = CreateWindow(TEXT("button"), TEXT("회원가입"),
			WS_CHILD | WS_VISIBLE | WS_BORDER, 220, 60,
			100, 40, hWnd, (HMENU)ID_BUTTON_LOGIN_JOIN,
			g_hInst, NULL);

		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		//로그인버튼 클릭시 행동
		case ID_BUTTON_LOGIN_CONNECT:

			//로그인 신호 송신
			send_code = REQUEST_LOGIN;
			send(hClntSock, &send_code, sizeof(char), 0);

			//계좌번호 추출 후 전송
			length = GetWindowText(hLoginCtl_Edit_Account, Account, 15);
			//유니코드->멀티바이트 변환
			//(코드페이지,오류처리,원본(유니),원본길이(-1은 자동),변환 후 담을 버퍼,버퍼크기,null,null)
			WideCharToMultiByte(CP_ACP, 0, Account, length, Clnt.Account, 20, NULL, NULL);

			Clnt.Account[length++] = NULL;	//length길이 널값 포함
			send(hClntSock, (char *)&length, sizeof(unsigned int), 0);
			send(hClntSock, Clnt.Account, length, 0);

			//비밀번호 추출 후 전송
			length = GetWindowText(hLoginCtl_Edit_Password, Password, 15);
			WideCharToMultiByte(CP_ACP, 0, Password, length, Clnt.Password, 20, NULL, NULL);	//length길이 널값 포함

			Clnt.Password[length++] = NULL;	//length길이 널값 포함
			send(hClntSock, (char *)&length, sizeof(unsigned int), 0);
			send(hClntSock, Clnt.Password, length, 0);


			//성공여부 수신
			recv(hClntSock, &recv_code, sizeof(char), 0);

			switch (recv_code)
			{
			case ACCEPT_LOGIN:	//로그인 성공
				/* 메인 폼 띄움 */
				hWndMain = CreateWindow(MainClassName,
					TEXT("계좌 이체"),
					WS_POPUP | WS_BORDER | WS_CAPTION | WS_SYSMENU,
					600,
					300,
					300,
					310,
					NULL,
					(HMENU)0,
					g_hInst,
					NULL);
				ShowWindow(hWndMain, SW_SHOW);		//메인 폼 화면에 띄움
				ShowWindow(hWndLogin, SW_HIDE);

				break;

			case REJECT_LOGIN:	//로그인 실패

				//실패 원인 수신
				recv(hClntSock, (char *)&length, sizeof(unsigned int), 0);
				recv(hClntSock, messagebuf, length, 0);

				//멀티바이트->유니코드 변환
				//(코드페이지,0,멀티바이트 문자열, 길이, 유니코드 버퍼, 버퍼 크기)
				MultiByteToWideChar(CP_ACP, 0, messagebuf, length, Tmessagebuf, 1000);
				MessageBox(NULL, Tmessagebuf, TEXT("로그인 실패."), MB_OK);

				break;
			}

			return 0;

			//회원가입 버튼 클릭
		case ID_BUTTON_LOGIN_JOIN:

			//회원가입 폼 생성
			hWndJoin = CreateWindow(JoinClassName,
				TEXT("회원가입"),
				WS_POPUP | WS_BORDER | WS_CAPTION | WS_SYSMENU,
				600,
				300,
				350,
				150,
				NULL,
				(HMENU)0,
				g_hInst,
				NULL);
			ShowWindow(hWndJoin, SW_SHOW);
			return 0;
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);	//그리기 정보 설정

		SetBkMode(hdc, TRANSPARENT);	//배경 투명하게하기
		TextOut(hdc, 10, 10, TEXT("계좌번호"),4);
		TextOut(hdc, 10, 60, TEXT("비밀번호"), 4); 

		EndPaint(hWnd, &ps);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	
	// WinProc에서 처리하지 않는 나머지 메시지를 윈도우 기본으로 처리하도록 전달
	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}

HWND hMainCtl_Tabcontrol_Menu;	//탭컨트롤 핸들
HWND hMainCtl_Edit_Balance;		//계좌 잔액 핸들
HWND hMainCtl_Edit_Record;		//계좌 조회 핸들
HWND hMainCtl_Edit_Password;	//이체 비밀번호 핸들
HWND hMainCtl_Edit_Account;		//이체 계좌번호 핸들
HWND hMainCtl_Edit_Amount;		//이체 금액 핸들
HWND hMainCtl_Button_Transfer;	//계좌이체 버튼 핸들
HWND hMainCtl_Static_Result;	//이체 결과 핸들
HWND hMainCtl_Static_Password;	//이체 금액 스태틱 핸들
HWND hMainCtl_Static_Account;	//비밀 번호 스태틱 핸들
HWND hMainCtl_Static_Amount;	//이체 금액 스태틱 핸들

LRESULT CALLBACK WndProc_Main(	HWND hWnd,			//메인 폼 코드
								UINT iMessage,
								WPARAM wParam,
								LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rt = { 0, 0, 400, 300 };
	TCITEM tie;
	Transfer trans;

	switch (iMessage)
	{
	case WM_CREATE:



		//잔액정보 요청 및 처리
		///////////////////////////////////////////////
		///////////////////////////////////////////////

		//잔액정보 요청
		send_code = REQUEST_BALANCE;
		send(hClntSock, &send_code, sizeof(char), 0);

		//잔액정보 수신
		recv(hClntSock, (char *) &(int_balance), sizeof(int), 0);

		itoa(int_balance, Clnt.Balance, 10);


		//멀티바이트->유니코드 변환
		//(코드페이지,0,멀티바이트 문자열, 길이, 유니코드 버퍼, 버퍼 크기)
		MultiByteToWideChar(CP_ACP, 0, Clnt.Balance, MAX_STR, Balance, MAX_STR);

		///////////////////////////////////////////////
		///////////////////////////////////////////////



		//계좌잔액표시
		hMainCtl_Edit_Balance = CreateWindow(TEXT("edit"), Balance,
								WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
								100, 10, 100, 20, hWnd, (HMENU)ID_EDIT_MAIN_BALANCE,
								g_hInst, NULL);

		//탭컨트롤 표시
		hMainCtl_Tabcontrol_Menu = CreateWindow(WC_TABCONTROL, NULL,
									WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS , 5, 30,
									275, 235, hWnd, (HMENU)ID_TABCONTROL_MAIN,
									g_hInst, NULL);

		//탭컨트롤 메뉴명 설정
		tie.mask = TCIF_TEXT;
		tie.pszText = TEXT("이체 내역");
		TabCtrl_InsertItem(hMainCtl_Tabcontrol_Menu, 0, &tie);
		tie.pszText = TEXT("계좌 이체");
		TabCtrl_InsertItem(hMainCtl_Tabcontrol_Menu, 1, &tie);
		
		/* 이체 내역탭 */
		//조회결과 생성
		hMainCtl_Edit_Record = CreateWindow(TEXT("edit"), Record,
			WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_AUTOHSCROLL | ES_MULTILINE | ES_READONLY,
			10, 30, 260, 195, hMainCtl_Tabcontrol_Menu, (HMENU)ID_EDIT_MAIN_RECORD,
			g_hInst, NULL);


		/* 계좌 이체탭 */
		//이체버튼 생성
		hMainCtl_Button_Transfer = CreateWindow(TEXT("button"), TEXT("이체"),
			WS_CHILD | WS_VISIBLE | WS_BORDER, 10, 210,
			255, 50, hWnd, (HMENU)ID_BUTTON_MAIN_TRANSFER,
			g_hInst, NULL);
		ShowWindow(hMainCtl_Button_Transfer, SW_HIDE);

		//이체 비밀번호 생성
		hMainCtl_Edit_Password = CreateWindow(TEXT("edit"), TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
			10, 55, 150, 20, hMainCtl_Tabcontrol_Menu, (HMENU)ID_EDIT_MAIN_PASSWORD,
			g_hInst, NULL);
		ShowWindow(hMainCtl_Edit_Password, SW_HIDE);

		//이체 계좌번호 생성
		hMainCtl_Edit_Account = CreateWindow(TEXT("edit"), TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_BORDER,
			10, 105, 150, 20, hMainCtl_Tabcontrol_Menu, (HMENU)ID_EDIT_MAIN_ACCOUNT,
			g_hInst, NULL);
		ShowWindow(hMainCtl_Edit_Account, SW_HIDE);

		//이체 금액 생성
		hMainCtl_Edit_Amount = CreateWindow(TEXT("edit"), TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_BORDER,
			10, 155, 150, 20, hMainCtl_Tabcontrol_Menu, (HMENU)ID_EDIT_MAIN_AMOUNT,
			g_hInst, NULL);
		ShowWindow(hMainCtl_Edit_Amount, SW_HIDE);

		//이체 결과 스태틱 생성
		hMainCtl_Static_Result = CreateWindow(TEXT("static"), TEXT("안녕하세요."),
			WS_CHILD | WS_VISIBLE | ES_CENTER,
			165, 105, 100, 20, hMainCtl_Tabcontrol_Menu, NULL,
			g_hInst, NULL);
		ShowWindow(hMainCtl_Static_Result, SW_HIDE);

		//비밀 번호 스태틱 생성
		hMainCtl_Static_Password = CreateWindow(TEXT("static"), TEXT("비밀 번호"),
			WS_CHILD | WS_VISIBLE,
			10, 35, 150, 20, hMainCtl_Tabcontrol_Menu, NULL,
			g_hInst, NULL);
		ShowWindow(hMainCtl_Static_Password, SW_HIDE);

		//계좌 번호 스태틱 생성
		hMainCtl_Static_Account = CreateWindow(TEXT("static"), TEXT("계좌 번호"),
			WS_CHILD | WS_VISIBLE,
			10, 85, 150, 20, hMainCtl_Tabcontrol_Menu, NULL,
			g_hInst, NULL);
		ShowWindow(hMainCtl_Static_Account, SW_HIDE);

		//이체 금액 스태틱 생성
		hMainCtl_Static_Amount = CreateWindow(TEXT("static"), TEXT("이체 금액"),
			WS_CHILD | WS_VISIBLE,
			10, 135, 150, 20, hMainCtl_Tabcontrol_Menu, NULL,
			g_hInst, NULL);
		ShowWindow(hMainCtl_Static_Amount, SW_HIDE);

		//이체 금액 생성
		hMainCtl_Edit_Amount = CreateWindow(TEXT("edit"), TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_BORDER,
			10, 155, 150, 20, hMainCtl_Tabcontrol_Menu, (HMENU)ID_EDIT_MAIN_AMOUNT,
			g_hInst, NULL);
		ShowWindow(hMainCtl_Edit_Amount, SW_HIDE);

		return 0;

	case WM_NOTIFY:		//탭변경이 일어났을경우// notify(통보)

		//잔액정보 요청 및 처리
		///////////////////////////////////////////////
		///////////////////////////////////////////////
		
		//잔액정보 요청
		send_code = REQUEST_BALANCE;
		send(hClntSock, &send_code, sizeof(char), 0);

		//잔액정보 수신
		recv(hClntSock, (char *) &int_balance, sizeof(int), 0);

		itoa(int_balance, Clnt.Balance, 10);


		//멀티바이트->유니코드 변환
		//(코드페이지,0,멀티바이트 문자열, 길이, 유니코드 버퍼, 버퍼 크기)
		MultiByteToWideChar(CP_ACP, 0, Clnt.Balance, MAX_STR, Balance, MAX_STR);
		SetWindowText(hMainCtl_Edit_Balance, Balance);
		///////////////////////////////////////////////
		///////////////////////////////////////////////
		
		switch (((LPNMHDR)lParam)->code)
		{
		case TCN_SELCHANGE:	//탭변경이 일어났을경우
			int Tab_Select = TabCtrl_GetCurSel(hMainCtl_Tabcontrol_Menu);
			switch (Tab_Select)
			{
			case 0:			//0번탭 선택(이체 내역 탭)

				//이체 내역 요청 및 처리
				///////////////////////////////////////////////
				///////////////////////////////////////////////

							//잔액정보 요청
				send_code = REQUEST_ACCOUNT;
				send(hClntSock, &send_code, sizeof(char), 0);

				//잔액정보 수신
				recv(hClntSock, (char *)&recv_code, sizeof(char), 0);
				switch (recv_code)
				{
				case RESULT_ACCOUNT_N:
					break;
				case RESULT_ACCOUNT_Y:

					recv(hClntSock, (char *)&length, sizeof(unsigned int), 0);
					Clnt.Record = new char[length];	//길이만큼 메모리 할당
					Record = new TCHAR[length];

					recv(hClntSock, Clnt.Record, length, 0);

					//멀티바이트->유니코드 변환
					//(코드페이지,0,멀티바이트 문자열, 길이, 유니코드 버퍼, 버퍼 크기)
					MultiByteToWideChar(CP_ACP, 0, Clnt.Record, length, Record, length);


					SetWindowText(hMainCtl_Edit_Record, Record);	//내역 에디트에 표시

					delete Clnt.Record;
					delete Record;

					break;
				}

				///////////////////////////////////////////////
				///////////////////////////////////////////////


				/* 1번탭 내용 숨기기 */
				ShowWindow(hMainCtl_Edit_Record, SW_SHOW);			//조회 결과 표시

				ShowWindow(hMainCtl_Button_Transfer, SW_HIDE);		//이체 버튼 숨기기
				ShowWindow(hMainCtl_Edit_Password, SW_HIDE);		//패스 워드 숨기기
				ShowWindow(hMainCtl_Edit_Account, SW_HIDE);			//계좌 번호 숨기기
				ShowWindow(hMainCtl_Edit_Amount, SW_HIDE);			//이체 금액 숨기기
				ShowWindow(hMainCtl_Static_Result, SW_HIDE);		//이체 결과 스태틱 숨기기
				ShowWindow(hMainCtl_Static_Password, SW_HIDE);		//비밀 번호 스태틱 숨기기
				ShowWindow(hMainCtl_Static_Account, SW_HIDE);		//계좌 번호 스태틱 숨기기
				ShowWindow(hMainCtl_Static_Amount, SW_HIDE);		//이체 금액 스태틱 숨기기

				

				return 0;
			case 1:			//1번탭 선택(계좌 이체 탭)

				ShowWindow(hMainCtl_Edit_Record, SW_HIDE);			//조회 결과 표시

				ShowWindow(hMainCtl_Button_Transfer, SW_SHOW);		//이체 버튼 표시
				ShowWindow(hMainCtl_Edit_Password, SW_SHOW);		//패스 워드 표시
				ShowWindow(hMainCtl_Edit_Account, SW_SHOW);			//계좌 번호 표시
				ShowWindow(hMainCtl_Edit_Amount, SW_SHOW);			//이체 금액 표시
				ShowWindow(hMainCtl_Static_Result, SW_SHOW);		//이체 결과 스태틱 표시
				ShowWindow(hMainCtl_Static_Password, SW_SHOW);		//비밀 번호 스태틱 표시
				ShowWindow(hMainCtl_Static_Account, SW_SHOW);		//계좌 번호 스태틱 표시
				ShowWindow(hMainCtl_Static_Amount, SW_SHOW);		//이체 금액 스태틱 표시

				return 0;
			}
			return 0;
		}




	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case ID_BUTTON_MAIN_TRANSFER:	//이체 버튼 클릭시
			
			//비밀번호 추출 후 전송
			length = GetWindowText(hMainCtl_Edit_Password, Password, 15);
			//유니코드->멀티바이트 변환
			//(코드페이지,오류처리,원본(유니),원본길이(-1은 자동),변환 후 담을 버퍼,버퍼크기,null,null)
			WideCharToMultiByte(CP_ACP, 0, Password, length, trans.Password, 20, NULL, NULL);
			trans.Password[length++] = NULL;	//length길이 널값 포함


			//비밀번호 비교
			if (strcmp(Clnt.Password, trans.Password) != 0)
			{
				MessageBox(NULL, TEXT("비밀번호가 틀렸습니다."), TEXT("계좌이체"), MB_OK);
				return 0;
			}
			

			//이체대상 계좌번호 추출
			length = GetWindowText(hMainCtl_Edit_Account, Account, 15);
			//유니코드->멀티바이트 변환
			//(코드페이지,오류처리,원본(유니),원본길이(-1은 자동),변환 후 담을 버퍼,버퍼크기,null,null)
			WideCharToMultiByte(CP_ACP, 0, Account, length, trans.Account, 20, NULL, NULL);

			trans.Account[length++] = NULL;	//length길이 널값 포함

			//이체 금액 추출
			length = GetWindowText(hMainCtl_Edit_Amount, Amount, 15);
			WideCharToMultiByte(CP_ACP, 0, Amount, length, char_amount, 20, NULL, NULL);	//length길이 널값 포함

			char_amount[length++] = NULL;	//length길이 널값 포함
			trans.Amount = atoi(char_amount);

			//자신에게 이체 요청했을경우
			if (strcmp(Clnt.Account, trans.Account) == 0)
			{
				MessageBox(NULL, TEXT("자신에게 이체 못합니다!!!!"), TEXT("계좌이체"), MB_OK);
				return 0;
			}

			//잔액보다 많은 금액 이체할경우
			if (atoi(Clnt.Balance) < atoi(char_amount))
			{
				MessageBox(NULL, TEXT("이체 금액이 잔액보다 많습니다!!!!"), TEXT("계좌이체"), MB_OK);
				return 0;
			}

			//계좌이체 요청 신호 송신
			send_code = REQUEST_TRANSFER;
			send(hClntSock, &send_code, sizeof(char), 0);

			//계좌번호 송신
			send(hClntSock, (char *)&length, sizeof(unsigned int), 0);
			send(hClntSock, trans.Account, length, 0);

			//이체금액 송신
			send(hClntSock, (char*) &trans.Amount, sizeof(int), 0);

			///계좌이체 결과 수신
			recv_code = 0;	//초기화
			recv(hClntSock, (char*) &recv_code, sizeof(char), 0);
			switch (recv_code)
			{
			case RESULT_TRANSFER_Y:
				MessageBox(NULL, TEXT("계좌이체가 완료되었습니다."), TEXT("계좌이체"), MB_OK);
				
				//잔액 변경
				int_balance = int_balance - trans.Amount;
				itoa(int_balance, Clnt.Balance, 10);

				//멀티바이트->유니코드 변환
				//(코드페이지,0,멀티바이트 문자열, 길이, 유니코드 버퍼, 버퍼 크기)
				MultiByteToWideChar(CP_ACP, 0, Clnt.Balance, MAX_STR, Balance, MAX_STR);
				SetWindowText(hMainCtl_Edit_Balance, Balance);

				break;

			case RESULT_TRANSFER_N:
				MessageBox(NULL, TEXT("계좌이체가 실패하였습니다."), TEXT("계좌이체"), MB_OK);

				break;
			}
			break;
		}


		return 0;




	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);	//그리기 정보 설정

		SetBkMode(hdc, TRANSPARENT);	//배경 투명하게하기
		TextOut(hdc, 10, 10, TEXT("계좌 잔액"), 5);

		EndPaint(hWnd, &ps);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}


HWND hJoinCtl_Static_Account;	//계좌번호 스태틱 핸들
HWND hJoinCtl_Edit_Account;		//계좌번호 에디트 핸들
HWND hJoinCtl_Static_Password;	//비밀번호 스태틱 핸들
HWND hJoinCtl_Edit_Password;	//비밀번호 에디트 핸들
HWND hJoinCtl_Button_Join;		//회원가입 버튼 핸들

LRESULT CALLBACK WndProc_Join(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
	case WM_CREATE:

		//////////////////////////
		//계좌번호 스태틱 생성
		hJoinCtl_Static_Account = CreateWindow(TEXT("static"), TEXT("계좌번호"),
			WS_CHILD | WS_VISIBLE,
			10, 10, 70, 20, hWnd, NULL,
			g_hInst, NULL);
		ShowWindow(hJoinCtl_Static_Account, SW_SHOW);

		//계좌번호 생성
		hJoinCtl_Edit_Account = CreateWindow(TEXT("edit"), TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_BORDER,
			10, 30, 200, 20, hWnd, (HMENU)ID_EDIT_MAIN_AMOUNT,
			g_hInst, NULL);
		ShowWindow(hJoinCtl_Edit_Account, SW_SHOW);
		SendMessage(hJoinCtl_Edit_Account, EM_LIMITTEXT, (WPARAM)EDIT_STRING_LIMIT, 0);	//에디트 입력수 제한

		//비밀번호 스태틱 생성
		hJoinCtl_Static_Password = CreateWindow(TEXT("static"), TEXT("비밀번호"),
			WS_CHILD | WS_VISIBLE,
			10, 60, 70, 20, hWnd, NULL,
			g_hInst, NULL);
		ShowWindow(hJoinCtl_Static_Password, SW_SHOW);

		//비밀번호 생성
		hJoinCtl_Edit_Password = CreateWindow(TEXT("edit"), TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
			10, 80, 200, 20, hWnd, (HMENU)ID_EDIT_MAIN_AMOUNT,
			g_hInst, NULL);
		ShowWindow(hJoinCtl_Edit_Password, SW_SHOW);
		SendMessage(hJoinCtl_Edit_Password, EM_LIMITTEXT, (WPARAM)EDIT_STRING_LIMIT, 0);	//에디트 입력수 제한

		//가입버튼 생성
		hJoinCtl_Button_Join = CreateWindow(TEXT("button"), TEXT("가입하기"),
			WS_CHILD | WS_VISIBLE | WS_BORDER,
			220, 10, 100, 90, hWnd, (HMENU)ID_BUTTON_JOIN_JOIN,
			g_hInst, NULL);
		ShowWindow(hJoinCtl_Button_Join, SW_SHOW);

		return 0;



	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			//가입버튼 클릭시 행동
		case ID_BUTTON_JOIN_JOIN:
			Join_Information a;

			//회원가입 요청 코드 전송
			send_code = REQUEST_JOIN;
			send(hClntSock, &send_code, sizeof(char), 0);


			//계좌번호 추출 후 전송
			length = GetWindowText(hJoinCtl_Edit_Account, Account, 15);
			//유니코드->멀티바이트 변환
			//(코드페이지,오류처리,원본(유니),원본길이(-1은 자동),변환 후 담을 버퍼,버퍼크기,null,null)
			WideCharToMultiByte(CP_ACP, 0, Account, length, a.Account, 20, NULL, NULL);

			a.Account[length++] = NULL;	//length길이 널값 포함
			send(hClntSock, (char *)&length, sizeof(unsigned int), 0);
			send(hClntSock, a.Account, length, 0);

			//비밀번호 추출 후 전송
			length = GetWindowText(hJoinCtl_Edit_Password, Password, 15);
			WideCharToMultiByte(CP_ACP, 0, Password, length, a.Password, 20, NULL, NULL);	//length길이 널값 포함
			
			a.Password[length++] = NULL;	//length길이 널값 포함
			send(hClntSock, (char *)&length, sizeof(unsigned int), 0);
			send(hClntSock, a.Password, length, 0);
			

			//성공여부 수신
			recv(hClntSock, &recv_code, sizeof(char), 0);

			//회원가입 성공 여부
			if (recv_code == ACCEPT_JOIN)
			{
				DestroyWindow(hWnd);
				MessageBox(NULL, TEXT("계좌를 생성하였습니다."), TEXT("회원가입"), MB_OK);
			}
			else
				MessageBox(NULL, TEXT("계좌가 존재합니다."), TEXT("회원가입"), MB_OK);


/*	삭제해도됨 참고용

			//멀티바이트->유니코드 변환
			//(코드페이지,0,멀티바이트 문자열, 길이, 유니코드 버퍼, 버퍼 크기)
			MultiByteToWideChar(CP_ACP, 0, &a.flag, 1, Account, 10);

			SetWindowText(hJoinCtl_Edit_Account, Account);

*/
			return 0;
		}
		return 0;

	case WM_DESTROY:

		return 0;
	}

	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}


void ErrorHandling(TCHAR *Message)
{
	MessageBox(NULL, Message, TEXT("오류!!"), MB_OK);
	exit(1);
}
