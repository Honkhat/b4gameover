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
        public string sZi;
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

        static int CalcHanziGBKValueFromDByte(Byte b1, Byte b2)
        {
            //严格用位运算, 不要用加减运算(因为会做符号计算)!
            int i1 = 0, i2 = 0;
            i1 |= b1;//Byte是无符号的,所以扩展位数都是补0;
            i2 |= b2;
            return ((i1 << 8) | i2);
        }

        static int CalcHanziUniValueFromDByte(Byte b1, Byte b2)
        {
            //严格用位运算, 不要用加减运算(因为会做符号计算)!
            int i1 = 0, i2 = 0;
            i1 |= b2;//Byte是无符号的,所以扩展位数都是补0;
            i2 |= b1;
            return ((i1 << 8) | i2);
        }

        static void BatchSaveHanziInfoToDB(ref SQLiteConnection connDb, ref string sHanziTb, ref List<THanziInfo> listHanziInfo)
        {
            SQLiteTransaction trans = connDb.BeginTransaction();
            try
            {
                SQLiteCommand cmd = connDb.CreateCommand();
                cmd.Parameters.Add(cmd.CreateParameter());
                cmd.Parameters.Add(cmd.CreateParameter());
                cmd.Parameters.Add(cmd.CreateParameter());
                cmd.Parameters.Add(cmd.CreateParameter());
                foreach (THanziInfo infoZi in listHanziInfo)
                {
                    cmd.Transaction = trans;
                    cmd.CommandText = "INSERT INTO " + sHanziTb + "(GBKVAL,UNIVAL,ZI,PINYINS) VALUES (?,?,?,?);";
                    cmd.Parameters[0].Value = infoZi.iGbkVal;
                    cmd.Parameters[1].Value = infoZi.iUniVal;
                    cmd.Parameters[2].Value = infoZi.sZi;
                    cmd.Parameters[3].Value = infoZi.sPinyins;
                    cmd.ExecuteNonQuery();
                }
                trans.Commit();
                listHanziInfo.Clear();//清空已保存的批次, 不支持多线程!
            }
            catch
            {
                trans.Rollback();
                throw;
            }
        }

        static void GetPinyinAndSaveToDB(Byte b1, Byte b2, ref SQLiteConnection connDB)
        {
            ChineseChar cHanzi;
            System.Collections.ObjectModel.ReadOnlyCollection<string> arPinyin;
            string sTmp;

            int iGbkVal = CalcHanziGBKValueFromDByte(b1, b2);
            //PinyinConverter的输入参数是UNICODE编码, 所以需要将GBK VALUE转化成UNICODE VALUE;
            g_gbkBytes[0] = b1;
            g_gbkBytes[1] = b2;
            Byte[] uniBytes = Encoding.Convert(g_gbkEncoding, Encoding.Unicode, g_gbkBytes);
            if(uniBytes.Length < 2)
            {
                b1 = uniBytes[0];//应该不会出现这种情况吧?!!!!!!!
                //错误统计;
            }
            else
            {
                int iUniVal = CalcHanziUniValueFromDByte(uniBytes[0], uniBytes[1]);//UNICODE的存放顺序是高字节在uniBytes的后面;
                char chHanzi = (char)iUniVal;
                g_dicPinyin.Clear();
                try
                {
                    cHanzi = new ChineseChar(chHanzi);
                    arPinyin = cHanzi.Pinyins;
                    //不要音调,拼音滤重;
                    for (int j = 0; j < cHanzi.PinyinCount; ++j)
                    {
                        sTmp = arPinyin[j].TrimEnd(g_arTone);
                        if (!g_dicPinyin.ContainsKey(sTmp))
                            g_dicPinyin.Add(sTmp, true);
                    }
                    //push到一个汉字保存批次中;
                    THanziInfo infoHz = new THanziInfo();
                    infoHz.iGbkVal = iGbkVal;
                    infoHz.iUniVal = iUniVal;
                    infoHz.sZi += chHanzi;
                    foreach (string sKey in g_dicPinyin.Keys)
                    {
                        if (infoHz.sPinyins != null)
                            infoHz.sPinyins += ",";
                        infoHz.sPinyins += sKey;
                    }
                    g_listHanziInfo.Add(infoHz);

                    //超过设定数量, 触发保存到DB表的动作;
                    ++g_uTotalHanziCount;
                    ++g_uBatchCount;
                    if (g_uBatchCount > g_kHanziBatchSaveCount)
                    {
                        sTmp = g_kHanziTb;
                        BatchSaveHanziInfoToDB(ref connDB, ref sTmp, ref g_listHanziInfo);
                        g_uBatchCount = 0;
                    }
                }
                catch
                {
                    THanziInfo infoZiX = new THanziInfo();
                    infoZiX.iGbkVal = iGbkVal;
                    infoZiX.iUniVal = iUniVal;
                    infoZiX.sZi += chHanzi;
                    infoZiX.sPinyins = "";
                    g_listHanziInfoNotSupport.Add(infoZiX);
                    ++g_uTotalHanziNotSupportCount;
                }
            }
        }

        const string g_kHanziTb = "GBK_Hanzi";
        const string g_kHanziTb_MsNotSupport = "GBK_Hanzi_MS_NOT_SUPPORT";
        static uint g_kHanziBatchSaveCount = 5000;
        static char[] g_arTone = new char[] { '1', '2', '3', '4', '5' };//5:轻声;
        static Encoding g_gbkEncoding = Encoding.GetEncoding(936);//GBK code page:936;
        static Byte[] g_gbkBytes = new Byte[2];
        static Dictionary<string, bool> g_dicPinyin = new Dictionary<string,bool>();
        static List<THanziInfo> g_listHanziInfo = new List<THanziInfo>();
        static List<THanziInfo> g_listHanziInfoNotSupport = new List<THanziInfo>();
        static uint g_uTotalHanziCount = 0, g_uBatchCount = 0, g_uTotalHanziNotSupportCount = 0;

        static void Main(string[] args)
        {
            //TestConvert_SimplifiedChineseToComplex();
            //TestConvert_ChineseToPinyin();
            //TestCalcHanziValue();

            string sTmp;           
            

            //=============================
            //创建DB TABLE [GBK_Hanzi];
            //=============================
            sTmp = @"..\..\..\data\hanzi.db";
            if (!System.IO.File.Exists(sTmp))
                System.IO.File.Create(sTmp);//注意不支持创建多级目录!

            SQLiteConnection connDB = new SQLiteConnection("data source=" + sTmp);
            if (connDB.State != System.Data.ConnectionState.Open)
            {
                connDB.Open();
                SQLiteCommand cmd = connDB.CreateCommand();
                cmd.Connection = connDB;
                cmd.CommandText = "DROP TABLE IF EXISTS " + g_kHanziTb + ";CREATE TABLE " + g_kHanziTb + "(GBKVAL INTEGER PRIMARY KEY, UNIVAL INTEGER UNIQUE, ZI VARCHAR UNIQUE, PINYINS VARCHAR);" +
                    "DROP TABLE IF EXISTS "+ g_kHanziTb_MsNotSupport + ";CREATE TABLE " + g_kHanziTb_MsNotSupport + "(GBKVAL INTEGER PRIMARY KEY,UNIVAL INTEGER UNIQUE,ZI VARCHAR UNIQUE,PINYINS VARCHAR);";
                cmd.ExecuteNonQuery();
            }
            
            //==============================================================================================================================================================================
            //起初的策略: 从GBK_HanZi.DAT中每读取一行, 对此行的每个汉字, 查询拼音, 5000个作为一个批次, 保存到DB表[GBK_Hanzi];
            //发现的问题: "郎凉秊裏隣兀嗀"等字符出现了2次！对比权威点的GBK字符集网站, 发现是那个网站(千千秀字)的文字录入人员投机取巧导致的错误。
            //比如"郎"和"郎"字形是不一样的!
            //修正后的策略: 遍历包含汉字的GBK/2/3/4区, 直接使用两个字节的数字组合值作为一个汉字,能找到拼音的保存到DB [GBK_Hanzi]表中, 不能识别的保存到DB [GBK_Hanzi_MS_NOT_SUPPORT]表中;
            //==============================================================================================================================================================================
            
            //GBK/2区: [B0–F7][A1–FE]
            Byte b1 = 0, b2 = 0;
            for(b1 = 0xB0; b1 <= 0xF7; ++b1)
            {
                for(b2 = 0xA1; b2 <= 0xFE; ++b2)
                {
                    GetPinyinAndSaveToDB(b1, b2, ref connDB);
                }
            }

            //GBK/3区: [81–A0][40–FE except 7F]
            for(b1 = 0x81; b1 <= 0xA0; ++b1)
            {
                for(b2 = 0x40; b2 <= 0xFE; ++b2)
                {
                    if(0x7F == b2)
                        continue;
                    GetPinyinAndSaveToDB(b1, b2, ref connDB);
                }
            }
            
            //GBK/4区: [AA–FE][40–A0 except 7F]
            for(b1 = 0xAA; b1 <= 0xFE; ++b1)
            {
                for(b2 = 0x40; b2 <= 0xA0; ++b2)
                {
                    if(0x7F == b2)
                        continue;
                    GetPinyinAndSaveToDB(b1, b2, ref connDB);
                }
            }

            //FLUSH ALL UNTRANSLATED HANZIS TO DB TABLE.
            if(g_listHanziInfo.Count > 0)
            {
                sTmp = g_kHanziTb;
                BatchSaveHanziInfoToDB(ref connDB, ref sTmp, ref g_listHanziInfo);
            }

            if(g_listHanziInfoNotSupport.Count > 0)
            {
                sTmp = g_kHanziTb_MsNotSupport;
                BatchSaveHanziInfoToDB(ref connDB, ref sTmp, ref g_listHanziInfoNotSupport);
            }
           
            connDB.Close();
            Console.WriteLine("Unidentified Hanzi Count: " + g_uTotalHanziNotSupportCount);
            Console.WriteLine("Total Hanzi Count:" + g_uTotalHanziCount);
            Console.ReadKey();
        }
    }
}
