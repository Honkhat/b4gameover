#pragma once

#include <curl/curl.h> 
#include <atlstr.h>

#define  WM_SDOWNLOADER    (WM_USER + 100)

//libcurl version:curl-7.38.0
enum EDownloadStatus
{
	WM_SDSTATUS_None = WM_SDOWNLOADER,
	WM_SDSTATUS_INIT,
	WM_STATUS_GOTFILEINFO, //get file info(name,size,http status code)
	WM_SDSTATUS_DOWNING,
	WM_SDSTATUS_PAUSED,
	WM_SDSTATUS_STOPPED,
	WM_SDSTATUS_FINSUC,
	WM_SDSTATUS_FINFAILED
};

struct TDownNodeInfo
{
	TDownNodeInfo(): hNotifyWnd(NULL),lHttpStatus(0),bOutToStr(false),pFILE(NULL),bFindContentDisp(false),dFileSize(0),bBreakptResume(true),
		lLocalSize(0),dAverSpeed(0.0),bPostData(false)
	{
	}
	~TDownNodeInfo()
	{
		if(pFILE)
		{
			fclose(pFILE);
			pFILE = NULL;
		}
	}

	CStringA sDownUrl;
	CStringA sSavePath; //MUST end up with "\\" or "/"
	CStringA sFileName;
	CStringA sFileNameAppend;//e.g.  FileName:"EntityData.mp3"  sFileNameAppend:"128bkps"==> "EntityData_128bkps.mp3"
	CStringA sIdentifier; //用于标识一个single downloader
	CStringA sDownDate;
	CStringA sDownData; //bOutToStr为true时输出到这里	
	HWND     hNotifyWnd;	
	long     lHttpStatus;
	double   dFileSize;
	double   dAverSpeed; //针对所有下载文件的平均速度,而非瞬时速度. 单位:bytes/second
	long     lLocalSize;
	bool     bBreakptResume; //是否启用断点续传
	bool     bOutToStr;
	bool     bFindContentDisp; //是否从server获取到了文件名
	FILE*    pFILE;
	bool     bPostData;
	CStringA sPostData;
};

struct TMsgParam
{
	TMsgParam():iPercent(0), dAverSpeed(0.0)
	{}

	CStringA sIdentifier;
	int      iPercent;
	double   dAverSpeed;
	CStringA sNameWithPath;
};

class CSingleDownloader
{
public:	
	CSingleDownloader(TDownNodeInfo* pDownNode);//pDownNode是外部new出来的;
	CSingleDownloader(char* url, char* savepath, char* idstr, HWND notifywnd, char* szNameAppend = NULL, bool bBreakptResume = true, bool bHttpGet = false);//no filename?
	~CSingleDownloader(void);
	
	bool ExtractFileInfo();
	bool StartDownload(); //如果返回true，不一定成功(因为是多线程);但如果返回失败，一定失败
	void StopDownload();
	void PauseDownload();
	void UnpauseDownload();
	void SetDownNodeInfo(char* sDownUrl,  char* sFileNameAppend, char* sIdentifier,bool bOutToStr = false, bool bBreakptResume=true, char* sSavePath = NULL, HWND hNotifyWnd = NULL);
	TDownNodeInfo* GetDownNodeInfo();
	bool GotFileName();
	void DoFinalClean();

	static void InitStaticEnv();//整个类的生存周期中只需调用一次
	static void FreeStaticEnv();//同上
	static unsigned __stdcall DownThreadFunc(void* param);
	static unsigned __stdcall ExtractThreadFunc(void* param);
	static size_t WriteFunc(char *str, size_t size, size_t nmemb, void *stream);
	//static size_t ProgressFunc(void* client, double t,double d,double ultotal,double ulnow); 
	static size_t ProgressFunc(void* client, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow); 
	static size_t HeaderCallBack(char *buffer,size_t size,size_t nmemb,void *userdata);
	static CString UnifyByteStr_Speed(double dBytePerSec);

private:
	CSingleDownloader(void);

	void _Down();
	void _Extract();
	bool InitCurl();
	void CloseFile();

	void Notify(EDownloadStatus status, LPARAM lParam);
	
	void ExtractFileNameFromUrl();
	void InsertFileNameAppend();

	
private:
	TDownNodeInfo* m_pNodeInfo;  //每一个CSingleDownloader维护一个m_pNodeInfo
	TMsgParam*     m_pMsgParam;  //发送消息直接发送这个指针
	bool     m_bDownPaused; //stop != pause
	bool     m_bStopped;
	bool     m_bInfoExtractSuc;
	bool     m_bSetUnpause;
	CURL*    m_eh;
};
