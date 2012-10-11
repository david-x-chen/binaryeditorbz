// BZAnalyzerView.cpp : 実装ファイル
//

#include "stdafx.h"
#include "BZ.h"
#include "BZAnalyzerView.h"
#include "ProgressDialog.h"
#include "zlib.h"
#include "BZDoc.h"
#include "Splitter.h"
//#include "..\..\CFolderDialog\folderdlg.h"//atldlgs.hからCFolderDialogだけ切り取ったもの。shlobj.hのinclude, CFolderDialogImpl, CFolderDialog, ATL_NO_VTABLEを__declspec(novtable)に変更, ATLTRACEをコメントアウトしたもの。代わりにWTLのatldlgs.hをインクルードしても良い。CPL感染するので同梱しない。


// CBZAnalyzerView

IMPLEMENT_DYNCREATE(CBZAnalyzerView, CFormView)

CBZAnalyzerView::CBZAnalyzerView()
	: CFormView(CBZAnalyzerView::IDD)
{

}

CBZAnalyzerView::~CBZAnalyzerView()
{
}

void CBZAnalyzerView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDP_ANALYZE_PERCENT, m_progress);
	DDX_Control(pDX, IDL_ANALYZE_RESULT, m_resultList);
	DDX_Control(pDX, IDC_ANALYZE_TYPE, m_combo_analyzetype);
}

BEGIN_MESSAGE_MAP(CBZAnalyzerView, CFormView)
	ON_BN_CLICKED(IDB_ANALYZE_START, &CBZAnalyzerView::OnBnClickedAnalyzeStart)
	ON_BN_CLICKED(IDB_ANALYZER_SAVE, &CBZAnalyzerView::OnBnClickedAnalyzerSave)
	ON_BN_CLICKED(IDB_ANALYZER_SAVEALL, &CBZAnalyzerView::OnBnClickedAnalyzerSaveall)
END_MESSAGE_MAP()


// CBZAnalyzerView 診断

#ifdef _DEBUG
void CBZAnalyzerView::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void CBZAnalyzerView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG


// CBZAnalyzerView メッセージ ハンドラ

void CBZAnalyzerView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();

	// TODO: ここに特定なコードを追加するか、もしくは基本クラスを呼び出してください。
	if(m_combo_analyzetype.GetCount()==0)
	{
		m_combo_analyzetype.InsertString(0, "zlib (deflate)");
		m_combo_analyzetype.SetCurSel(0);

		m_resultList.DeleteAllItems();
		m_resultList.InsertColumn(0, "Address", LVCFMT_LEFT, 120);
	//	m_resultList.InsertColumn(1, "Size", LVCFMT_LEFT, 80);
	}
	CSplitterWnd* pSplit = (CSplitter*)GetParent();
	pSplit->SetColumnInfo(0, 180, 0);
}

__inline BOOL IsZlibDeflate(unsigned char firstChar)
{
	switch(firstChar)
	{
	case 0x08:
	case 0x18:
	case 0x28:
	case 0x38:
	case 0x48:
	case 0x58:
	case 0x68:
	case 0x78:
		return true;
	default:
		return false;
	}
}

void CBZAnalyzerView::OnBnClickedAnalyzeStart()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	m_resultList.DeleteAllItems();

//	CProgressDialog dlgProgress;
//	dlgProgress.DoModal();

//	m_resultList.InsertItem(0, "0x00000000");
//	m_resultList.SetItemText(0, 1, "5000");

	int iListIndex = -1;

	CBZDoc* pDoc = (CBZDoc*)GetDocument();
	ASSERT(pDoc);
	LPBYTE p  = pDoc->GetDocPtr();
	DWORD filesize = pDoc->GetDocSize();

	unsigned int outbufsize = 1;
	LPBYTE outbuf = (LPBYTE)malloc(outbufsize);
	int inflateStatus = Z_OK;

	for(DWORD ofs_inflateStart = 0; ofs_inflateStart < filesize; ofs_inflateStart++)
	{
		if(!IsZlibDeflate(*(p+ofs_inflateStart)))continue;

		z_stream z = {0};
		z.next_out = outbuf;
		z.avail_out = outbufsize;

		DWORD ofs = ofs_inflateStart;
		DWORD dwSize_Nokori = 1000;
		if(inflateInit(&z)!=Z_OK)continue;
		/*do*/ {
		/*	if(z.avail_out==0)
			{
				z.next_out = outbuf;
				z.avail_out = outbufsize;
				//リスト追加
				CString str;
				str.Format("0x%08X", ofs_inflateStart);
				m_resultList.InsertItem(++iListIndex, str);
			//	m_resultList.SetItemText(0, 1, "5000");
				break;
			}*/
#ifdef FILE_MAPPING
			if(p && !(p = pDoc->QueryMapView(p, ofs))) return;
			DWORD dwFind = pDoc->GetMapSize();
#endif //FILE_MAPPING
			DWORD dwSize = min(min(dwFind, dwSize_Nokori), 100);
			z.next_in = p+ofs;
			z.avail_in = dwSize;
			inflateStatus = inflate(&z, Z_NO_FLUSH);
			dwSize_Nokori -= dwSize;
//バグ：設定のファイルマッピングのサイズが１０バイトくらい以下だと誤判定するかも。ループで最低５０バイト程度は供給しないと
		} /*while(dwSize_Nokori > 0 && inflateStatus==Z_OK);*/
		if(inflateStatus==Z_OK||inflateStatus==Z_STREAM_END)
		{
			CString str;
			str.Format("0x%08X", ofs_inflateStart);
			m_resultList.InsertItem(++iListIndex, str);
		//	m_resultList.SetItemText(0, 1, "5000");
		}
		TRACE("BZAnalyzerView(0x%08X) inflateStatus==%d\n",ofs_inflateStart , inflateStatus);
		inflateEnd(&z);
	}

	free(outbuf);
	
//	m_resultList.InsertItem(++iListIndex, "End");
}

unsigned long CBZAnalyzerView::GetAddress(int nItem)
{
	TCHAR tmpbuf[20];
	m_resultList.GetItemText(nItem, 0, tmpbuf, 19);
	tmpbuf[19]=NULL;//Safety
	return strtoul(tmpbuf+2, NULL, 16);
}

BOOL CBZAnalyzerView::MakeExportDirA(LPSTR pathOutputDir, LPCSTR pathDstFolder)
{
	char lastDir[_MAX_PATH];
	sprintf_s(lastDir, _MAX_PATH, "%s\\", ::PathFindFileNameA(GetDocument()->GetPathName()));
	strcpy_s(pathOutputDir, _MAX_PATH, pathDstFolder);
	return ::PathAppendA(pathOutputDir, lastDir);
}

int CBZAnalyzerView::MakeExportPathA(LPSTR pathOutput, LPCSTR pathDir, unsigned long ulStartAddr)
{
	return sprintf_s(pathOutput, _MAX_PATH, "%s%08X.bin", pathDir, ulStartAddr);
}

void CBZAnalyzerView::OnBnClickedAnalyzerSave()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	WTL::CFolderDialog dlg(NULL, NULL, BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
	if(dlg.DoModal()==IDOK)
	{
		POSITION pos = m_resultList.GetFirstSelectedItemPosition();
		if(pos==NULL)
		{
			MessageBox("no selected", "Error", MB_OK);
			return;
		}

		unsigned int outbufsize = 1024000;
		LPBYTE outbuf = (LPBYTE)malloc(outbufsize);

		char pathOutputDir[_MAX_PATH];
		MakeExportDirA(pathOutputDir, dlg.GetFolderPath());
		CAtlList<int> delList;

		while(pos)
		{
			int nItem = m_resultList.GetNextSelectedItem(pos);
			unsigned long ulStartAddr = GetAddress(nItem);
			if(FAILED(SaveFileA(pathOutputDir, ulStartAddr, outbuf, outbufsize))) delList.AddHead(nItem);
		}
		free(outbuf);

		POSITION listpos = delList.GetHeadPosition();
		while(listpos!=NULL)
		{
			int nDelItem = delList.GetNext(listpos);
			m_resultList.DeleteItem(nDelItem);
		}
	}
}

void CBZAnalyzerView::OnBnClickedAnalyzerSaveall()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	WTL::CFolderDialog dlg(NULL, NULL, BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
	if(dlg.DoModal()==IDOK)
	{
		unsigned int outbufsize = 1024000;
		LPBYTE outbuf = (LPBYTE)malloc(outbufsize);

		char pathOutputDir[_MAX_PATH];
		MakeExportDirA(pathOutputDir, dlg.GetFolderPath());
		CAtlList<int> delList;
		
		int itemcount = m_resultList.GetItemCount();

		for(int i=0; i<itemcount; i++)
		{
			unsigned long ulStartAddr = GetAddress(i);
			if(FAILED(SaveFileA(pathOutputDir, ulStartAddr, outbuf, outbufsize))) delList.AddHead(i);
		}
		free(outbuf);

		POSITION listpos = delList.GetHeadPosition();
		while(listpos!=NULL)
		{
			int nDelItem = delList.GetNext(listpos);
			m_resultList.DeleteItem(nDelItem);
		}
	}
}

HRESULT CBZAnalyzerView::SaveFileA(LPCSTR pathOutputDir, unsigned long ulStartAddr, LPBYTE outbuf, unsigned int outbufsize)
{
	char pathOutput[_MAX_PATH];

	CBZDoc* pDoc = (CBZDoc*)GetDocument();
	ASSERT(pDoc);
	LPBYTE p  = pDoc->GetDocPtr();
	if(p==NULL)
	{
		MessageBox("GetDocPtr() error", "Error", MB_OK);
		return E_FAIL;
	}

	int retMakePath = MakeExportPathA(pathOutput, pathOutputDir, ulStartAddr);
	FILE *fp;
	if(retMakePath==-1)
	{
		MessageBox("Path error", "Error", MB_OK);
		return E_FAIL;
	}
	if(!MakeSureDirectoryPathExists(pathOutputDir))
	{
		MessageBox("makedir error", "Error", MB_OK);
		return E_FAIL;
	}
	if(fopen_s(&fp, pathOutput, "wb")!=0)
	{
		MessageBox("fopen error", "Error", MB_OK);
		return E_FAIL;
	}


	z_stream z = {0};
	z.next_out = outbuf;
	z.avail_out = outbufsize;
	int inflateStatus = Z_OK;

	if(inflateInit(&z)!=Z_OK)
	{
		MessageBox("inflateInit error", "Error", MB_OK);
		goto saveerr;
	}

	DWORD nextOffset = ulStartAddr;
	DWORD dwTotal = pDoc->GetDocSize();
	
	do {
		if(z.avail_in==0)
		{
			if(nextOffset>=dwTotal)goto saveerr2;
#ifdef FILE_MAPPING
	/*		if(!(p = pDoc->QueryMapView(p, nextOffset)))
			{
				MessageBox("mapping error", "Error", MB_OK);
				goto saveerr2;
			}
			DWORD dwFind = pDoc->GetMapSize();
		//	if(
		ファイルマッピング関連の問題があるので保留
		ファイル全体がマッピングされてないとアクセス例外がでる
		*/
#endif //FILE_MAPPING
			DWORD dwSize = /*min(dwFind,*/ 4096/*)*/;
			z.next_in = p+nextOffset;
			z.avail_in = dwSize;
			nextOffset+=dwSize;
		}
		inflateStatus = inflate(&z, Z_NO_FLUSH);
		if(z.avail_out==0)
		{
			if(fwrite(outbuf, 1, outbufsize, fp)!=outbufsize)
			{
				MessageBox("fwrite error", "Error", MB_OK);
				goto saveerr2;
			}
			z.next_out = outbuf;
			z.avail_out = outbufsize;
		}
	} while(inflateStatus==Z_OK);

	if(inflateStatus==Z_STREAM_END)
	{
		DWORD nokori = outbufsize - z.avail_out;
		if(nokori!=0)
		{
			if(fwrite(outbuf, 1, nokori, fp)!=nokori)
			{
				MessageBox("fwrite error2", "Error", MB_OK);
				goto saveerr2;
			}
		}
	} else {
//		MessageBox("inflate error", "Error", MB_OK);
		goto saveerr2;
	}

	if(_ftelli64(fp)<=0)
	{
		goto saveerr2;
	}

	inflateEnd(&z);
	fclose(fp);

	return S_OK;


saveerr2:
	inflateEnd(&z);

saveerr:
	fclose(fp);
	DeleteFileA(pathOutput);

	return E_FAIL;
}