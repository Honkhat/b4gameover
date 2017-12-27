// StringSearch_BoyerMoore.cpp : 定义控制台应用程序的入口点。
//
/*
	@Date: 2016/03/21 18:31;
	@Where: Tongzhou District of Beijing;
	@Author: Alex Hong;
	@Mail: worksdata@163.com;
	@Reference: 
		http://www.ruanyifeng.com/blog/2013/05/boyer-moore_string_search_algorithm.html BM算法-阮一峰的网络日志;
*/

#include <sstream>
#include <string>
#include <vector>
#include <map>

typedef std::vector<int> TArrayInt;
TArrayInt g_arSearchResult;//store the index of searched result;
typedef std::map<std::wstring, int> TMapWszToInt; //E.g. "我,4":7;
typedef std::map<int, int> TMapIntToInt;


void StringSearch_BM(wchar_t* wszStrMain, int iLenMain, wchar_t* wszSearch, int iLenSearch, bool bOnlyFirst)
{
	//reset variables;
	g_arSearchResult.clear();

	//handle exceptions..
	if( NULL == wszStrMain || NULL == wszSearch)
		return;
	if(iLenSearch > iLenMain)
		return;
	/*
		未知的字符有很多, 但是已知的搜索字符是有限的. 我们只初始化已知的字符的跳跃值(Skip), 数组中不存在的就是-1;
		Shift数组的初始化同样基于这种想法.
		初始化这两个数组的值后, 对于长主串/耗时的任务是有利的;
	*/

	int i = 0, j = 0;

	//Initialize the Skip array data for Bad-Character-Rules..
	TMapWszToInt mapSkip;
	mapSkip.clear();
	TMapWszToInt::iterator it1;
	std::wostringstream oStream;
	for(i = iLenSearch - 1; i > 0; --i)//0-->1;
	{
		for(j = i - 1; j >= 0; --j)
		{
			if(wszSearch[j] != wszSearch[i]) //Skip is for bad character which is different with wszSearch;
			{
				oStream.clear();//clear bad bits;
				oStream.str(L"");//clear data;
				oStream<<wszSearch[j]<<','<<i;//format to key string;
				//wcout<<oStream.str().c_str()<<endl;//test..
				it1 = mapSkip.find(oStream.str());
				if(mapSkip.end() == it1) //only set the pos of nearest same bad character;
					mapSkip.insert(TMapWszToInt::value_type(oStream.str(), i-j));
			}
		}
	}

	//好后缀的前提是: 至少有一个字符是相同的!!!!!!
	//Initialize the Shift array data for Good-Suffix-Rules..
	//Shift数组的键值是好后缀的最左字符的下标;
	//1. 首先判断模式串中是否含有完全一致的好后缀;有就直接计算位移, EXIT;
	//  其实这本身就是一个字符串搜索的过程,为了简化计算, 这里只使用Bad-Character-Rule;
	//2. 如果没有, 就在串首寻找好后缀的子串;
	TMapIntToInt mapShift;	
	mapShift.clear();
	mapShift.insert(TMapIntToInt::value_type(0, iLenSearch));//0==i的话就匹配了, Shift[]=iLenSearch;
	//1. try to find the same suffix..
	for(int iSuffixPosL = iLenSearch - 1; iSuffixPosL > 0; --iSuffixPosL)//iSuffixPosL: left position of good suffix;
	{
		int iSuffixLen = iLenSearch - iSuffixPosL;
		wchar_t* wszSuffix = wszSearch + iSuffixPosL;
		
		for(int iPosMainL = iSuffixPosL - 1; iPosMainL >= 0;)//不断向左移动suffix串比较;
		{
			//compare..
			for(i = 0; i < iSuffixLen; ++i)
			{
				if(wszSearch[iPosMainL+i] != wszSuffix[i])//bad character
					break;
			}
			//check result..
			if(iSuffixLen == i) //find it!
			{				
				mapShift[iLenSearch-i] = iSuffixPosL - iPosMainL; 
				break; //结束为当前suffix找Shift值;
			}
			else //calc the skip distance;
			{
				for(j = i + 1; j < iSuffixLen; ++j)//在suffix中寻找最近的坏字符
				{
					if(wszSuffix[j] == wszSearch[iPosMainL+i])
						break;
				}
				iPosMainL = iPosMainL - (j - i);//iSuffixLen==j的情况也适用;	
			}
		}
	}
	//2. try to find the child suffix at head..
	TMapIntToInt::iterator it2;
	for(int iSuffixPosL = iLenSearch - 1; iSuffixPosL > 0; --iSuffixPosL)//iSuffixPosL: left position of good suffix;
	{
		it2 = mapShift.find(iSuffixPosL);
		if(mapShift.end() == it2)
		{
			wchar_t* wszSuffix = NULL;
			int iSuffixLen  = 0;
			for(i = 0; i < iLenSearch - iSuffixPosL; ++i)//form sub suffix big to small.
			{
				wszSuffix = wszSearch + iSuffixPosL + i;
				iSuffixLen = iLenSearch - iSuffixPosL - i;
				//compare if match..				
				for(j = 0; j < iSuffixLen; ++j)
				{
					if(wszSearch[j] != wszSuffix[j])
						break;
				}
				if(iSuffixLen == j) //find it!
				{
					mapShift[iSuffixPosL] = iLenSearch - iSuffixLen; //以好后缀的最后一个字符为准;
					break;
				}
			}

			//if no sub suffix at all..
			it2 = mapShift.find(iSuffixPosL);
			if(mapShift.end() == it2)
				mapShift[iSuffixPosL] = iLenSearch;
		}
	}

	//================
	//start to search!
	//================
	int iPosMainL = 0;//left iterator of main string;
	//int iPosSearch = 0;//iterator of search string; no use!
	while( iPosMainL <= iLenMain - iLenSearch + 1)
	{
		//compare with aligned sub-string..
		int i = iLenSearch - 1;
		for(; i >= 0; --i)//right to left comparison;
		{
			if(wszSearch[i] != wszStrMain[iPosMainL+i])//bad character!
				break;
		}
		//----calc how many steps to move rightwards..----
		int iSkip = 0;
		if(i < 0)
		{
			g_arSearchResult.push_back(iPosMainL);
			if(bOnlyFirst)
			{
				mapSkip.clear();
				mapShift.clear();
				return;
			}
			iSkip = iLenSearch;
		}
		else
		{
			//1. calc max skip steps for bad characters..
			oStream.clear();//clear bits;
			oStream.str(L"");//clear data;
			oStream<<wszStrMain[iPosMainL+i]<<','<<i;
			it1 = mapSkip.find((wchar_t*)oStream.str().c_str());
			if(mapSkip.end() == it1)
				iSkip = i + 1;
			else
				iSkip = it1->second;
		}
		
		//--2. calc max shift steps for good suffix..--
		int iShift = 0;
		if(i < iLenSearch - 1)//至少有一个相同的字符才可以使用"好后缀"规则!
		{
			it2 = mapShift.find(i-1);
			if(mapShift.end() != it2)
				iShift = it2->second;
		}
		iPosMainL += ((iSkip>iShift) ? iSkip : iShift);
	}

	//clear..
	mapSkip.clear();
	mapShift.clear();
}
