using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.VisualBasic;
using Microsoft.International.Converters.PinYinConverter;
using System.Data.SQLite;
using System.IO;

namespace GBK_HanziToPinyin
{
    struct THanziInfo
    {
        public int iGbkVal;
        public int iUniVal;
        public string sPinyins;//以","分割;
    }

    class Program
    {
        static void TestConvert_SimplifiedChineseToComplex()
        {
            string str = "车水马龙丒";
            string sSim, sCpx;
            //简体转繁体
            sCpx = Microsoft.VisualBasic.Strings.StrConv(str, Microsoft.VisualBasic.VbStrConv.TraditionalChinese, 1033);//1033:多字节编码;
            //繁体转简体
            sSim = Microsoft.VisualBasic.Strings.StrConv(str, Microsoft.VisualBasic.VbStrConv.SimplifiedChinese, 1033);
            Console.WriteLine(sSim + " " + sCpx);
        }

        static void TestConvert_ChineseToPinyin()
        {
            char ch1 = (char)0xb0a1;
            ChineseChar ccTEST1 = new ChineseChar(ch1);
            System.Collections.ObjectModel.ReadOnlyCollection<string> pinyins1 = ccTEST1.Pinyins;

            ChineseChar ccTEST = new ChineseChar('区');
            System.Collections.ObjectModel.ReadOnlyCollection<string> pinyins = ccTEST.Pinyins;
            int iPyCount = ccTEST.PinyinCount;
            for (int i = 0; i < pinyins.Count(); ++i)
                Console.WriteLine(pinyins[i] + " ");
        }

        static void TestDB()
        {
            string path = @"d:\hej.sqlite";
            if (!System.IO.File.Exists(path))
                System.IO.File.Create(path);//注意不支持创建多级目录!

            SQLiteConnection cn = new SQLiteConnection("data source=" + path);
            if (cn.State != System.Data.ConnectionState.Open)
            {
                cn.Open();
                SQLiteCommand cmd = new SQLiteCommand();
                cmd.Connection = cn;
                cmd.CommandText = "CREATE TABLE t1(id varchar(4),score int)";
                //cmd.CommandText = "CREATE TABLE IF NOT EXISTS t1(id varchar(4),score int)";
                cmd.ExecuteNonQuery();
            }
            cn.Close();
        }

        static void TestCalcHanziValue()
        {
            byte[] arHzBytes = new byte[2]{0xB0, 0xA1};
            int i1 = 0, i2 = 0;
            i1 |= arHzBytes[0];
            i2 |= arHzBytes[1];
            i1 = (i1 << 8) | i2;
            string sHex = Convert.ToString(i1, 16);
            Console.WriteLine(sHex);
        }

        //******************************************************************************
        //                       以下为动真格的方法
        //******************************************************************************

        static void CalcHanziValueFromBytes(ref byte[] arHzBytes, out int iGbkVal)
        {
            //严格用位运算, 不要用加减运算(因为会做符号计算)!
            int i1 = 0, i2 = 0;
            i1 |= arHzBytes[0];
            i2 |= arHzBytes[1];
            iGbkVal = (i1 << 8) | i2;
        }

        static void BatchSaveHanziInfoToDB(ref SQLiteConnection connDb, ref string kHanziTb, ref List<THanziInfo> listHanziInfo)
        {
            SQLiteTransaction trans = connDb.BeginTransaction();
            try
            {
                SQLiteCommand cmd = connDb.CreateCommand();
                cmd.Parameters.Add(cmd.CreateParameter());
                cmd.Parameters.Add(cmd.CreateParameter());
                cmd.Parameters.Add(cmd.CreateParameter());
                foreach (THanziInfo infoZi in listHanziInfo)
                {
                    cmd.Transaction = trans;
                    cmd.CommandText = "INSERT INTO " + kHanziTb + "(GBKVAL,UNIVAL,PINYINS) VALUES (?,?,?);";
                    cmd.Parameters[0].Value = infoZi.iGbkVal;
                    cmd.Parameters[1].Value = infoZi.iUniVal;
                    cmd.Parameters[2].Value = infoZi.sPinyins;
                    cmd.ExecuteNonQuery();
                }
                trans.Commit();
                listHanziInfo.Clear();//清空已保存的批次, 不支持多线程!
            }
            catch
            {
                throw;
                //trans.Rollback();
            }
        }

        static void Main(string[] args)
        {
            //TestConvert_SimplifiedChineseToComplex();
            //TestConvert_ChineseToPinyin();
            //TestCalcHanziValue();

            const string kHanziTb = "GBK_Hanzi";
            uint kHanziBatchSaveCount = 5000;
            uint iTotalHanziCount = 0, uBatchCount = 0;
            string sLine, sTmp, sUnknown="";
            string[] arHanzi;
            ChineseChar cHanzi;
            int iLangCount = 0, iLiangCount = 0, i3 = 0, i4 = 0, i5= 0,i6=0, i7=0;//"郎"字在GBK中居然有两个编码(C0C9+FD9C)!
            System.Collections.ObjectModel.ReadOnlyCollection<string> arPinyin;
            Dictionary<string, bool> dicPinyin = new Dictionary<string,bool>();
            char[] arTone = new char[] { '1', '2', '3', '4', '5' };//5:轻声;            
            List<THanziInfo> listHanziInfo = new List<THanziInfo>();
            Encoding gbkEncoding = Encoding.GetEncoding(936);//GBK code page:936;

            //=============================
            //创建DB TABLE [GBK_Hanzi];
            //=============================
            sTmp = @"..\..\data\hanzi.db";
            if (!System.IO.File.Exists(sTmp))
                System.IO.File.Create(sTmp);//注意不支持创建多级目录!

            SQLiteConnection connDB = new SQLiteConnection("data source=" + sTmp);
            if (connDB.State != System.Data.ConnectionState.Open)
            {
                connDB.Open();
                SQLiteCommand cmd = connDB.CreateCommand();
                cmd.Connection = connDB;
                cmd.CommandText = "DROP TABLE IF EXISTS " + kHanziTb + ";CREATE TABLE " + kHanziTb + "(GBKVAL INTEGER PRIMARY KEY, UNIVAL INTEGER UNIQUE, PINYINS VARCHAR);";//UNICODE转GBK很容易, 所以不必有UNICODEVALUE字段;
                cmd.ExecuteNonQuery();
            }
            
            //==============================================================================================================================================================================
            //起初的策略: 从GBK_HanZi.DAT中每读取一行, 对此行的每个汉字, 查询拼音, 5000个作为一个批次, 保存到DB表[GBK_Hanzi];
            //发现的问题: "郎凉秊裏隣兀嗀"等字符出现了2次！对比权威点的GBK字符集网站, 发现是那个网站(千千秀字)的文字录入人员投机取巧导致的错误。
            //比如"郎"和"郎"字形是不一样的!
            //修正后的策略: 遍历包含汉字的GBK/2/3/4区, 直接使用两个字节的数字组合值作为一个汉字,能找到拼音的保存到DB [GBK_Hanzi]表中, 不能识别的保存到DB [GBK_Hanzi_MS_NOT_SUPPORT]表中;
            //==============================================================================================================================================================================
            FileStream fIn = new FileStream(@"..\..\data\GBK_HanZi.DAT", FileMode.Open);
            StreamReader sr = new StreamReader(fIn, Encoding.GetEncoding(936)); //Encoding.Default //System.Text.DBCSCodePageEncoding;//使用Encoding.ASCII会出现不懂的后果!//GB2312对应的CodePage为936;
            while((sLine = sr.ReadLine()) != null)//已经由指定的Encoding编码转成了C#内部使用的UNICODE编码!
            {
                arHanzi = sLine.Split(' ');
                for (int i = 0; i < arHanzi.Length; ++i)
                {
                    int iLen = arHanzi[i].Length;
                    if(iLen > 0)
                    {
                        dicPinyin.Clear();
                        //iHzUni = (int)arHanzi[i][0];
                        try
                        {
                            cHanzi = new ChineseChar(arHanzi[i][0]);
                            arPinyin = cHanzi.Pinyins;
                            //不要音调,拼音滤重;
                            for(int j = 0; j < cHanzi.PinyinCount; ++j)
                            {
                                sTmp = arPinyin[j].TrimEnd(arTone);
                                if(!dicPinyin.ContainsKey(sTmp))
                                    dicPinyin.Add(sTmp, true);
                            }
                            //push到一个汉字保存批次中;
                            THanziInfo infoHz = new THanziInfo();
                            infoHz.iUniVal = arHanzi[i][0];
                            
                            byte[] gbkBytes = gbkEncoding.GetBytes(arHanzi[i]);
                            if (37070 == infoHz.iUniVal)//'郎'在GBK中有两个编码!
                            {
                                ++iLangCount;
                                if (iLangCount == 2)
                                    continue;
                            }
                            if (infoHz.iUniVal == 20937)//'凉'在GBK中有两个编码!
                            {
                                ++iLiangCount;
                                if (iLiangCount == 2)
                                    continue;
                            }

                            if (infoHz.iUniVal == 31178)//'凉'在GBK中有两个编码!
                            {
                                ++i3;
                                if (i3 == 2)
                                    continue;
                            }

                            if (infoHz.iUniVal == 35023)//'凉'在GBK中有两个编码!
                            {
                                ++i4;
                                if (i4 == 2)
                                    continue;
                            }

                            if (infoHz.iUniVal == 38563)//'凉'在GBK中有两个编码!
                            {
                                ++i5;
                                if (i5 == 2)
                                    continue;
                            }
                            if (infoHz.iUniVal == 20800)//'凉'在GBK中有两个编码!
                            {
                                ++i6;
                                if (i6 == 2)
                                    continue;
                            }
                            if (infoHz.iUniVal == 21952)//'凉'在GBK中有两个编码!
                            {
                                ++i7;
                                if (i7 == 2)
                                    continue;
                            }
                            
                            31036

                            CalcHanziValueFromBytes(ref gbkBytes, out infoHz.iGbkVal);
                            foreach (string sKey in dicPinyin.Keys)
                            {
                                if (infoHz.sPinyins != null)
                                    infoHz.sPinyins += ",";
                                infoHz.sPinyins += sKey;
                            }
                            listHanziInfo.Add(infoHz);

                            //超过设定数量, 触发保存到DB表的动作;
                            ++iTotalHanziCount;
                            ++uBatchCount;
                            if (uBatchCount > kHanziBatchSaveCount)
                            {
                                sTmp = kHanziTb;
                                BatchSaveHanziInfoToDB(ref connDB, ref sTmp, ref listHanziInfo);
                                uBatchCount = 0;
                            }
                        }
                        catch
                        {
                            //尝试繁体转换为简体后获取拼音..
                            //sCpx = Microsoft.VisualBasic.Strings.StrConv(str, Microsoft.VisualBasic.VbStrConv.TraditionalChinese, 1033);//1033:多字节编码;
                            //无法获取拼音的汉字也要插进表里去! 占位!
                            sUnknown += arHanzi[i];
                        }                        
                    }
                    else
                    {
                        sTmp = arHanzi[i];
                    }
                }
                //sql不支持无符号整型, 无符号有符号只不过对第1位的解释不同,二进制是一样的;                
            }

            //最后一批待保存到DB的汉字;

            Dictionary<int, int> dictTest = new Dictionary<int, int>();
            foreach (THanziInfo info1 in listHanziInfo)
            {
                if(dictTest.ContainsKey(info1.iGbkVal))
                {
                    int i = 1;
                    i += 2;
                }
                dictTest.Add(info1.iGbkVal, 1);
            }

            if (listHanziInfo.Count > 0)
            {
                sTmp = kHanziTb;
                //BatchSaveHanziInfoToDB(ref connDB, ref sTmp, ref listHanziInfo);
                SQLiteTransaction trans = connDB.BeginTransaction();

                SQLiteCommand cmd = connDB.CreateCommand();
                cmd.Parameters.Add(cmd.CreateParameter());
                cmd.Parameters.Add(cmd.CreateParameter());

                for (int i = 0; i < listHanziInfo.Count; ++i)
                {
                    if(i == 49352 || i == 20937)
                    {
                        i += 1;
                        i -= 1;
                    }
                    THanziInfo infoZi = listHanziInfo[i];
                    cmd.Transaction = trans;
                    cmd.CommandText = "INSERT INTO " + kHanziTb + "(GBKVAL,PINYINS) VALUES (?,?);";
                    cmd.Parameters[0].Value = infoZi.iGbkVal;
                    cmd.Parameters[1].Value = infoZi.sPinyins;
                    cmd.ExecuteNonQuery();
                }
                trans.Commit();
            }

            connDB.Close();
            Console.WriteLine("Unidentified Hanzi: " + sUnknown);
            Console.WriteLine("Total Hanzi Count:" + iTotalHanziCount);
            Console.ReadKey();
        }
    }
}
