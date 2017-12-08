// HanziToPin_UsingShowAPI.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include "SingleDownloader.h"


using namespace std;


int _tmain(int argc, _TCHAR* argv[])
{
	TDownNodeInfo* pDownInfo = new TDownNodeInfo;
	pDownInfo->sDownUrl = "http://www.baidu.com";
	pDownInfo->sSavePath = "D:\\test_libcurl\\";//MUST end up with "\\" or "/"
	pDownInfo->sFileName = "baidu.html";
	pDownInfo->sIdentifier = "1";
	pDownInfo->hNotifyWnd = NULL;//GetHWND();
	pDownInfo->bBreakptResume = false;
	pDownInfo->bOutToStr = false;
	pDownInfo->bPostData = false;
	//std::string sUsr = SystemConfig::GetDefUser();
	//std::string sPwd = SystemConfig::GetPassword();
	//pDownInfo->sPostData.Format("id=%s&pwd=%s", sUsr.c_str(),sPwd.c_str());
	CSingleDownloader* pDowner = new CSingleDownloader(pDownInfo);
	if(pDowner)
	{
		pDowner->StartDownload();
	}


	cout<<"OVER."<<endl;
	getchar();
	return 0;
}

