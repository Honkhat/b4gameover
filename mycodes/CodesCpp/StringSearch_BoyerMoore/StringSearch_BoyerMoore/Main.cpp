#include "stdafx.h"
#include <iostream>
#include "StringSearch_BoyerMoore.h"
#include <Windows.h>

int _tmain(int argc, _TCHAR* argv[])
{
	wchar_t wszMainStr[] = L"THIS IS A �������Ҳ��ǵ�LE EXA��������MPLEEXAMPLE����";
	wchar_t wszSearchStr[] = L"����";

	DWORD dwBegin = 0, dwEnd = 0;
	dwBegin = GetTickCount();
	StringSearch_BM(wszMainStr, wcslen(wszMainStr), wszSearchStr, wcslen(wszSearchStr), false);
	std::cout<<"Elapsed time using BM: "<<(GetTickCount()-dwBegin)*1.0/1000<<std::endl;

	for(TArrayInt::iterator it = g_arSearchResult.begin(); it != g_arSearchResult.end(); ++it)
		std::cout<<*it<<std::endl;

	//clear..
	g_arSearchResult.clear();

	getchar();
	return 0;
}


