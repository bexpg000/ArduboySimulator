#include "PlatformPrecomp.h"
#include "ResourceUtils.h"
#include "MiscUtils.h"

#ifndef C_NO_ZLIB
#include <zlib.h>
#endif
 
bool IsPowerOf2(int n) { return (!(n & (n - 1))); }

bool SaveToFile(const string &str, FILE *fp)
{
	int size = (int)str.size();

	fwrite(&size, sizeof(int), 1, fp);
	if (size> 0)
	{
		fwrite(str.c_str(), size, 1, fp);
	}
	return true;
}

bool LoadFromFile(string &str, FILE *fp)
{
	int size;
	fread(&size, sizeof(int), 1, fp);

	if (size > 0)
	{
		str.resize(size,' ');
		fread(&str[0], size, 1, fp);
	} else
	{
		str.clear();
	}

	return true;
}


bool SaveToFile(int num, FILE *fp)
{
	fwrite(&num, sizeof(int), 1, fp);
	return true;
}

bool SaveToFile(uint32 num, FILE *fp)
{
	fwrite(&num, sizeof(uint32), 1, fp);
	return true;
}

bool SaveToFile(float num, FILE *fp)
{
	fwrite(&num, sizeof(float), 1, fp);
	return true;
}

bool LoadFromFile(int &num, FILE *fp)
{
	fread(&num, sizeof(int), 1, fp);
	return true;
}
bool LoadFromFile(float &num, FILE *fp)
{
	fread(&num, sizeof(float), 1, fp);
	return true;
}

bool LoadFromFile(bool &num, FILE *fp)
{
	fread(&num, sizeof(bool), 1, fp);
	return true;
}
bool LoadFromFile(uint32 &num, FILE *fp)
{
	fread(&num, sizeof(uint32), 1, fp);
	return true;
}

#ifndef CLANLIB_1
bool LoadFromFile(CL_Vec2f &num, FILE *fp)
{
	fread(&num, sizeof(CL_Vec2f), 1, fp);
	return true;
}

bool LoadFromFile(CL_Vec3f &num, FILE *fp)
{
	fread(&num, sizeof(CL_Vec3f), 1, fp);
	return true;
}

bool LoadFromFile(CL_Rectf &num, FILE *fp)
{
	fread(&num, sizeof(CL_Rectf), 1, fp);
	return true;
}
#endif


bool FileExists(const string &fName)
{

//this is so my RTPacker command line util will compile using this even though it doesn't use the filemanager

#if !defined(CLANLIB_1) && !defined(_CONSOLE)
	if (GetFileManager())
	{
		return GetFileManager()->FileExists(fName, false);
	} 

#endif
	FILE *fp = fopen( (fName).c_str(), "rb");
	if (!fp)
	{
		//file not found	
		return NULL;
	}

	fclose(fp);
	return true;

}

//up to you to use SAFE_DELETE_ARRAY
byte * DecompressRTPackToMemory(byte *pMem, unsigned int *pDecompressedSize)
{
	assert(IsAPackedFile(pMem));

#ifdef C_NO_ZLIB

	assert(!"Can't decompress, zlib disabled with C_NO_ZLIB flag");
return NULL;

#else
	rtpack_header *pHeader = (rtpack_header*)pMem;
	byte *pDeCompressed = zLibInflateToMemory( pMem+sizeof(rtpack_header), pHeader->compressedSize, pHeader->decompressedSize);

	if (pDecompressedSize)
	{
		*pDecompressedSize = pHeader->decompressedSize;
	}
	return pDeCompressed;
#endif
}


#ifndef UINT_MAX
	//fix problem for webOS compile
	#define UINT_MAX      0xffffffff
#endif
byte * LoadFileIntoMemoryBasic(string fileName, unsigned int *length, bool bUseSavePath, bool bAddBasePath)
{
	*length = 0;

	if (bAddBasePath)
	{
		if (bUseSavePath)
		{
			fileName = GetSavePath() + fileName;
		} else
		{
			fileName = GetBaseAppPath() + fileName;
		}
	}
	
	FILE *fp = fopen(fileName.c_str(), "rb");
	if (!fp)
	{
		//file not found	
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	*length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	byte *pData = new byte[(*length) +1];
	
	if (!pData)
	{
		fclose(fp);
		*length = UINT_MAX; //signal a mem error
		return NULL;
	}
	pData[*length] = 0; 
	fread(pData, *length, 1, fp);
	fclose(fp);

	//we add an extra null at the end to be nice, when loading text files this can be useful

	return pData;
}

bool IsAPackedFile(byte *pFile)
{
	return (strncmp((char*)pFile, C_RTFILE_PACKAGE_HEADER, 6) == 0);
}

bool IsARTFile(byte *pFile)
{
	//not so safe... so don't count on this
	return (strncmp((char*)pFile, "RT", 2) == 0);
}

bool CompressFile(string fName)
{
	unsigned int size;
	byte *pInput = LoadFileIntoMemoryBasic(fName, &size); //the basic means don't try to decompress it

	if (IsAPackedFile(pInput))
	{
		SAFE_DELETE_ARRAY(pInput);
		LogMsg("%s is already packed, ignoring.", fName.c_str());
		return true; //well, no error at least
	}

	LogMsg("Compressing %s..", fName.c_str());

#ifdef C_NO_ZLIB

	assert(!"ZLIB disabled with C_NO_ZLIB flag, no can do, sir");
	return false;
#else
	int compressedSize; 
	byte *pCompressedFile = zlibDeflateToMemory(pInput, size, &compressedSize);
	SAFE_DELETE_ARRAY(pInput); //done with that part

	rtpack_header header =  BuildRTPackHeader(size, compressedSize);

	string finalFilename = fName;
	string ext = GetFileExtension(fName);

	if (ext != "rtfont" && ext != "rttex" && ext != "rtpak")
	{
		finalFilename = ModifyFileExtension(fName, "rtpak");
	}

	//save out the real file
	FILE *fp = fopen(finalFilename.c_str(), "wb");
	fwrite(&header, sizeof(rtpack_header), 1, fp);
	fwrite(pCompressedFile, compressedSize, 1, fp);
	fclose(fp);

	int totalBytes = sizeof(rtpack_header) + compressedSize;
	LogMsg("Compressed to %s.  (%d kb, %.0f%%%%)", finalFilename.c_str(),totalBytes/1024,  100*float(totalBytes)/float(size) );
#endif
	return true;
}

rtpack_header BuildRTPackHeader(int size, int compressedSize)
{
	rtpack_header header;
	memset(&header, 0, sizeof(rtpack_header));

	header.compressedSize = compressedSize;
	header.decompressedSize = size;
	header.compressionType = C_COMPRESSION_ZLIB;
	memcpy(header.rtFileHeader.fileTypeID, C_RTFILE_PACKAGE_HEADER, 6);
	header.rtFileHeader.version = C_RTFILE_PACKAGE_LATEST_VERSION;
	return header;
}

#ifndef C_NO_ZLIB


//you must SAFE_FREE what it returns
byte * zlibDeflateToMemory(byte *pInput, int sizeBytes, int *pSizeCompressedOut)
{
	z_stream strm;
	int ret;

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
	if (ret != Z_OK)
		return 0;


#define ZLIB_PADDING_BYTES (1024*5)

	byte *pOut = (byte*) new byte[sizeBytes + ZLIB_PADDING_BYTES];  //some extra padding in case the compressed version is larger than the decompressed version for some reason
	if (!pOut) return 0;
	strm.avail_in = sizeBytes;
	strm.next_in = pInput;
	strm.avail_out = sizeBytes + ZLIB_PADDING_BYTES;
	strm.next_out = pOut;

	ret = deflate(&strm, Z_FINISH);
	assert(ret != Z_STREAM_ERROR);  /* state not clobbered */

	//	assert(ret == Z_STREAM_END);
	deflateEnd(&strm);
	*pSizeCompressedOut = strm.total_out;
	return pOut;

}

//you must SAFE_FREE what it returns
byte * zLibInflateToMemory(byte *pInput, unsigned int compressedSize, unsigned int decompressedSize)
{
	int ret;
	z_stream strm;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
		return 0;
	byte *pDestBuff = new byte[decompressedSize+1]; //room for extra null at the end;
	if (!pDestBuff)
	{
		return 0;
	}
	pDestBuff[decompressedSize] = 0; //add the extra null, if we decompressed a text file this can be useful
	strm.avail_in = compressedSize;
	strm.next_in = pInput;
	strm.avail_out = decompressedSize;
	strm.next_out = pDestBuff;
	
	ret = inflate(&strm, Z_NO_FLUSH);
	if (! (ret == Z_OK || ret == Z_STREAM_END))
	{
		SAFE_DELETE_ARRAY(pDestBuff);
		return 0;
	}

	(void)inflateEnd(&strm);

	return pDestBuff;
}

#endif

//You must SAFE_FREE the pointer it returns at some point!
byte * LoadFileIntoMemory(string fileName, unsigned int *p_ui_size, bool bUseSavePath)
{
	assert(p_ui_size && "You need to send in a valid int to be filled with the size, not a NULL.");

	byte *p_resource = LoadFileIntoMemoryBasic(fileName, p_ui_size, bUseSavePath);

	if (!p_resource)
	{
		return 0; //out of memory or something
	}

	return p_resource;
}

string SeparateStringSTL(string input, int index, char delimiter)
{
	//yes, this is pretty crap
	char stInput[255];
	if (SeparateString(input.c_str(), index, delimiter, stInput))
	{
		return stInput;
	} 
	LogError("SeparateStringSTL unable to find delimiter");
	return "";
}

bool SeparateString (const char str[], int num, char delimiter, char *return1) 
{
	int l = 0;
	return1[0] = 0;

	for (unsigned int k = 0; str[k] != 0; k++)
	{
		if (str[k] == delimiter)
		{
			l++;
			if (l == num+1)
				break;

			if (k < strlen(str)) strcpy(return1,"");
		}
		if (str[k] != delimiter)
			sprintf(return1, "%s%c",return1 ,str[k]);
	}

	if (l < num)
	{
		return1[0] = 0;
		return(false);
	}
	return true;
}

//snippet from Zahlman's post on gamedev:  http://www.gamedev.net/community/forums/topic.asp?topic_id=372125
void StringReplace(const std::string& what, const std::string& with, std::string& in)
{
	size_t pos = 0;
	size_t whatLen = what.length();
	size_t withLen = with.length();
	while ((pos = in.find(what, pos)) != std::string::npos)
	{
		in.replace(pos, whatLen, with);
		pos += withLen;
	}
}

int GetFileSize(const string &fName)
{
	FILE * file;
	int fileSizeBytes = 0;
	file = fopen(fName.c_str(),"r");
	if(file > 0)
	{
		fseek(file, 0, SEEK_END);
		fileSizeBytes = ftell(file);
		fseek(file, 0, SEEK_SET);
		fclose(file);
	} else
	{
		LogMsg("Unable to open %s to get file size", fName.c_str());
	}
	return fileSizeBytes;
}

//add ipad to the filename if needed

string AddIPADToFileName(string file)
{
	
	if (!IsLargeScreen()) return file;
	size_t index = file.find_last_of('.');
	if (index == string::npos)
	{
		assert(!"Well, it doesn't have an extension to begin with");
		return file;
	}

	return file.substr(0, index) + "_ipad."+file.substr(index+1, file.length()-index);	
}

//replace lowercase iphone with ipad in string if needed
string ReplaceWithDeviceNameInFileName(const string &fName)
{
	if (IsIPADSize)
	{
		string final = fName;
		StringReplace("iphone", "ipad", final);
		return final;
	}
	
	if (IsIphone4Size)
	{
		string final = fName;
		StringReplace("iphone", "iphone4", final);
		return final;
	}

	return fName; //no change
}

string ReplaceWithLargeInFileName(const string &fName)
{
	if (!IsLargeScreen())
	{
		return fName; //no conversation done
	}

	string final = fName;
	StringReplace("iphone", "large", final);
	return final;
}

string ReplaceWithLargeInFileNameAndOSSpecific(const string &fName)
{
	if (!IsLargeScreen())
	{
		return fName; //no conversation done
	}

	string final = fName;
	
	if (GetEmulatedPlatformID() == PLATFORM_ID_WINDOWS
		|| GetEmulatedPlatformID() == PLATFORM_ID_OSX)
	{
		StringReplace("iphone", "win", final);

	} else
	{
		//default, just large
		StringReplace("iphone", "large", final);
	}
	return final;
}

string ReplaceMP3( const string &fName)
{
	if (GetEmulatedPlatformID() != PLATFORM_ID_ANDROID)
	{
		return fName; //leave it as mp3
	}
	
	string final = fName;

	StringReplace("mp3", "ogg", final);
	return final;
}