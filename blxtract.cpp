/*
*  blxtract -- extracts CSVs from Dennis Montgomery's BLX format.
* 
* Create default C++ command-line project.
* Advanced -- C++/CLI Properties -- Common Language Runtime Support: Common Language Runtime Support (/clr)
* C/C++ -- All Options -- Additional Options: /Zc:twoPhase- %(AdditionalOptions)
* 
*/
#include <iostream>
#include <string>

using namespace System;
using namespace System::Collections;
using namespace System::Threading;
using namespace System::Text;
using namespace System;
using namespace System::IO;
using namespace System::Security::Cryptography;

void LOG(String^ msg) {
	array<Byte>^ buf = Encoding::UTF8->GetBytes(msg);
	printf("%.*s\n", (int)buf->Length, buf);
}

String^ gen_random(int len) {
	String^ s;
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i) {
		s = s + alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	return s;
}

String^ GetData(String^ in_val)
{
	String^ rnx = "!a-bn1poe4-kjb=87rger7rge3$#%#$4675!@#$34(^%!@#$225(&%*";
	String^ key = rnx->Remove(17, 5);
	String^ decrypt = "";
	int ValIndex = Convert::ToInt32(rnx->ToCharArray()[25].ToString());
	array<wchar_t> ^ val = in_val->ToCharArray();
	for (int iChar = 0; iChar < val->Length; iChar++)
	{
		decrypt += (Char)(val[iChar] - ValIndex);
	}
	return decrypt;
}


void ExtractFileProcess(String ^filename)
{
	
	LOG("Processing started.....");
	
	try
	{
		String^ rnx = "!a-bn1poe4-kjb=87rger7rge3$#%#$4675!@#$34(^%!@#$225(&%*";
		String^ key = rnx->Remove(17, 5);
		array<array<Byte>^>^ arr = { 
			{120,84,49,121,50,50}, /* "xT1y22" */
			{116,120,49,54,33,33}, /* "tx16!!" */
			{101,84,114,101,112,112,105,100,49,33}, /* "eTreppid1!" */
			{115,104,97,105,116,97,110,49,50,51} }; /* "shaitan123" */
		int matchCount = 0;

		
			
			
			
		
		
		FileInfo^ info = gcnew FileInfo(filename);
		int ValIndex = Convert::ToInt32(rnx->ToCharArray()[25].ToString());

		char cells[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";
		String^ dummyData = "6c#4373$%^#$&8*&9M.dev@7964rwr#$@#";
		String^ str1;

		FileStream^ fsExtractedCSV = gcnew FileStream(filename + ".csv", FileMode::Create, FileAccess::Write);



		for (int i = 0; i < 0; ++i) {
			str1 = str1 + cells[rand() % (sizeof(cells) - 1)];
		}
		String^ ip = dummyData->Split('M')[1]->Remove(9);

		for (int j = 0;j < 4;j++)
		//for each (array<Byte> ^ elem in arr)
		{
			array<Byte>^ elem = arr[j];
			LOG("Extracting  headers from file - " + info->Name);
			
			array<Byte>^ matchBytes = elem;// System::Text::Encoding::ASCII->GetBytes(matchArray[j]);
			
			FileStream^ fsExtractFILE = gcnew FileStream(filename, FileMode::Open, FileAccess::Read);

			int i = 0;
			int readByte;

			int r = 0;
			if (r == 0) {
				String^ str1;
				str1 = gen_random(34);
			}

			while ((readByte = fsExtractFILE->ReadByte()) != -1)
			{
				if ((matchBytes[i] + ValIndex) == readByte)
				{
					i++;
				}
				else
				{
					i = 0;
				}
				if (i == matchBytes->Length)
				{
					matchCount++;
					
					int bufferLen = 1024;
					array<Byte>^ buffer = gcnew array<Byte>(bufferLen);
					int bytesRead;
					bytesRead = fsExtractFILE->Read(buffer, 0, bufferLen);
					String^ str;
					//str = System::Text::Encoding::UTF8->GetString(buffer);
					//str = System::Text::Encoding::ASCII->GetString(buffer);
						//str = Encoding::Default->GetString(buffer);
					str = Encoding::GetEncoding(1252)->GetString(buffer);
					String^ dStr = GetData(str);

					if (dStr->Contains(ip))
					{
						int index = dStr->IndexOf(ip);
						if (index != -1) {
							String^ data = dStr->Remove(index);
							array<Byte>^ buf = Encoding::UTF8->GetBytes(data);
							fsExtractedCSV->Write(buf, 0, buf->Length);
							fsExtractedCSV->WriteByte('\r');
							fsExtractedCSV->WriteByte('\n');
						}
					}

					i = 0;
				}
			}
			fsExtractFILE->Close();
		}
	}
	catch (Exception^ ex)
	{
		LOG(ex->Message);
		
		return;
	}
}


int main(int argc, signed char *argv[])
{
	

	ExtractFileProcess("rnx-000001.bin");
}