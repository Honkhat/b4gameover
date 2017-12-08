#include "StdAfx.h"
#include "SingleDownloader.h"
#include <io.h>
#include <direct.h>
#include <process.h>
#include <curl/curl.h>
#include <cmath>

#define  SAFE_DEL_POINER(p) if(p) {delete p; p = NULL;}
#define  SUFFIX_FILENAMEAPPEND "_tag" //"TylorSwift-Fifteen.flv"  --> "TylorSwift-Fifteen_tag18.flv"
#define  MAX_NAME_LEN  50

bool CreateMultiLevelDir(const char *strFilePathName)
{
	char strFilePath[MAX_PATH] = {0};
	char *s, *p;

	strcpy(strFilePath, strFilePathName);

	s = strFilePath;

	/*
	File Protocol Format: file:///sDrives[|sFile] 
	e.g:
	(1)file:///c|/AlexTestDir/test.txt
	(2)file:///c:/AlexTestDir/test.txt
	(3)c:\AlexTestDir\test.txt (Right Slash!!)
	(4)\AlexTestDir\

	e2dk format: ed2k://|file|SOE-235.avi|1277141504|B802B722521F991F758325BC302EC8C4|/

	_mkdir()不支持FILE e2dk这些协议的
	如果已经存在_mkdir()也返回-1;只要失败就-1吧
	*/

	//handle situation "\\Dir1\\"(the root path of belonged drive)
// 	if(0 == strncmp(s, "\\", 1))
// 	{
// 		s += 1;
// 		s = strchr(s, '\\');
// 		if(!s)
// 		{
// 			return FALSE;
// 		}
// 		else
// 		{
// 			s += 1;
// 		}
// 	}

	do
	{
		p = strchr(s, '\\');
		if(p)
		{
			*p = '\0';
		}
		s = strFilePath;

		if(strlen(s) == 0)
		{
			s = p + 1;
			continue;
		}


		// directory doesn't exist
		if(-1 == _access(s, 0))
		{
			// failed to create directory.
			if(-1 == _mkdir(s)) 
			{
				return (FALSE);
			}
		}

		if(p)
		{
			*p = '\\';
			s = p + 1;
		}
	} while(p);

	//last check
	return (_access(strFilePath, 0) == 0);
}


CStringA EncodeURI(CStringA url) //JS: encodeURI()
{
	CStringA sEncoded;
	//Former Valid Characters Set: a-z,A-Z,0-9,* @ - _ + . /
	
	//encodeURI不编码字符有82个：!，#，$，&，'，  (，)，*，+，,，   -，.，/，:，;，  =，?，@，_，~，0-9，a-z，A-Z
	for(int i = 0; i < url.GetLength(); ++i)
	{
		if( (url[i] >= 'A' && url[i] <= 'Z') || (url[i] >= 'a' && url[i] <= 'z') || (url[i] >= '0' && url[i] <= '9') || 
			(url[i] == '!') || (url[i] == '#') || (url[i] == '$') || (url[i] == '&') || (url[i] == '\'') 
		|| (url[i] == '(') || (url[i] == ')') || (url[i] == '*') || (url[i] == '+') || (url[i] == ',')
		|| (url[i] == '-')  || (url[i] == '.') || (url[i] == '/') || (url[i] == ':') || (url[i] == ';')
		|| (url[i] == '=')  || (url[i] == '?') || (url[i] == '@') || (url[i] == '_') || (url[i] == '~'))
			sEncoded += url[i];
		else if(url[i] == '%') //不做二次编码
		{
			i += 3;
		}
		else
		{
			char szEncoded[5] = {0};
			sprintf_s(szEncoded, 5, "%%%02x", url[i]); //'%02x'
			sEncoded += szEncoded;
		}
	}

	return sEncoded;
}

CSingleDownloader::CSingleDownloader(void):m_bDownPaused(false),m_bSetUnpause(false),m_bStopped(false),m_eh(NULL),m_pNodeInfo(NULL),m_pMsgParam(NULL)
{
	
}

CSingleDownloader::CSingleDownloader(TDownNodeInfo* pDownNode)
	:m_bDownPaused(false),m_bStopped(false),m_bSetUnpause(false),m_bInfoExtractSuc(false), m_eh(NULL),m_pNodeInfo(nullptr),m_pMsgParam(nullptr)
{
	if(!pDownNode)
		return;

	m_pNodeInfo = pDownNode;
	m_pMsgParam = new TMsgParam;
	m_pMsgParam->sIdentifier = pDownNode->sIdentifier;
}


CSingleDownloader::~CSingleDownloader(void)
{
	DoFinalClean();
	SAFE_DEL_POINER(m_pNodeInfo);
	SAFE_DEL_POINER(m_pMsgParam);
}

CSingleDownloader::CSingleDownloader(char* url, char* savepath, char* idstr, HWND notifywnd, char* szNameAppend,bool bBreakptResume, bool bHttpGet)
	:m_bDownPaused(false),m_bStopped(false),m_bSetUnpause(false),m_bInfoExtractSuc(false), m_eh(NULL)
{
	if(!url)
		return;

	m_pNodeInfo = new TDownNodeInfo;
	m_pNodeInfo->sDownUrl = url;
	m_pNodeInfo->sSavePath = savepath;
	m_pNodeInfo->sIdentifier = idstr;
	m_pNodeInfo->hNotifyWnd = notifywnd;	
	m_pNodeInfo->bBreakptResume = bBreakptResume;
	m_pNodeInfo->bOutToStr = bHttpGet;
	m_pNodeInfo->sFileNameAppend = szNameAppend;

	m_pMsgParam = new TMsgParam;
	m_pMsgParam->sIdentifier = idstr;
}


CString CSingleDownloader::UnifyByteStr_Speed(double dBytePerSec)
{
	CString cSpeed;
	if(dBytePerSec - 0.0f < 0.0f)
	{
		cSpeed = "0.00 kb/s";
		return cSpeed;
	}

	int expo = (int)(log10(dBytePerSec) / log10(1000.0));

	switch(expo)
	{
	case 0:
		{
			cSpeed.Empty();
			cSpeed.Format(_T("%.2f b/s"), dBytePerSec);
		}
		break;

	case 1:
		{
			cSpeed.Empty();
			cSpeed.Format(_T("%.2f Kb/s"), dBytePerSec/1000.0);
		}
		break;

	case 2:
		{
			cSpeed.Empty();
			cSpeed.Format(_T("%.2f Mb/s"), dBytePerSec/1000000.0);
		}
		break;

	case 3:
		{
			cSpeed.Empty();
			cSpeed.Format(_T("%.2f Gb/s"), dBytePerSec/1000000.0/1000.0);
		}
		break;


	default:
		break;
	}

	return cSpeed;
}

bool CSingleDownloader::ExtractFileInfo()
{
	_beginthreadex(NULL, 0, ExtractThreadFunc, this, 0, NULL);

	return true; //不可靠
}

unsigned __stdcall CSingleDownloader::ExtractThreadFunc(void* param)
{
	if(!param)
		return 1;

	CSingleDownloader* pThis = (CSingleDownloader*)param;
	pThis->_Extract();

	return 0;
}

void CSingleDownloader::_Extract()
{
	m_bInfoExtractSuc = false;
	m_bStopped = false;

	m_eh = curl_easy_init(); //curl easy pointer
	if(!m_eh)
	{
		Notify(WM_STATUS_GOTFILEINFO, 0);
		return;
	}

	curl_easy_setopt(m_eh, CURLOPT_URL, m_pNodeInfo->sDownUrl.GetString());
	/* if url is redirected, so we tell libcurl to follow redirection */
	curl_easy_setopt(m_eh, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(m_eh, CURLOPT_HEADER, 0L);//don't include header-info into body
	curl_easy_setopt(m_eh, CURLOPT_HEADERDATA, this);
	curl_easy_setopt(m_eh, CURLOPT_HEADERFUNCTION,HeaderCallBack);
	curl_easy_setopt(m_eh, CURLOPT_NOBODY, 1); //只是想获取FileInfo的话就不需要body
	//curl_easy_setopt(m_eh, CURLOPT_WRITEFUNCTION, WriteFunc);
	//curl_easy_setopt(m_eh, CURLOPT_WRITEDATA, this);  //file pointer usually
	curl_easy_setopt(m_eh, CURLOPT_NOPROGRESS, 1L);
	//curl_easy_setopt(m_eh, CURLOPT_PROGRESSFUNCTION, ProgressFunc);
	//curl_easy_setopt(m_eh, CURLOPT_PROGRESSDATA, this);
	if(m_bStopped)
	{
		Notify(WM_STATUS_GOTFILEINFO, 0);
		return;
	}

	curl_easy_perform(m_eh);

	curl_easy_getinfo(m_eh, CURLINFO_RESPONSE_CODE, &m_pNodeInfo->lHttpStatus);
	curl_easy_getinfo(m_eh, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &m_pNodeInfo->dFileSize);

	//4xx: client error; 5xx: server error
	if((m_pNodeInfo->lHttpStatus >= 400 && m_pNodeInfo->lHttpStatus <= 600) || m_pNodeInfo->dFileSize < 1.0)
	{
		Notify(WM_STATUS_GOTFILEINFO, 0);
		return;
	}

	if(!m_pNodeInfo->bFindContentDisp)
		ExtractFileNameFromUrl();

	InsertFileNameAppend();

	m_pMsgParam->sNameWithPath.Format("%s%s", m_pNodeInfo->sSavePath.GetString(), m_pNodeInfo->sFileName);

	m_bInfoExtractSuc = true;

	Notify(WM_STATUS_GOTFILEINFO, 1);
}

void CSingleDownloader::ExtractFileNameFromUrl()
{
	// 从后往前url被选择截取的优先级: = & ? /
	char szCut[] = {'=', '&', '?', '/'};
	int iLenCut = sizeof(szCut) / sizeof(char);

	int* piPos = new int[iLenCut];
	int iValueMax = m_pNodeInfo->sDownUrl.ReverseFind(szCut[0]);
	int iValueMaxPos = 0;
	piPos[0] = iValueMax;
	for(int i = 1; i < iLenCut; ++i)
	{
		piPos[i] = m_pNodeInfo->sDownUrl.ReverseFind(szCut[i]);
		if(piPos[i] > iValueMax)
			iValueMaxPos = i;
	}
	
	int iPosEnd = m_pNodeInfo->sDownUrl.GetLength() - 1;
	int iPosBeg = piPos[iValueMaxPos];
	if(iPosBeg != -1 && iPosBeg < iPosEnd)
	{
		if(iPosEnd - iPosBeg < MAX_NAME_LEN)
			m_pNodeInfo->sFileName = m_pNodeInfo->sDownUrl.Mid(iPosBeg+1, iPosEnd - iPosBeg);
		else
			m_pNodeInfo->sFileName = m_pNodeInfo->sDownUrl.Right(MAX_NAME_LEN);
	}


	//如果非常罕见的仍然失败,直接以时间戳命名
	if(m_pNodeInfo->sFileName.GetLength() == 0)
	{
		SYSTEMTIME SysTime;
		GetSystemTime(&SysTime);
		m_pNodeInfo->sFileName.Format("%04d%02d%02d_%02d%02d%02d", SysTime.wYear, SysTime.wMonth, SysTime.wDay,
			SysTime.wHour, SysTime.wMinute, SysTime.wSecond);
	}

	SAFE_DEL_POINER(piPos);
}

size_t CSingleDownloader::WriteFunc(char *str, size_t size, size_t nmemb, void *stream)
{
	size_t tRetSize = size * nmemb; 
	CSingleDownloader* pSDer = (CSingleDownloader*)stream;
	if(!pSDer || !pSDer->m_pNodeInfo || pSDer->m_bStopped)
		return tRetSize;

	TDownNodeInfo* pDownNode = pSDer->m_pNodeInfo;

	if(!pDownNode->bOutToStr)
	{
		if(pDownNode->pFILE)
			tRetSize = fwrite(str, size, nmemb, (FILE*)pDownNode->pFILE);
	}
	else
	{
		pDownNode->sDownData += str;
	}

	if(pSDer->m_bDownPaused)
		return CURL_WRITEFUNC_PAUSE; //curl_easy_pause()

	if(pSDer->m_bStopped)
		return -1; //will cause curl to stop 

	return tRetSize;
}

size_t CSingleDownloader::ProgressFunc(void* client,curl_off_t dltotal,curl_off_t dlnow,curl_off_t ultotal, curl_off_t ulnow) 
{
	if(dltotal - 0 < 0) return 0;

	CSingleDownloader* pSDer = (CSingleDownloader*)client;
	if(!pSDer || !pSDer->m_pNodeInfo ||pSDer->m_bStopped)
		return 1;

	TDownNodeInfo* pNode = pSDer->m_pNodeInfo;
	double dAver = 0.0;
	curl_easy_getinfo(pSDer->m_eh, CURLINFO_SPEED_DOWNLOAD, &dAver);
	pSDer->m_pNodeInfo->dAverSpeed = dAver;
	int iPercent = 0;
	if(dltotal > 0.4)
		iPercent = (dlnow * 100.0f / dltotal);
	pSDer->m_pMsgParam->dAverSpeed = dAver;
	pSDer->m_pMsgParam->iPercent = iPercent;
	if(GetTickCount() % 10 == 0)
		pSDer->Notify(WM_SDSTATUS_DOWNING, 0);

	
	if(pSDer->m_bSetUnpause)
	{
		pSDer->m_bSetUnpause = false;
		curl_easy_pause(pSDer->m_eh, CURLPAUSE_CONT);
	}
	

	return 0;
}

size_t CSingleDownloader::HeaderCallBack(char *buffer,size_t size,size_t nmemb,void *userdata)
{
	CSingleDownloader* pSDer = (CSingleDownloader*)userdata;
	if(!pSDer || !pSDer->m_pNodeInfo)
		return size * nmemb;

	TDownNodeInfo* node = pSDer->m_pNodeInfo;

 	CStringA cHeader = buffer;  
// 	int nPos = cHeader.Find("HTTP/"); //curl提供了:curl_easy_getinfo(eh,CURLINFO_RESPONSE_CODE,..)
// 	if(nPos != -1) //parse the the http status (e.g. HTTP/1.1 400 Bad Request)
// 	{
// 		int nFirstSpace = cHeader.Find(' ',nPos+4);
// 		int nSecondSpace = -1;
// 		int nHttpStatus = -1;
// 		if(nFirstSpace != -1)
// 			nSecondSpace = cHeader.Find(' ',nFirstSpace+1);  //atoi(' 400 ')?!!test
// 		if(nFirstSpace != -1 && nSecondSpace != -1)
// 			nHttpStatus = atoi(cHeader.Mid(nFirstSpace+1,nSecondSpace-nFirstSpace-1).GetString());
// 
// 		//4xx: Client Error; 5xx: Server Error
// 		node->lHttpStatus = nHttpStatus;	
// 
// 	}
	
	if(!node->bFindContentDisp) ////try to extract the file name from the "Content-Disposition" field
	{
		int nPos1 = cHeader.Find("Content-Disposition");
		if(nPos1 != -1)
		{
			int nPos2 = cHeader.Find("filename", nPos1+strlen("Content-Disposition"));
			if(nPos2 != -1)
			{
				int nPosBeg = -1, nPosEnd = -1;
				nPosBeg = cHeader.Find('"', nPos2);
				if(nPosBeg != -1)
				{
					nPosEnd = cHeader.Find('"', nPosBeg+1);
					if(nPosEnd != -1 && nPosBeg < nPosEnd)
					{ 
						CStringA cFileName = cHeader.Mid(nPosBeg+1, nPosEnd-nPosBeg-1);
						node->sFileName = cFileName;
						node->bFindContentDisp = true;
					}
				}
			}

		}
	}

	//try to extract file length
	//int nPosLen = cHeader.Find("Content-Length"); //curl_easy_getinfo(m_eh, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &m_pNodeInfo->dFileSize);
	
	return size * nmemb;
}

void CSingleDownloader::InsertFileNameAppend()
{
	if(!m_pNodeInfo || m_pNodeInfo->sFileNameAppend.GetLength() == 0)
		return;

	//e.g.  FileName:"EntityData.mp3"  sFileNameAppend:"128bkps"==> "EntityData_128bkps.mp3"
	int iPosInsert = m_pNodeInfo->sFileName.ReverseFind('.'); //insert()参数位置前插
	bool bWithExtName = true;
	if(iPosInsert == -1)
	{
		iPosInsert = m_pNodeInfo->sFileName.GetLength();
		bWithExtName = false;
	}

	CStringA sAppend = SUFFIX_FILENAMEAPPEND;
	sAppend += m_pNodeInfo->sFileNameAppend;
	if(!bWithExtName)
		m_pNodeInfo->sFileName.Insert(iPosInsert, sAppend.GetString());
	else
	{    //去掉sFileNameAppend中的ExtName
		CStringA sAppend2 = m_pNodeInfo->sFileNameAppend;
		int iPosDot = sAppend2.ReverseFind('.');
		if(iPosDot != -1)
		{
			sAppend2 = sAppend2.Left(iPosDot);
		}
		
		m_pNodeInfo->sFileName.Insert(iPosInsert, sAppend2.GetString());
	}
}

//在调用StartDownload()之前先要调用ExtractFileInfo()
bool CSingleDownloader::StartDownload() 
{
	if(m_pNodeInfo->sDownUrl.GetLength() < 7) //"http://"
		return false;

	m_bDownPaused = false;
	m_bStopped = false;
	CloseFile();

//	static int iTmp = 0;
	//..Extracting real download url..
// 	if(!m_bInfoExtractSuc)
// 	{
// 		Notify(WM_SDSTATUS_FINFAILED, 0);
// 		DoFinalClean();
// 		return false;
// 	}


	_beginthreadex(NULL, 0, DownThreadFunc, this, 0, NULL);

	return true;
}

void CSingleDownloader::InitStaticEnv()
{
	curl_global_init(CURL_GLOBAL_ALL);
}

void CSingleDownloader::FreeStaticEnv()
{
	curl_global_cleanup();
}

void CSingleDownloader::DoFinalClean()
{
	CloseFile();
	if(m_eh)
	{
		curl_easy_cleanup(m_eh);
		m_eh = NULL;
	}
	SAFE_DEL_POINER(m_pNodeInfo);
	SAFE_DEL_POINER(m_pMsgParam);
}

void CSingleDownloader::StopDownload()
{
	m_bStopped = true;
	//DoFinalClean();
}

void CSingleDownloader::PauseDownload()
{
	m_bDownPaused = true;
	// return magic code in WriteFunc
}

void CSingleDownloader::UnpauseDownload()
{
	m_bSetUnpause = true;
	m_bDownPaused = false;
}

unsigned __stdcall CSingleDownloader::DownThreadFunc(void* param)
{
	if(!param)
		return 1;

	CSingleDownloader* pThis = (CSingleDownloader*)param;
	pThis->_Down();

	return 0;
}

void CSingleDownloader::_Down()
{
	Notify(WM_SDSTATUS_INIT, 0); //收到这个消息，一般就可以从TDownNode中获取文件信息了
	
	if(!InitCurl())
	{
		Notify(WM_SDSTATUS_FINFAILED, 0);
		CloseFile();

		return;
	}

	if(m_pNodeInfo->bBreakptResume && (m_pNodeInfo->dFileSize - m_pNodeInfo->lLocalSize < 0.3f))
	{
		Notify(WM_SDSTATUS_FINSUC, 0);
		CloseFile();
		return;
	}
			

	/* Perform the request, iRet will get the return code */
	CURLcode iRet = curl_easy_perform(m_eh);

	CloseFile(); //FIRST CLOSE FILE ALWAYS

	/* Check for errors */
	if(iRet == CURLE_OK && m_pNodeInfo->lHttpStatus < 400)
	{
		//fprintf(stderr, "ok.");
		Notify(WM_SDSTATUS_FINSUC, 0);
		
	}
	else if(m_bStopped)
	{
		Notify(WM_SDSTATUS_STOPPED, 0);
	}
	else
	{
		//fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(iRet));
		Notify(WM_SDSTATUS_FINFAILED, 0);
	}

	/* always cleanup */
	//CloseFile(); //DoFinalClean();don't call it for reuse

}

void CSingleDownloader::Notify(EDownloadStatus status, LPARAM lParam)
{
	if(!m_pNodeInfo || !(m_pNodeInfo->hNotifyWnd))
		return;

	//closing file plays a vital role
	if((status == WM_SDSTATUS_STOPPED) || (status == WM_SDSTATUS_FINSUC) || (status == WM_SDSTATUS_FINFAILED))
		CloseFile();

	if(m_pMsgParam->sNameWithPath.GetLength() < 1)
		m_pMsgParam->sNameWithPath.Format("%s%s", m_pNodeInfo->sSavePath.GetString(),m_pNodeInfo->sFileName.GetString());
	//::PostMessageW(m_pNodeInfo->hNotifyWnd, status, (WPARAM)m_pMsgParam, lParam);
	::SendMessageW(m_pNodeInfo->hNotifyWnd, status, (WPARAM)m_pMsgParam, lParam);
}

void CSingleDownloader::CloseFile()
{
	if(m_pNodeInfo && m_pNodeInfo->pFILE)
	{
		fclose(m_pNodeInfo->pFILE);
		m_pNodeInfo->pFILE = NULL;
	}
}

bool CSingleDownloader::InitCurl()
{
	//init file
	if(!(m_pNodeInfo->bOutToStr))//once bOutToStr is true, bBreakptResume won't matter
	{
		if(!PathFileExistsA(m_pNodeInfo->sSavePath.GetString()))
		{
			if(!CreateMultiLevelDir(m_pNodeInfo->sSavePath.GetString()))
				return false;
		}

		char szFile[MAX_PATH + 1] = {0};
		_snprintf(szFile,-1,"%s%s",m_pNodeInfo->sSavePath.GetString(), m_pNodeInfo->sFileName.GetString());

		if(!(m_pNodeInfo->bBreakptResume))
		{
			if(PathFileExistsA(szFile))  
				DeleteFileA(szFile);
		}
		
		FILE* pFile = NULL;
		fopen_s(&pFile, szFile,"ab+");
		if(pFile == NULL) //文件打开错误，进行下一个文件的下载
		{
			CloseFile();
			return false;
		}
		m_pNodeInfo->pFILE = pFile;
	}
	else
		m_pNodeInfo->sDownData.Empty();

	//get local file size
	if(m_pNodeInfo->bBreakptResume)
		m_pNodeInfo->lLocalSize = _filelength(_fileno(m_pNodeInfo->pFILE));

	//url encode..
	//m_pNodeInfo->sDownUrl = EncodeURI(m_pNodeInfo->sDownUrl);//在下载YouTube视频方面有问题


	//init curl easy instance
	if(m_eh)
		curl_easy_reset(m_eh); //reuse curl handle
	else
		m_eh = curl_easy_init();

	curl_easy_setopt(m_eh, CURLOPT_URL, m_pNodeInfo->sDownUrl.GetString());
	/* if url is redirected, so we tell libcurl to follow redirection */
	curl_easy_setopt(m_eh, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(m_eh, CURLOPT_HEADER, 0L);//don't include header-info into body
	//curl_easy_setopt(m_eh, CURLOPT_HEADERDATA, this);
	//curl_easy_setopt(m_eh, CURLOPT_HEADERFUNCTION,HeaderCallBack);
	curl_easy_setopt(m_eh, CURLOPT_NOBODY, 0); //只是想获取FileInfo的话就不需要body
	curl_easy_setopt(m_eh, CURLOPT_WRITEFUNCTION, WriteFunc);
	curl_easy_setopt(m_eh, CURLOPT_WRITEDATA, this);  //file pointer usually
	curl_easy_setopt(m_eh, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(m_eh, CURLOPT_XFERINFOFUNCTION, ProgressFunc); //not encouraged:CURLOPT_PROGRESSFUNCTION
	curl_easy_setopt(m_eh, CURLOPT_XFERINFODATA, this); //not encouraged:CURLOPT_PROGRESSDATA
	if(m_pNodeInfo->bBreakptResume)
		 curl_easy_setopt(m_eh, CURLOPT_RESUME_FROM, m_pNodeInfo->lLocalSize);
	if(m_pNodeInfo->bPostData)
	{
		curl_easy_setopt(m_eh, CURLOPT_POSTFIELDS, m_pNodeInfo->sPostData.GetString());
		curl_easy_setopt(m_eh, CURLOPT_POST, 1);
	}
	return true;
}

void CSingleDownloader::SetDownNodeInfo(char* sDownUrl, char* sFileNameAppend, char* sIdentifier,bool bOutToStr, bool bBreakptResume, char* sSavePath, HWND hNotifyWnd)
{
	//!调用此方法设置DownNodeInfo,一定要先调用ExtractFileInfo()再调用StartDownload()否则FileName设置没有效果
	if(sDownUrl)
		m_pNodeInfo->sDownUrl = sDownUrl;
	if(sFileNameAppend)
		m_pNodeInfo->sFileNameAppend = sFileNameAppend;
	if(sIdentifier)
	{
		m_pNodeInfo->sIdentifier = sIdentifier;
		m_pMsgParam->sIdentifier = sIdentifier;
	}
	m_pNodeInfo->bOutToStr = bOutToStr;
	m_pNodeInfo->bBreakptResume = bBreakptResume;
	if(sSavePath)
	{
		m_pNodeInfo->sSavePath = sSavePath;//m_pMsgParam->sNameWithPath在ExtractFile()中完成拼合
	}

	if(hNotifyWnd)
		m_pNodeInfo->hNotifyWnd = hNotifyWnd;
}

TDownNodeInfo* CSingleDownloader::GetDownNodeInfo()
{
	return m_pNodeInfo;
}

bool CSingleDownloader::GotFileName()
{
	//assert(m_pNodeInfo);
	return m_pNodeInfo->sFileName.GetLength() > 0;
}