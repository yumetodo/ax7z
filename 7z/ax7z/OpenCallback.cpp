// OpenCallback.cpp
#include <windows.h>
#include "OpenCallback.h"
#include "PasswordManager.h"

#include "../../3rdparty/7z/CPP/Common/StdOutStream.h"
#include "../../3rdparty/7z/CPP/Common/StdInStream.h"
#include "../../3rdparty/7z/CPP/Common/StringConvert.h"

#include "../../3rdparty/7z/CPP/7zip/Common/FileStreams.h"

#include "../../3rdparty/7z/CPP/Windows/PropVariant.h"

STDMETHODIMP COpenCallbackImp2::SetTotal(const UINT64 *files, const UINT64 *bytes)
{
    return S_OK;
}

STDMETHODIMP COpenCallbackImp2::SetCompleted(const UINT64 *files, const UINT64 *bytes)
{
    return S_OK;
}

STDMETHODIMP COpenCallbackImp2::GetProperty(PROPID propID, PROPVARIANT *value)
{
    NWindows::NCOM::CPropVariant propVariant;
    switch(propID)
    {
    case kpidName:
        propVariant = _fileInfo.Name;
        break;
    case kpidIsFolder:
        propVariant = _fileInfo.IsDir();
        break;
    case kpidSize:
        propVariant = _fileInfo.Size;
        break;
    case kpidAttributes:
        propVariant = (UINT32)_fileInfo.Attributes;
        break;
    case kpidLastAccessTime:
        propVariant = _fileInfo.LastAccessTime;
        break;
    case kpidCreationTime:
        propVariant = _fileInfo.CreationTime;
        break;
    case kpidLastWriteTime:
        propVariant = _fileInfo.LastWriteTime;
        break;
    }
    propVariant.Detach(value);
    return S_OK;
}

STDMETHODIMP COpenCallbackImp2::GetStream(const wchar_t *name, 
                                         IInStream **inStream)
{
    *inStream = NULL;
    UString fullPath = _folderPrefix + name;
    if (!_fileInfo.Find(fullPath) || _fileInfo.IsDir())
        return S_FALSE;
    CInFileStream *inFile = new CInFileStream;
    CMyComPtr<IInStream> inStreamTemp = inFile;
    if (!inFile->Open(fullPath))
        return ::GetLastError();
    *inStream = inStreamTemp.Detach();
    return S_OK;
}

STDMETHODIMP COpenCallbackImp2::CryptoGetTextPassword(BSTR *password)
{
	UString usPassword = PasswordManager::Get().GetPassword(true);
	CMyComBSTR temp(usPassword);
    *password = temp.Detach();

    return S_OK;
}

