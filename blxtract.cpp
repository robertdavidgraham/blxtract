/*
*  blxtract -- extracts CSVs from Dennis Montgomery's BLX format.
* 
* To build this, you need "Microsoft Visual Studio 2022". The free "Community"
* edition works just fine. You need to configure the /CLR option.
*  - Create default C++ command-line project.
*  - Advanced -- C++/CLI Properties -- Common Language Runtime Support: Common Language Runtime Support (/clr)
*  - C/C++ -- All Options -- Additional Options: /Zc:twoPhase- %(AdditionalOptions)
* 
*/
#include <iostream>
#include <string>

using namespace System;
using namespace System::Text;
using namespace System::IO;

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

String^ ROT3(String^ in_val)
{
	String^ decrypt = "";
	array<wchar_t> ^ val = in_val->ToCharArray();

	for (int i = 0; i < val->Length; i++)
		decrypt += (Char)(val[i] - 3);

	return decrypt;
}


void ExtractFileProcess(String ^filename)
{
	
	LOG("Processing started.....");
	
	try
	{
		/* Four strings that mark the start of data */
		array<array<Byte>^>^ arr = { 
			{120,84,49,121,50,50}, /* "xT1y22" */
			{116,120,49,54,33,33}, /* "tx16!!" */
			{101,84,114,101,112,112,105,100,49,33}, /* "eTreppid1!" */
			{115,104,97,105,116,97,110,49,50,51} }; /* "shaitan123" */

		/* output file , hard coded name for now */
		FileStream^ fsExtractedCSV = gcnew FileStream(filename + ".csv", FileMode::Create, FileAccess::Write);

		/* End-of-record delimeter of a 1024-byte record, stripping everything off after this point */
		String^ ip = ".dev@7964";
		
		/* Read the file 4 times, each time searching for a different
		* start-of-record delimiter */
		for (int j = 0; j < 4; j++) {
			LOG("Extracting  headers from file - " + filename);

			/* The start-of-record delimeter to search for */
			array<Byte>^ matchBytes = arr[j];

			/**/
			FileStream^ fsExtractFILE = gcnew FileStream(filename, FileMode::Open, FileAccess::Read);

			int i = 0;
			int readByte;

			

			while ((readByte = fsExtractFILE->ReadByte()) != -1)
			{
				if ((matchBytes[i] + 3) == readByte)
				{
					i++;
				}
				else
				{
					i = 0;
				}
				if (i != matchBytes->Length)
					continue;

					
				int bufferLen = 1024;
				array<Byte>^ buffer = gcnew array<Byte>(bufferLen);
				int bytesRead;
				bytesRead = fsExtractFILE->Read(buffer, 0, bufferLen);
				String^ str;
				str = Encoding::GetEncoding(1252)->GetString(buffer);
				String^ dStr = ROT3(str);

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