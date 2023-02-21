
// GUIclientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "GUIclient.h"
#include "GUIclientDlg.h"
#include "afxdialogex.h"
#include "../include/protocol.h"	//协议头文件

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <memory>
using namespace std;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CGUIclientDlg 对话框

CGUIclientDlg::CGUIclientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GUICLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGUIclientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, LST_FRIEND, m_list);
}

BEGIN_MESSAGE_MAP(CGUIclientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(BTN_ONLINE, &CGUIclientDlg::OnBnClickedOnline)
	ON_BN_CLICKED(BTN_OFFLINE, &CGUIclientDlg::OnBnClickedOffline)
	ON_BN_CLICKED(BTN_PUB, &CGUIclientDlg::OnBnClickedPub)
	ON_BN_CLICKED(BTN_PRI, &CGUIclientDlg::OnBnClickedPri)
END_MESSAGE_MAP()


// CGUIclientDlg 消息处理程序

BOOL CGUIclientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	//初始化ws2_32网络库
	InitWs2_32();

	//连接服务器
	ConnetServer();

	//设置按钮
	SetButtonOFFline();

	//开启线程 接收服务器发送的包
	std::thread(&CGUIclientDlg::WorkThread, this).detach();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CGUIclientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CGUIclientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CGUIclientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

/**
 * @brief 初始化ws2_32库
*/
void CGUIclientDlg::InitWs2_32()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return;
	}

	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */

	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		WSACleanup();
		return;
	}

	/* The WinSock DLL is acceptable. Proceed. */
}

void CGUIclientDlg::ConnetServer()
{
	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sock == INVALID_SOCKET)
	{
		AfxMessageBox("socket 创建失败");
	}

	m_siServer.sin_family = AF_INET;
	m_siServer.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	m_siServer.sin_port = htons(5555);

	int nRet = connect(m_sock, (sockaddr*)&m_siServer, sizeof(m_siServer));
	if (nRet == SOCKET_ERROR)
	{
		AfxMessageBox("服务器连接失败");
	}
}

void CGUIclientDlg::SetButtonOnline()
{
	//发送消息按钮可用
	GetDlgItem(BTN_ONLINE)->EnableWindow(false);
	GetDlgItem(BTN_OFFLINE)->EnableWindow(true);
	GetDlgItem(BTN_PUB)->EnableWindow(true);
	GetDlgItem(BTN_PRI)->EnableWindow(true);
}

void CGUIclientDlg::SetButtonOFFline()
{
	//上线按钮可用
	GetDlgItem(BTN_ONLINE)->EnableWindow(true);
	GetDlgItem(BTN_OFFLINE)->EnableWindow(false);
	GetDlgItem(BTN_PUB)->EnableWindow(false);
	GetDlgItem(BTN_PRI)->EnableWindow(false);
}

void CGUIclientDlg::WorkThread()
{
	while (true)
	{
		//接收服务器包
		shared_ptr<ProtoHeader> ptrPkg = CTcp(m_sock).RecvPackage();
		m_mtxLst.lock();
		switch (ptrPkg->m_cmd)
		{
		case S2C_LOGIN://服务器发送的 新上线的客户端信息
		{
			S2CLogin* pPkg = (S2CLogin*)ptrPkg.get();

			//姓名作为key,显示在列表中
			int n = m_list.AddString(pPkg->m_szName);

			//ip&port 保存在堆上，将地址保存在data中
			auto pSi = new sockaddr_in(pPkg->m_siFrom);
			m_list.SetItemData(n, (DWORD_PTR)pSi);

		#if 0
	CString fmt;
			fmt.Format("即将新增好友：序号%d name:%s ip:%s port:%d\r\n", n,pPkg->m_szName,inet_ntoa(pPkg->m_siFrom.sin_addr),ntohs(pPkg->m_siFrom.sin_port));
			OutputDebugString(fmt);
		#endif
			break;
		}

		case S2C_LOGOUT:
		{
			S2CLogout* pPkg = (S2CLogout*)ptrPkg.get();
			#if 0
CString fmt;
			fmt.Format("准备删除的好友：ip:%s port:%d name:%s\r\n", inet_ntoa(pPkg->m_siFrom.sin_addr),ntohs(pPkg->m_siFrom.sin_port),pPkg->m_szName);
			OutputDebugString(fmt);
			#endif

			for (size_t i = 0; i < m_list.GetCount(); i++)
			{
				sockaddr_in* pSi = (sockaddr_in*)m_list.GetItemData(i);
				if (pPkg->m_siFrom == *pSi)
				{
					delete pSi;
					m_list.DeleteString(i);
					break;
				}
			}
			break;
		}

		case S2C_PUBCHAT:
		{
			S2CPubChat* pPkg = (S2CPubChat*)ptrPkg.get();

			CString fmt;
			fmt.Format("ip:%s port:%d name:%s 说: %s\r\n",
				inet_ntoa(pPkg->m_siFrom.sin_addr),
				ntohs(pPkg->m_siFrom.sin_port),
				pPkg->m_szName,
				pPkg->m_szMsg
			);

			m_szHistory += fmt;
			SetDlgItemText(EDT_HISTORY, m_szHistory);

			//实现聊天框自动滚动
			CEdit* pedit = (CEdit*)GetDlgItem(EDT_HISTORY);
			pedit->LineScroll(pedit->GetLineCount());

			break;
		}


		case S2C_PRICHAT:
		{
			S2CPriChat * pPkg = (S2CPriChat *)ptrPkg.get();

			CString fmt;
			fmt.Format("ip:%s port:%d name:%s 对ip:%s port:%d name:%s 私聊说: %s\r\n",
				inet_ntoa(pPkg->m_siFrom.sin_addr),
				ntohs(pPkg->m_siFrom.sin_port), 
				pPkg->m_szNameFrom,
				inet_ntoa(pPkg->m_siTo.sin_addr),
				ntohs(pPkg->m_siTo.sin_port), 
				pPkg->m_szNameTo,
				pPkg->m_szMsg
			);
			m_szHistory += fmt;
			SetDlgItemText(EDT_HISTORY, m_szHistory);

			//实现聊天框自动滚动
			CEdit* pedit = (CEdit*)GetDlgItem(EDT_HISTORY);
			pedit->LineScroll(pedit->GetLineCount());
			break;
		}

		}
		m_mtxLst.unlock();
	}
}
void CGUIclientDlg::OnBnClickedOnline()
{
	//获取EDT_NAME框中的姓名
	char szName[NAMELEN] = {};
	GetDlgItemText(EDT_NAME, szName, sizeof(szName));
	if (strlen(szName) == 0)
	{
		AfxMessageBox("昵称不能为空");
		return;
	}
	//构造登陆包
	shared_ptr<ProtoHeader> ptrOnlinePkg(new C2SLogin(C2S_LOGIN, szName));

	//发送给服务器
	int nRet = CTcp(m_sock).SendPackage(ptrOnlinePkg);

	//上线成功，上线按钮变灰，防止重复上线
	if (nRet != SOCKET_ERROR)
	{
		//上线变灰
		GetDlgItem(BTN_ONLINE)->EnableWindow(FALSE);

		//下线恢复
		GetDlgItem(BTN_OFFLINE)->EnableWindow(true);

		//发送消息变灰
		GetDlgItem(BTN_PUB)->EnableWindow(true);
		GetDlgItem(BTN_PRI)->EnableWindow(true);
	}

	
}
void CGUIclientDlg::OnBnClickedOffline()
{
	//获取EDT_NAME框中的姓名
	char szName[NAMELEN] = {};
	GetDlgItemText(EDT_NAME, szName, sizeof(szName));
	if (strlen(szName) == 0)
	{
		AfxMessageBox("昵称不能为空");
		return;
	}

	//构造下线包
	shared_ptr<ProtoHeader> ptrLogout(new C2SLogout(C2S_LOGOUT, szName));
 
	//发送给服务器
	int nRet = CTcp(m_sock).SendPackage(ptrLogout);

	//上线成功，上线按钮变灰，防止重复上线
	if (nRet != SOCKET_ERROR)
	{
		//上线按钮恢复
		GetDlgItem(BTN_ONLINE)->EnableWindow(true);

		//发送消息变灰
		GetDlgItem(BTN_PUB)->EnableWindow(false);
		GetDlgItem(BTN_PRI)->EnableWindow(false);

		//下线按钮变灰
		GetDlgItem(BTN_OFFLINE)->EnableWindow(false);
	}

	//删除列表，必须反着删除
	m_mtxLst.lock();
	for (int i = m_list.GetCount() - 1; i >= 0; i--)
	{
		//删除data
		delete (sockaddr_in*)m_list.GetItemData(i);

		//删除key
		m_list.DeleteString(i);
	}
	m_mtxLst.unlock();
}


void CGUIclientDlg::OnBnClickedPub()
{
	//获取EDT_NAME框中的姓名
	char szName[NAMELEN] = {};
	GetDlgItemText(EDT_NAME, szName, sizeof(szName));
	if (strlen(szName) == 0)
	{
		AfxMessageBox("昵称不能为空");
	}

	//获取EDT_CHAT框中的消息
	char szMsg[MSGLEN] = {};
	GetDlgItemText(EDT_CHAT, szMsg, sizeof(szMsg));

	//构造公聊包
	shared_ptr<ProtoHeader> ptrPkg(new C2SPubChat(C2S_PUBCHAT,szName,szMsg));
	int nRet = CTcp(m_sock).SendPackage(ptrPkg);

	//清空聊天框
	SetDlgItemText(EDT_CHAT, "");
}


void CGUIclientDlg::OnBnClickedPri()
{
	//获取EDT_NAME框中的姓名
	char szName[NAMELEN] = {};
	GetDlgItemText(EDT_NAME, szName, sizeof(szName));
	if (strlen(szName) == 0)
	{
		AfxMessageBox("昵称不能为空");
		return;
	}
	//获取EDT_CHAT框中的消息
	char szMsg[MSGLEN] = {};
	GetDlgItemText(EDT_CHAT, szMsg, sizeof(szMsg));

	//获取发送对象的sockaddr_in
	int n = m_list.GetCurSel();
	if (n == -1)
	{
		AfxMessageBox("未选择好友，无法发送私聊");
		return;
	}
	sockaddr_in siTo = *(sockaddr_in*)m_list.GetItemData(n);

	//构造私聊包
	shared_ptr<ProtoHeader> ptrPkg(new C2SPriChat(C2S_PRICHAT,szName,szMsg,siTo));
	CTcp(m_sock).SendPackage(ptrPkg);

	//清空聊天框
	SetDlgItemText(EDT_CHAT, "");
}


BOOL CGUIclientDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)  // 判断是否有按键按下
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:     // 回车键
			OnBnClickedPub();
			return TRUE;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

