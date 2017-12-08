#ifndef UIIMEDIT_3DC89058_72C4_4225_A6EB_0F39D182913F_H__
#define UIIMEDIT_3DC89058_72C4_4225_A6EB_0F39D182913F_H__


#include "../DuiLib/UIlib.h"
using namespace DuiLib;
#include "GlobalDefine.h"
#include "MessageEntity.h"
#include <RichOle.h>

#define UIIMEdit_MSGTYPE_TEXTCHANGED (_T("UIIMEdit_TEXT_Changed"))


/******************************************************************************/

/**
 * The class <code>UIIMEdit</code> 
 *
 */
namespace
{
	const string CS_SPLIT_CODE_START = _T("&$#@~^@[{:");
	const string CS_SPLIT_CODE_END = _T(":}]&$~@#@");
}
struct ST_picData
{
	UINT32	nPos;
	string strLocalPicPath;
	string strNetPicPath;
	int attachMj;
	string strThumbFile;
	int64  fileSize;
};

class MixedMsg
{
public:
	MixedMsg();
	BOOL SetNetWorkPicPath(IN string strLocalPicPath, IN string strNetPicPath);
	BOOL SucceedToGetAllNetWorkPic();
	string MakeMixedLocalMSG();
	string MakeMixedNetWorkMSG();
	BOOL  MakeSendMsgExtend(MessageEntity& msg);
	BOOL IsPureTextMsg();
	static string AddPicTeg2Pic(IN string picPath);
    void ReplaceReturnKey(void);
	static string GetThumbFile(string src, std::string mjName, boolean isNeedWatermaker = true);
	BOOL SucceedReturnAllUploadPicResult();
	void SetFailerUploadPicTime();
public:
	string                  m_id;
	string					m_strTextData;					//文字
	std::vector<ST_picData>	m_picDataVec;				//图片所在文字中的位置，图片的本地路径；图片的网络路径
	UINT32                   m_msgType;
	UINT32                   m_msgFeature;
	DWORD                    m_validate;
	string                   m_mailVerifier;
private:
	UINT32					m_nSetNetWorkPathSuccTime;	//成功获得的图片的次数
	UINT32                  m_nFailerUploadPicTime;
};


class UIIMEdit :public CRichEditUI
{
public:
    /** @name Constructors and Destructor*/

    //@{
    /**
     * Constructor 
     */
    UIIMEdit();
    /**
     * Destructor
     */
    virtual ~UIIMEdit();
    //@}

public:
	virtual void DoInit();
	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
	BOOL GetContent(OUT	MixedMsg& mixMsg);
	virtual LPVOID GetInterface(LPCTSTR pstrName);
	virtual LPCTSTR GetClass() const;
	virtual bool OnTxViewChanged();
	int	GetObjectPos();
	void InsertImage(BSTR bstrFileName, SIZE size, BOOL isGif);
	HRESULT GetNewStorage(LPSTORAGE* ppStg);
	void	GetObjectInfo(IRichEditOle *pIRichEditOle);
	void ReleaseAllGif();
	BOOL GetPicPosAndPathbyOrder(IN UInt32 nOrder,OUT UInt32& nPos,OUT string& path );
	int GetObjectCount();
private:
	void _ImEditPaste();
	BOOL _SaveFile(IN HBITMAP hbitmap, OUT string& strFilePath);
	HBITMAP _LoadAnImage(IN string filePath);
	
	void DoEvent(TEventUI& event);
	int FindFirstObject(int cpMin, int nObjectCount);
	
private:
	string                     m_strImagePath;
};
/******************************************************************************/
#endif// UIIMEDIT_3DC89058_72C4_4225_A6EB_0F39D182913F_H__
