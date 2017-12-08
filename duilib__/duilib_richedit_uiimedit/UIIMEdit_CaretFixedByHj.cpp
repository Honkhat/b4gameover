#include "stdafx.h"
#include <atlbase.h>
#include <atlcom.h>
#include "UIIMEdit.h"
//#include "Modules/IMiscModule.h"
//#include "Modules/IP2PCmdModule.h"
#include "../Base/IEmotionModule.h"
#include "GifSmiley/GifSmiley.h"
#include "../Utils/UtilsHelper.h"
#include "cxImage/cxImage/ximage.h"
#include "UI/Animator//OleImage.h"
#include "IUserListModule.h"
#include "../Utils/imagelibs.h"

#define FRM_TIMER_ID			1

string MixedMsg::AddPicTeg2Pic(IN string picPath)
{
	string strAddedMSG;
	strAddedMSG += CS_SPLIT_CODE_START.c_str();
	strAddedMSG += picPath.c_str();
	strAddedMSG += CS_SPLIT_CODE_END.c_str();
	return strAddedMSG;
}

MixedMsg::MixedMsg()
:m_nSetNetWorkPathSuccTime(0),m_nFailerUploadPicTime(0),m_msgFeature(0),m_validate(0)
{
  m_id = utils::GenerateUUID();
  transform(m_id.begin(), m_id.end(), m_id.begin(), ::tolower);
}

BOOL MixedMsg::SetNetWorkPicPath(IN string strLocalPicPath, IN string strNetPicPath)
{
    for(auto iter = m_picDataVec.begin(); iter != m_picDataVec.end(); iter++)
	{
        auto &picData = *iter;
		if(picData.strLocalPicPath == strLocalPicPath)
		{
			picData.strNetPicPath = strNetPicPath;
			m_nSetNetWorkPathSuccTime++;
			return true;
		}

	}

	return FALSE;
}

BOOL MixedMsg::SucceedToGetAllNetWorkPic()
{
	return m_nSetNetWorkPathSuccTime == m_picDataVec.size();
}

void MixedMsg::SetFailerUploadPicTime()
{
	m_nFailerUploadPicTime++;
}

BOOL MixedMsg::SucceedReturnAllUploadPicResult()
{
	return (m_nSetNetWorkPathSuccTime + m_nFailerUploadPicTime) == m_picDataVec.size();
}

string MixedMsg::MakeMixedLocalMSG()
{
	/*wstring msg = utils::AsciiToUnicode(m_strTextData.c_str());
	int nPosAdd = 0;
	for(auto iter = m_picDataVec.begin(); iter != m_picDataVec.end(); iter++)
	{
		auto picData = *iter;

		string strPic = AddPicTeg2Pic((picData.strLocalPicPath));
		msg.insert(nPosAdd+picData.nPos, utils::Utf8ToUnicode(strPic).c_str());
		nPosAdd += strPic.length();
	}

	return utils::UnicodeToAscii(msg);*/

	string msg = m_strTextData.c_str();
	int nPosAdd = 0;
	for(auto iter = m_picDataVec.begin(); iter != m_picDataVec.end(); iter++)
	{
		auto picData = *iter;

		//david, edit, 2017-11-13
		//string strPic = AddPicTeg2Pic((picData.strLocalPicPath));
		string strPic = AddPicTeg2Pic((picData.strThumbFile));
		msg.insert(nPosAdd+picData.nPos, strPic.c_str());
		nPosAdd += strPic.length();
	}

	return msg;
}

string MixedMsg::MakeMixedNetWorkMSG()
{
	/*wstring msg = utils::AsciiToUnicode(m_strTextData.c_str());
	int nPosAdd = 0;
	for(auto iter = m_picDataVec.begin(); iter != m_picDataVec.end(); iter++)
	{
		auto picData = *iter;

		wstring strPic = utils::Utf8ToUnicode(AddPicTeg2Pic((picData.strNetPicPath)));
		msg.insert(nPosAdd+picData.nPos, strPic.c_str());

		int pos = nPosAdd + picData.nPos + strPic.length();
		msg.erase(pos, 1);//?
		nPosAdd += strPic.length();
		--nPosAdd;
	}

	return utils::UnicodeToAscii(msg);*/

	string msg = m_strTextData.c_str();
	int nPosAdd = 0;
	for(auto iter = m_picDataVec.begin(); iter != m_picDataVec.end(); iter++)
	{
		auto picData = *iter;

		string strPic = AddPicTeg2Pic((picData.strNetPicPath));
		msg.insert(nPosAdd+picData.nPos, strPic.c_str());

		int pos = nPosAdd + picData.nPos + strPic.length();
		msg.erase(pos, 1);//?
		nPosAdd += strPic.length();
		--nPosAdd;
	}

	return msg;
}

std::string getMJName(int mj)
{
	for(auto iter = g_CurrentAccountMjVec.begin(); iter != g_CurrentAccountMjVec.end(); iter++)
	{
		auto mjInfo = *iter;

		if(mjInfo.value == mj)
		{
			return mjInfo.name;
		}
	}

	return "";
}

string MixedMsg::GetThumbFile(string src, std::string mjName, boolean isNeedWatermaker)
{
	CxImage cximage;
	bool bSucc = cximage.Load(src.c_str());
	if (bSucc)
	{
		double width = cximage.GetWidth();
		double height = cximage.GetHeight();
		double msgFrameWidth = 300;//639*0.6;

		char buf[1024]= {0};
		sprintf_s(buf, "%sthumb_%s.jpg", utils::getCurAccountCachePath().c_str(), utils::GenerateUUID().c_str());
		string filename = buf;

		if(width > msgFrameWidth)
		{
			CxImage thumbImg;
			double dratio = msgFrameWidth/width;
			cximage.Resample(msgFrameWidth, height * dratio, 0, &thumbImg);

			if(thumbImg.IsValid())
			{
				thumbImg.SetJpegQuality(90);
				if(thumbImg.Save(filename.c_str(),CXIMAGE_FORMAT_JPG))
				{
					if(isNeedWatermaker)
						AddTextWaterMarker(filename, mjName);

					return filename;
				}
			}
		}
		else 
		{
			cximage.SetJpegQuality(100);
			if(cximage.Save(filename.c_str(),CXIMAGE_FORMAT_JPG))
			{
				if(isNeedWatermaker)
					AddTextWaterMarker(filename,mjName);

				return filename;
			}
		}
	}

	return src;
}

BOOL  MixedMsg::MakeSendMsgExtend(MessageEntity& msg)
{
	static const string EMAIL_IMG_TAG = "<img name=\"thumb\" src=\"cid:%s\"/>";

	string textData = m_strTextData.c_str();
	int nPosAdd = 0;
	for(auto iter = m_picDataVec.begin(); iter != m_picDataVec.end(); iter++)
	{
		auto picData = *iter;

		//string strPic = picData.strLocalPicPath;

		string cid = utils::GenerateUUID();

		char pstrImgTag[100]={0};
		sprintf_s(pstrImgTag, EMAIL_IMG_TAG.c_str(), cid.c_str());

		textData.insert(nPosAdd+picData.nPos, pstrImgTag);
		nPosAdd += strlen(pstrImgTag);

		EmbeddedEntity embedded;

		embedded.cid = cid;
		embedded.sourceFile = picData.strLocalPicPath;
		embedded.filename = picData.strThumbFile; //GetThumbFile(strPic, mjName);
		embedded.size = picData.fileSize; //utils::getFileSize(strPic.c_str());
		embedded.url = picData.strNetPicPath;
		embedded.position = picData.nPos;
		embedded.mj = picData.attachMj;


		msg.emebeddedVec.push_back(embedded);

	}

	if( (msg.msgType == MESSAGE_TYPE_IMAGE) || (msg.msgType == MESSAGE_TYPE_AUDIO) || (msg.msgType == MESSAGE_TYPE_VIDEO) )
		msg.sendContent = "";
	else 
		msg.sendContent = textData;

	return true;
}

BOOL MixedMsg::IsPureTextMsg()
{
	return m_picDataVec.empty();
}

void MixedMsg::ReplaceReturnKey(void)
{
	if (m_strTextData.empty())
    {
        return;
    }

    std::vector<std::string> _vecSpliter;
    _vecSpliter.push_back("\r\n");
    _vecSpliter.push_back("\r");
    _vecSpliter.push_back("\n");

    std::vector<std::string> _vecList;
    utils::splitString(m_strTextData.c_str(), _vecSpliter, _vecList);

    m_strTextData = _T("");
    for (unsigned int i = 0; i < _vecList.size(); i++)
    {
        m_strTextData += _vecList[i].c_str();

		if(i<_vecList.size()-1)
          m_strTextData += _T("\r\n");
    }
}


// -----------------------------------------------------------------------------
//  UIIMEdit: Public, Constructor

UIIMEdit::UIIMEdit()
{
}

// -----------------------------------------------------------------------------
//  UIIMEdit: Public, Destructor

UIIMEdit::~UIIMEdit()
{
	this->m_pManager->KillTimer(this);
	SetText("");
}

LPVOID UIIMEdit::GetInterface(LPCTSTR pstrName)
{
	if (_tcscmp(pstrName, _T("UIIMEdit")) == 0) return static_cast<UIIMEdit*>(this);
	return __super::GetInterface(pstrName);
}

LPCTSTR UIIMEdit::GetClass() const
{
	return _T("UIIMEdit");
}

void UIIMEdit::_ImEditPaste()
{
	BOOL bHadPic = FALSE;
	if (::OpenClipboard(::GetDesktopWindow()))
	{
		if (IsClipboardFormatAvailable(CF_BITMAP))
		{
			HBITMAP hBitmap = (HBITMAP)::GetClipboardData(CF_BITMAP);
			BITMAP bitmap;
			GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bitmap);
			SIZE bitmapSize = { bitmap.bmWidth, bitmap.bmHeight};
			if (hBitmap)
			{
				_SaveFile(hBitmap, m_strImagePath);

				_variant_t strVar(m_strImagePath.c_str()); 
				InsertImage(strVar.bstrVal,bitmapSize,FALSE);
				bHadPic = TRUE;
			}
		}
		CloseClipboard();
	}
}

BOOL UIIMEdit::_SaveFile(IN HBITMAP hbitmap, OUT string& strFilePath)
{
	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);
	char strTime[1024]={0};
	sprintf(strTime, _T("%d-%02d-%02d-%02d%02d%02d-%03d"), st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		st.wMilliseconds);	

	utils::CreateFullPath(utils::getCurAccountCachePath())
		;
	string csDstFileName = utils::getCurAccountCachePath() + strTime;
	csDstFileName += _T(".jpg");
	
	CxImage cximage;
	cximage.CreateFromHBITMAP(hbitmap);

	if (cximage.Save(csDstFileName.c_str(), CXIMAGE_FORMAT_JPG))
	{
		strFilePath = csDstFileName;
		return TRUE;
	}
	
	return FALSE;
}

HBITMAP UIIMEdit::_LoadAnImage(IN string filePath)
{
	HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); //��ָ����·��szImagePath�ж�ȡ�ļ����
	DWORD dwFileSize = GetFileSize(hFile, NULL); //���ͼƬ�ļ��Ĵ�С����������ȫ���ڴ�
	HGLOBAL hImageMemory = GlobalAlloc(GMEM_MOVEABLE, dwFileSize); //��ͼƬ����ȫ���ڴ�
	void *pImageMemory = GlobalLock(hImageMemory); //�����ڴ�
	DWORD dwReadedSize; //����ʵ�ʶ�ȡ���ļ���С
	ReadFile(hFile, pImageMemory, dwFileSize, &dwReadedSize, NULL); //��ȡͼƬ��ȫ���ڴ浱��
	GlobalUnlock(hImageMemory); //�����ڴ�
	CloseHandle(hFile); //�ر��ļ����

	HRESULT hr = NULL;
	IStream *pIStream = NULL;//����һ��IStream�ӿ�ָ�룬��������ͼƬ��
	IPicture *pIPicture = NULL;//����һ��IPicture�ӿ�ָ�룬��ʾͼƬ����

	hr = CreateStreamOnHGlobal(hImageMemory, false, &pIStream); //��ȫ���ڴ��ʹ��IStream�ӿ�ָ��
	ASSERT(SUCCEEDED(hr));

	hr = OleLoadPicture(pIStream, 0, false, IID_IPicture, (LPVOID*)&(pIPicture));//��OleLoadPicture���IPicture�ӿ�ָ��
	ASSERT(SUCCEEDED(hr));

	HBITMAP hB = NULL;
	pIPicture->get_Handle((unsigned int*)&hB);
	// Copy the image. Necessary, because upon p's release,
	// the handle is destroyed.
	HBITMAP hBB = (HBITMAP)CopyImage(hB, IMAGE_BITMAP, 0, 0,
		LR_COPYRETURNORG);

	GlobalFree(hImageMemory); //�ͷ�ȫ���ڴ�
	pIStream->Release(); //�ͷ�pIStream
	pIPicture->Release(); //�ͷ�pIPictur
	return hBB;

	return NULL;
}

int UIIMEdit::GetObjectPos()
{
	bool findObject = false;

	IRichEditOle *pRichEditOle = m_pRichEditOle;
	if (NULL == pRichEditOle)
	{
		return 0;
	}

	int nCount = pRichEditOle->GetObjectCount();
	for (int i = nCount - 1; i >= 0; i--)
	{
		REOBJECT reobj = { 0 };
		reobj.cbStruct = sizeof(REOBJECT);
		pRichEditOle->GetObject(i, &reobj, REO_GETOBJ_POLEOBJ);
		reobj.poleobj->Release();
	}
	return 0;
}


void UIIMEdit::InsertImage(BSTR bstrFileName,SIZE size,BOOL isGif)
{

	// ȫ��ʹ������ָ��
	CComPtr<IStorage> spStorage;
	CComPtr<ILockBytes> spLockBytes;
	CComPtr<IOleClientSite> spOleClientSite;	
	CComPtr<COleImage> spOleImage;
	CComPtr<IOleObject> spOleObject;
	CLSID clsid;
	REOBJECT reobject;
	HRESULT hr = E_FAIL;

	do {

		// ����LockBytes
		hr = CreateILockBytesOnHGlobal(NULL, TRUE, &spLockBytes);
		if (hr != S_OK) {
			break;
		}

		ASSERT(spLockBytes != NULL);

		// ����Storage
		hr = StgCreateDocfileOnILockBytes(spLockBytes,
			STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE, 0, &spStorage);
		if (hr != S_OK) {
			break;
		}

		// ��ȡClientSite
		hr = m_pRichEditOle->GetClientSite(&spOleClientSite);
		if (hr != S_OK) {
			break;
		}

		// ����COleImageʵ��
		hr = CoCreateInstance(CLSID_OleImage, NULL, CLSCTX_INPROC, IID_IOleImage, (LPVOID*) &spOleImage);
		if (hr != S_OK) {
			break;
		}

		// ����ͼ��
		hr = spOleImage->LoadFromFile(bstrFileName, this->m_pManager->GetPaintWindow(), this->GetWidth()-20, isGif);
		if (hr != S_OK) {
			break;
		}

		// ��ȡIOleObject�ӿ�
		hr = spOleImage->QueryInterface(IID_IOleObject, (LPVOID *) &spOleObject);
		if (hr != S_OK) {
			break;
		}

		// ��ȡIOleObject���û�CLSID
		hr = spOleObject->GetUserClassID(&clsid);
		if (hr != S_OK) {
			break;
		}

		// ���OLE��������
		ZeroMemory(&reobject, sizeof(REOBJECT));		
		reobject.cbStruct	= sizeof(REOBJECT);
		reobject.clsid		= clsid;
		reobject.cp			= REO_CP_SELECTION;
		reobject.dvaspect	= DVASPECT_CONTENT;
		reobject.dwFlags	= REO_BELOWBASELINE;
		reobject.poleobj	= spOleObject;
		reobject.polesite	= spOleClientSite;
		reobject.pstg		= spStorage;
		SIZEL sizel = {0};
		reobject.sizel = sizel;

		// ����OLE����
		hr = m_pRichEditOle->InsertObject(&reobject);
		if (hr != S_OK) {
			break;
		}

		// ֪ͨOLE������֤OLE������ȷ����
		hr = OleSetContainedObject(spOleObject, TRUE);

	} while (0);

	return;

	/*LPSTORAGE lpStorage = NULL;
	LPOLEOBJECT	lpObject = NULL;
	LPLOCKBYTES lpLockBytes = NULL;
	LPOLECLIENTSITE lpClientSite = NULL;
	GifSmiley::IGifSmileyCtrl* lpAnimator = nullptr;
	HRESULT hr = ::CoCreateInstance(GifSmiley::CLSID_CGifSmileyCtrl, NULL, CLSCTX_INPROC, GifSmiley::IID_IGifSmileyCtrl, (LPVOID*)&lpAnimator);
	if (NULL == lpAnimator || FAILED(hr))
	{
		LOG__(ERR, _T("InsertImage CoCreateInstance failed"));
		goto End;
	}

	COLORREF backColor = (COLORREF)(::GetSysColor(COLOR_WINDOW));
	HWND hwnd = (HWND)((long)m_pManager->GetPaintWindow());
	IRichEditOle *pRichEditOle = m_pRichEditOle;
	if (NULL == pRichEditOle)
		goto End;
	BSTR path = NULL;
	//Create lockbytes
	hr = ::CreateILockBytesOnHGlobal(NULL, TRUE, &lpLockBytes);
	if (FAILED(hr))
	{
		LOG__(ERR, _T("InsertImage CreateILockBytesOnHGlobal failed"));
		goto End;
	}
	//use lockbytes to create storage
	SCODE sc = ::StgCreateDocfileOnILockBytes(lpLockBytes, STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE, 0, &lpStorage);
	if (sc != S_OK)
	{
		LOG__(ERR, _T("InsertImage StgCreateDocfileOnILockBytes failed"));
		goto End;
	}
	// retrieve OLE interface for richedit   and  Get site
	pRichEditOle->GetClientSite(&lpClientSite);
	try
	{
		//COM operation need BSTR, so get a BSTR
		path = bstrFileName;
		//Load the image
		if (isGif)
			lpAnimator->LoadFromFile(path);
		else
		{
			UInt32 height = (size.cy < GetHeight()) ? size.cy : GetHeight();
			UInt32 width = (size.cx < GetWidth() / 2) ? size.cx : GetWidth() / 2;
			lpAnimator->LoadFromFileSized(path, width, height);
		}
		//Set back color
		OLE_COLOR oleBackColor = (OLE_COLOR)backColor;
		lpAnimator->put_BackColor(oleBackColor);
		//get the IOleObject
		hr = lpAnimator->QueryInterface(IID_IOleObject, (void**)&lpObject);
		if (FAILED(hr))
		{
			LOG__(ERR, _T("InsertImage lpAnimator QueryInterface failed"));
			goto End;
		}
		//Set it to be inserted
		OleSetContainedObject(lpObject, TRUE);
		//to insert into richedit, you need a struct of REOBJECT
		REOBJECT reobject;
		ZeroMemory(&reobject, sizeof(REOBJECT));
		reobject.cbStruct = sizeof(REOBJECT);
		CLSID clsid;
		hr = lpObject->GetUserClassID(&clsid);
		//set clsid
		reobject.clsid = clsid;
		//can be selected
		reobject.cp = REO_CP_SELECTION;
		//content, but not static
		reobject.dvaspect = DVASPECT_CONTENT;
		//goes in the same line of text line
		reobject.dwFlags = REO_BELOWBASELINE;
		//reobject.dwUser = (DWORD)myObject;
		//the very object
		reobject.poleobj = lpObject;
		//client site contain the object
		reobject.polesite = lpClientSite;
		//the storage 
		reobject.pstg = lpStorage;
		SIZEL sizel = { 0 };
		reobject.sizel = sizel;
		LPOLECLIENTSITE lpObjectClientSite = NULL;
		hr = lpObject->GetClientSite(&lpObjectClientSite);
		if (FAILED(hr) || lpObjectClientSite == NULL)
			lpObject->SetClientSite(lpClientSite);
		pRichEditOle->InsertObject(&reobject);
		//redraw the window to show animation
		::RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
	}
	catch (...)
	{
		LOG__(ERR, _T("InsertImage unknown exeption"));
	}

End:
	if (lpClientSite)
	{
		lpClientSite->Release();
		lpClientSite = nullptr;
	}
	if (lpObject)
	{
		lpObject->Release();
		lpObject = nullptr;
	}
	if (lpLockBytes)
	{
		lpLockBytes->Release();
		lpLockBytes = nullptr;
	}
	if (lpStorage)
	{
		lpStorage->Release();
		lpStorage = nullptr;
	}
	if (lpAnimator)
	{
		lpAnimator->Release();
		lpAnimator = nullptr;
	}
	*/
}


/*
void UIIMEdit::InsertImage(BSTR bstrFileName,SIZE size,BOOL isGif)
{
	CComPtr<IStorage> spStorage;
	CComPtr<ILockBytes> spLockBytes;
	CComPtr<IOleClientSite> spOleClientSite;	
	CComPtr<IOleObject> spOleObject;
	CComPtr<GifSmiley::IGifSmileyCtrl> lpAnimator;

	//GifSmiley::IGifSmileyCtrl* lpAnimator = nullptr;
	HRESULT hr = ::CoCreateInstance(GifSmiley::CLSID_CGifSmileyCtrl, NULL, CLSCTX_INPROC, GifSmiley::IID_IGifSmileyCtrl, (LPVOID*)&lpAnimator);
	if (NULL == lpAnimator || FAILED(hr))
	{
		LOG__(ERR, _T("InsertImage CoCreateInstance failed"));
        return;
	}

	COLORREF backColor = (COLORREF)(::GetSysColor(COLOR_WINDOW));
	HWND hwnd = (HWND)((long)m_pManager->GetPaintWindow());
	IRichEditOle *pRichEditOle = m_pRichEditOle;
	if (NULL == pRichEditOle)
		return;

	BSTR path = NULL;
	//Create lockbytes
	hr = ::CreateILockBytesOnHGlobal(NULL, TRUE, &spLockBytes);
	if (FAILED(hr))
	{
		LOG__(ERR, _T("InsertImage CreateILockBytesOnHGlobal failed"));
		return;
	}
	//use lockbytes to create storage
	SCODE sc = ::StgCreateDocfileOnILockBytes(spLockBytes, STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE, 0, &spStorage);
	if (sc != S_OK)
	{
		LOG__(ERR, _T("InsertImage StgCreateDocfileOnILockBytes failed"));
		return;
	}
	// retrieve OLE interface for richedit   and  Get site
	pRichEditOle->GetClientSite(&spOleClientSite);
	try
	{
		//COM operation need BSTR, so get a BSTR
		path = bstrFileName;
		//Load the image
		if (isGif)
			lpAnimator->LoadFromFile(path);
		else
		{
			UInt32 height = (size.cy < GetHeight()) ? size.cy : GetHeight();
			UInt32 width = (size.cx < GetWidth() / 2) ? size.cx : GetWidth() / 2;
			lpAnimator->LoadFromFileSized(path, width, height);
		}
		//Set back color
		OLE_COLOR oleBackColor = (OLE_COLOR)backColor;
		lpAnimator->put_BackColor(oleBackColor);
		//get the IOleObject
		hr = lpAnimator->QueryInterface(IID_IOleObject, (void**)&spOleObject);
		if (FAILED(hr))
		{
			LOG__(ERR, _T("InsertImage lpAnimator QueryInterface failed"));
			return;
		}
		//Set it to be inserted
		OleSetContainedObject(spOleObject, TRUE);
		//to insert into richedit, you need a struct of REOBJECT
		REOBJECT reobject;
		ZeroMemory(&reobject, sizeof(REOBJECT));
		reobject.cbStruct = sizeof(REOBJECT);
		CLSID clsid;
		hr = spOleObject->GetUserClassID(&clsid);
		//set clsid
		reobject.clsid = clsid;
		//can be selected
		reobject.cp = REO_CP_SELECTION;
		//content, but not static
		reobject.dvaspect = DVASPECT_CONTENT;
		//goes in the same line of text line
		reobject.dwFlags = REO_BELOWBASELINE;
		//reobject.dwUser = (DWORD)myObject;
		//the very object
		reobject.poleobj = spOleObject;
		//client site contain the object
		reobject.polesite = spOleClientSite;
		//the storage 
		reobject.pstg = spStorage;
		SIZEL sizel = { 0 };
		reobject.sizel = sizel;
		LPOLECLIENTSITE lpObjectClientSite = NULL;
		hr = spOleObject->GetClientSite(&lpObjectClientSite);
		if (FAILED(hr) || lpObjectClientSite == NULL)
			spOleObject->SetClientSite(spOleClientSite);
		pRichEditOle->InsertObject(&reobject);
		//redraw the window to show animation
		::RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
	}
	catch (...)
	{
		LOG__(ERR, _T("InsertImage unknown exeption"));
	}

}
*/

void UIIMEdit::DoEvent(TEventUI& event)
{
	if( event.Type == UIEVENT_TIMER ) {

		if (event.wParam == FRM_TIMER_ID)
		{
			int nObjectCount = m_pRichEditOle->GetObjectCount();
			if (nObjectCount > 0) {
				RECT rc = GetPos();
				// �ֱ�ʹ�����ϽǺ����½ǵĵ�õ���С������ַ�����
				// ���ɼ��������С������ַ�����
				CPoint topleft = CPoint(rc.left, rc.top);
				CPoint bottomright = CPoint(rc.right, rc.bottom);
				int cpMin = CharFromPos(topleft);
				int cpMax = CharFromPos(bottomright);

				// ʹ�ö��ֲ����㷨�ҵ���һ���ַ��������ڻ����cpMin�Ķ�������
				int iFirst = FindFirstObject(cpMin, nObjectCount);
				REOBJECT reo = {0};
				reo.cbStruct = sizeof(REOBJECT);

				// �ӵ�һ��������ʼ�����������֡
				for (int i = iFirst; i < nObjectCount; i++) {
					if (m_pRichEditOle->GetObject(i, &reo, REO_GETOBJ_POLEOBJ) == S_OK) {

						if(reo.poleobj)
						   reo.poleobj->Release();
						// ��ǰ������ַ�������������ַ�������˵�������ڿɼ�����ֹͣ����
						if (reo.cp > cpMax) {
							break;
						}

						// ��COleImage����ʱ���ܸ���
						if (InlineIsEqualGUID(reo.clsid, CLSID_OleImage)) {
							// ����֡
							COleImage *pOleImage = COleImage::FromOleObject(reo.poleobj);
							if(pOleImage)
							   pOleImage->ChangeFrame();
						}
					}
				}
			}
		}
	}

	CRichEditUI::DoEvent(event);
}


HRESULT UIIMEdit::GetNewStorage(LPSTORAGE* ppStg)
{

	if (!ppStg)
		return E_INVALIDARG;

	*ppStg = NULL;

	//
	// We need to create a new storage for an object to occupy.  We're going
	// to do this the easy way and just create a storage on an HGLOBAL and let
	// OLE do the management.  When it comes to saving things we'll just let
	// the RichEdit control do the work.  Keep in mind this is not efficient, 
	// but this program is just for demonstration.
	//

	LPLOCKBYTES pLockBytes;
	HRESULT hr = CreateILockBytesOnHGlobal(NULL, TRUE, &pLockBytes);
	if (FAILED(hr))
		return hr;

	hr = StgCreateDocfileOnILockBytes(pLockBytes,
		STGM_SHARE_EXCLUSIVE | STGM_CREATE |
		STGM_READWRITE,
		0,
		ppStg);
	pLockBytes->Release();
	return (hr);
}


void UIIMEdit::GetObjectInfo(IRichEditOle *pIRichEditOle)
{
	long count = pIRichEditOle->GetObjectCount();
	if (count)
	{
		REOBJECT reobj = { 0 };
		reobj.cbStruct = sizeof(REOBJECT);
		pIRichEditOle->GetObject(0, &reobj, REO_GETOBJ_POLEOBJ);
		GifSmiley::IGifSmileyCtrl* lpAnimator = 0;
		HRESULT hr = reobj.poleobj->QueryInterface(GifSmiley::IID_IGifSmileyCtrl, (void**)&lpAnimator);
		if (SUCCEEDED(hr) && lpAnimator)
		{
			BSTR*  fileName = nullptr;
			hr = lpAnimator->FileName(fileName);
		}

		if(reobj.poleobj)
			reobj.poleobj->Release();
	}
}

void UIIMEdit::ReleaseAllGif()
{
	/*SetText(_T(""));
	IRichEditOle *pRichEditOle = m_pRichEditOle;
	PTR_VOID(pRichEditOle);

	for (int i = 0; i < pRichEditOle->GetObjectCount(); i++)
	{
		REOBJECT reobj = { 0 };
		reobj.cbStruct = sizeof(REOBJECT);
		pRichEditOle->GetObject(i, &reobj, REO_GETOBJ_POLEOBJ);
		GifSmiley::IGifSmileyCtrl* lpAnimator = 0;
		HRESULT hr = reobj.poleobj->QueryInterface(GifSmiley::IID_IGifSmileyCtrl, (void**)&lpAnimator);
		if (SUCCEEDED(hr) && lpAnimator)
		{
			lpAnimator->FreeImage();
			lpAnimator->Release();
		}
		reobj.poleobj->Release();
	}*/
}

BOOL UIIMEdit::GetPicPosAndPathbyOrder(IN UInt32 nOrder, OUT UInt32& nPos, OUT string& path)
{
	IRichEditOle* pRichEditOle = m_pRichEditOle;
	PTR_FALSE(pRichEditOle);

	REOBJECT reobj = { 0 };
	reobj.cbStruct = sizeof(REOBJECT);
	HRESULT hr = pRichEditOle->GetObject(nOrder, &reobj, REO_GETOBJ_POLEOBJ);
	if (SUCCEEDED(hr) && reobj.poleobj)
	{
	
		/*//GifSmiley::IGifSmileyCtrl* lpAnimator = 0;
		CComPtr<COleImage> lpAnimator;
		//hr = reobj.poleobj->QueryInterface(GifSmiley::IID_IGifSmileyCtrl, (void**)&lpAnimator);
		hr = reobj.poleobj->QueryInterface(IID_IOleImage, (void**)&lpAnimator);
		if (SUCCEEDED(hr) && lpAnimator)
		{
			CComBSTR  fileName;
			hr = lpAnimator->FileName(&fileName);
			if (SUCCEEDED(hr) && 0 != fileName.Length())
			{
				nPos = reobj.cp;
				_bstr_t b = fileName.m_str;
				path = b;
			}
			//lpAnimator->Release();
		}
		reobj.poleobj->Release();*/


		// ��COleImage����ʱ���ܸ���
		if (InlineIsEqualGUID(reobj.clsid, CLSID_OleImage)) {
			// ����֡
			COleImage *pOleImage = COleImage::FromOleObject(reobj.poleobj);
			if(pOleImage) {

				CComBSTR  fileName;
				hr = pOleImage->FileName(&fileName);
				if (SUCCEEDED(hr) && 0 != fileName.Length())
				{
					nPos = reobj.cp;
					_bstr_t b = fileName.m_str;
					path = b;
				}

			}
		}

		if(reobj.poleobj)
			reobj.poleobj->Release();

		return TRUE;
	}
	return FALSE;
}

int UIIMEdit::FindFirstObject(int cpMin, int nObjectCount)
{
	// ��׼�Ķ��ֲ����㷨�����ý�����
	int low = 0;
	int high = nObjectCount - 1;
	REOBJECT reoMid = {0};
	reoMid.cbStruct = sizeof(REOBJECT);
	while (low <= high) {
		int mid = (low + high) >> 1;
		if (m_pRichEditOle->GetObject(mid, &reoMid, REO_GETOBJ_POLEOBJ) != S_OK) {
			return -1;
		}
		if(reoMid.poleobj)
		  reoMid.poleobj->Release();
		
		if (reoMid.cp == cpMin) {
			return mid;
		} else if (reoMid.cp < cpMin) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	// ֻ�������û�ҵ�ʱ���Ƿ���-1�����Ƿ���low����ʱlow��Ȼ���ڻ����high
	// �պ���������
	return low;
}

LRESULT UIIMEdit::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
{
	if (uMsg == WM_KEYDOWN)
	{
		if(m_pManager->GetFocus() == this)//���һ��PaintManager�����ж��UIIMEdit, Ctrl+Vճ��, ���е�UIIMEdit����ճ����;jian.he 2017/11/22 14:01;
		{
			if ('V' == wParam && ::GetAsyncKeyState(VK_CONTROL) & 0x8000)
			{
				_ImEditPaste();
			}
			else if (VK_RETURN == wParam)//�س�
			{
				int sysBaseConfig = SystemConfig::GetBaseConfig();
				BOOL bWantCtrlEnter = (sysBaseConfig & BASE_FLAG_SENDIMG_BY_CTRLENTER);
				SetWantReturn(!bWantCtrlEnter);
			}
		}	
	}
	else if (uMsg == WM_KILLFOCUS)
	{
		//removed by kuaidao 2015-03-05,��λ��ᵼ�½���֮���л�������
		////fix bug,��Ƕ��Windows�ؼ�ʱ��������windows�ؼ�����Ƕ���IE�����Ǹô��ڵ�һ���Ӵ��ڣ��õ�Focus��ʱ���Լ��Ľ��㲢û��ȥ��������´β���������
		//https://code.google.com/p/duilib/issues/detail?id=102
		//if (m_bFocused && m_pManager
		//	&& m_pManager->GetFocus() == this)
		//{
		//	OutputDebugString(_T("-------WM_KILLFOCUS\n"));
		//	m_pManager->SetFocus(NULL);
		//	m_bFocused = FALSE;
		//}
		/*
		if (m_pTwh)
		{
			m_pTwh->GetTextServices()->OnTxUIDeactivate();
			m_pTwh->GetTextServices()->TxSendMessage(WM_KILLFOCUS, 0, 0, 0);
			m_pTwh->TxShowCaret(FALSE);
		}*/
	}
	else if (uMsg == WM_SETFOCUS)
	{/*
		if (m_pTwh)
		{
			m_pTwh->GetTextServices()->OnTxUIActivate();
			m_pTwh->GetTextServices()->TxSendMessage(WM_SETFOCUS, 0, 0, 0);
			m_pTwh->TxShowCaret(TRUE);
		}*/
	}
	else if(uMsg == WM_TIMER)
	{

	
	}

	return CRichEditUI::MessageHandler(uMsg, wParam, lParam, bHandled);
}

bool UIIMEdit::OnTxViewChanged()
{
	m_pManager->SendNotify(this, UIIMEdit_MSGTYPE_TEXTCHANGED);
	return true;
}

BOOL UIIMEdit::GetContent(OUT MixedMsg& mixMsg)
{
	mixMsg.m_strTextData = GetTextRange(0, GetTextLength());
	if (mixMsg.m_strTextData.empty())
		return FALSE;
    mixMsg.ReplaceReturnKey();

	IRichEditOle *pRichEditOle = m_pRichEditOle;
	if (NULL == pRichEditOle)
	{
		SetText(_T(""));
		return FALSE;
	}
	UInt32 nImageCount = pRichEditOle->GetObjectCount();
	if (nImageCount == 0)//������
	{
		string strContent = mixMsg.m_strTextData;
		//strContent.Trim();

		strContent = utils::trim(strContent);

		if (strContent.empty())
		{
			LOG__(DEBG, _T("After trimed,is empty msg"));//��־����
			return FALSE;
		}
	}
	else//ͼ�Ļ���
	{
		string strEmotionFilesDir = utils::getEmotionFilesDir();
		wstring wsTextData = utils::AsciiToUnicode(mixMsg.m_strTextData);

		int nPosAdd = 0;
		for (UInt32 i = 0; i < nImageCount; i++)
		{
			ST_picData picData;
			UInt32 wsTextPos = 0;
			if (GetPicPosAndPathbyOrder(i, wsTextPos, picData.strLocalPicPath))
			{

				picData.nPos = utils::UniPosToAscPos(wsTextData, wsTextPos);

				TCHAR fullPath[MAX_PATH] = { 0 };
				LPTSTR* pStart = nullptr;
				if (!GetFullPathName(picData.strLocalPicPath.c_str(), MAX_PATH, fullPath, pStart))
				{
					continue;
				}
				string strFullPath = fullPath;
				int nPos = strFullPath.find(strEmotionFilesDir);
				if (nPos != string::npos)
				{
					//�Ǳ��飬�����ϴ�ͼƬ
					int nLen = picData.strLocalPicPath.length();

					string fileName = picData.strLocalPicPath.substr(strEmotionFilesDir.length(), nLen-strEmotionFilesDir.length());

					string fileID;
					if(!module::getEmotionModule()->getEmotionIDByName(fileName, fileID))
					{
						return FALSE;
					}
					mixMsg.m_strTextData.insert(nPosAdd + picData.nPos, fileID);
					mixMsg.m_strTextData.erase(nPosAdd + picData.nPos + fileID.length(), 1);
					nPosAdd += fileID.length()-1;
				}
				else
				{
					picData.nPos += nPosAdd;
					mixMsg.m_picDataVec.push_back(picData);
				}
			}
		}
	}

	//SetText(_T(""));
	
	return true;
}

void UIIMEdit::DoInit()
{
	__super::DoInit();
	GetRichEditOle();
	this->m_pManager->SetTimer(this, FRM_TIMER_ID, MIN_FRM_DELAY);
}

int UIIMEdit::GetObjectCount()
{
   return m_pRichEditOle->GetObjectCount();
}




/******************************************************************************/