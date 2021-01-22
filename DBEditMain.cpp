#include <windows.h>
#define STRICT_TYPED_ITEMIDS
#include <shlobj.h>
#include <objbase.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <knownfolders.h>
#include <propvarutil.h> 
#include <propkey.h>
#include <propidl.h>
#include <strsafe.h>
#include <shtypes.h> 
#include <new>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <vector>

#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

const COMDLG_FILTERSPEC c_rgSaveTypes[] =
{
	{L"PY FILE (*.py)",       L"*.py"},
};

#include <stdio.h>  /* defines FILENAME_MAX */
#include <direct.h>
#define GetCurrentDir _getcwd
std::string GetCurrentWorkingDir(void) {
	char buff[FILENAME_MAX];
	GetCurrentDir(buff, FILENAME_MAX);
	std::string current_working_dir(buff);
	return current_working_dir;
}

const std::string toolPath = GetCurrentWorkingDir() + "\\Tools\\";
std::string dumpPath;
std::string parsePath;
std::string pakPath;

//Verifies whether the file is retrievable.
bool VerifyFileOrFolder(std::string filePath = "")
{
	std::string debugS;
	std::string testPath = GetCurrentWorkingDir()+ "\\" + filePath;
	DWORD ftyp = GetFileAttributesA(filePath.c_str());
	if (ftyp != INVALID_FILE_ATTRIBUTES)
	{
		debugS = "File found - "+ testPath + "\n";
		OutputDebugString(debugS.c_str());
		return true;
	}
	debugS = "No file found - " + testPath + "\n";
	OutputDebugString(debugS.c_str());
	return false;
}

void FixParser() 
{
	std::string testPath = GetCurrentWorkingDir() + "\\Tools\\bbtools-master\\dbfz_script_parser.py";
	std::string pyFile;
	if (VerifyFileOrFolder(testPath))
	{
		std::ifstream ipf;
		ipf.open(testPath);
		if (!ipf.is_open())
		{
			std::string debugs = "Cannot Open File load" + testPath;
			OutputDebugString(debugs.c_str());
		}
		else
		{
			for (std::string line; std::getline(ipf, line);)
			{
				std::istringstream in(line);
				std::string type;
				in >> type;
				if (type == "mypath")
					line = "mypath = \"" + parsePath + "\"";
				pyFile+= line + "\n";
			}
			ipf.close();
		}
		std::ofstream opf;
		opf.open(testPath, std::ofstream::trunc);
		opf.close();
		opf.open(testPath, std::ofstream::app);
		if (!opf.is_open())
		{
			std::string debugs = "Cannot Open File save" + testPath;
			OutputDebugString(debugs.c_str());
		}
		else
		{
			opf << pyFile;
			opf.close();
		}
	}
}

//Verifies whether the file is retrievable.
bool VerifyTools()
{
	std::string debugS;
	std::string testPath;
	for (int i = 0; i < 3; i++)
	{
		testPath = "Tools\\";
		if (i == 1)
			testPath += "UnrealPak\\UnrealPak-With-Compression.bat";
		if (i == 0) 
		{
			testPath += "bbtools-master\\dbfz_script_parser.py";
			if(parsePath != "")
				FixParser();
		}
		if (i == 2)
			testPath += "bbtools-master\\dbfz_script_rebuilder.py";
		if (VerifyFileOrFolder(testPath))
			continue;
		else
			return false;
	}
	OutputDebugString("Alls right in the world \n");

	return true;
}
int GetFileSize(std::string filename)
{
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

//1 = dump, 2 = parse, 3 = gamePak
void SaveFolderPaths() 
{
	std::ofstream op;
	op.open(GetCurrentWorkingDir() + "\\fp.dll");
	if (!op.is_open())
	{
		std::string debugs = "Cannot Open File" + GetCurrentWorkingDir() + "\\fp.dll";
		OutputDebugString(debugs.c_str());
	}
	else
	{
		op << "1 " + dumpPath + "\n2 " + parsePath + "\n3 " + pakPath;
	}
	op.close();
}

//
//void SaveActiveFile(FileType fileType, std::string fileName, std::string data)
//{
//	std::string filePath = FetchPath(fileType, fileName, true);
//	WriteDebug(filePath);
//	std::ofstream file;
//	file.open(filePath + ".txt");
//	file << data;
//	file.close();
//}

void LoadFolderPaths() 
{
	if (VerifyFileOrFolder("fp.dll")) 
	{
		std::ifstream op;
		op.open(GetCurrentWorkingDir() + "\\fp.dll");
		if (!op.is_open())
		{
			std::string debugs = "Cannot Open File" + GetCurrentWorkingDir() + "\\fp.dll";
			OutputDebugString(debugs.c_str());
		}
		else
		{
			for (std::string line; std::getline(op, line);)
			{
				std::istringstream in(line);
				std::string type;
				in >> type;
				if (type == "1")
					in >> dumpPath;
				if (type == "2") 
				{
					in >> parsePath;
				}
				if (type == "3")
					in >> pakPath;
			}
			op.close();
		}
	}
}

// Indices of file types
#define INDEX_PY 1

// IDs for the Task Dialog Buttons
#define DBE_SelectDumpFolder                    100
#define DBE_ParseCharaFile						101
#define DBE_RebuildCharaFile					102
#define DBE_PakFiles							103

#pragma region CDialog
class CDialogEventHandler : public IFileDialogEvents,
	public IFileDialogControlEvents
{
public:
	// IUnknown methods
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] = {
			QITABENT(CDialogEventHandler, IFileDialogEvents),
			QITABENT(CDialogEventHandler, IFileDialogControlEvents),
			{ 0 },
#pragma warning(suppress:4838)
		};
		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		long cRef = InterlockedDecrement(&_cRef);
		if (!cRef)
			delete this;
		return cRef;
	}

	// IFileDialogEvents methods
	IFACEMETHODIMP OnFileOk(IFileDialog*) { return S_OK; };
	IFACEMETHODIMP OnFolderChange(IFileDialog*) { return S_OK; };
	IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; };
	IFACEMETHODIMP OnHelp(IFileDialog*) { return S_OK; };
	IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; };
	IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; };
	IFACEMETHODIMP OnTypeChange(IFileDialog* pfd);
	IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; };

	// IFileDialogControlEvents methods
	IFACEMETHODIMP OnItemSelected(IFileDialogCustomize* pfdc, DWORD dwIDCtl, DWORD dwIDItem);
	IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*, DWORD) { return S_OK; };
	IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD, BOOL) { return S_OK; };
	IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*, DWORD) { return S_OK; };

	CDialogEventHandler() : _cRef(1) { };
private:
	~CDialogEventHandler() { };
	long _cRef;
};

HRESULT CDialogEventHandler::OnTypeChange(IFileDialog* pfd)
{
	IFileSaveDialog* pfsd;
	HRESULT hr = pfd->QueryInterface(&pfsd);
	if (SUCCEEDED(hr))
	{
		UINT uIndex;
		hr = pfsd->GetFileTypeIndex(&uIndex);   // index of current file-type
		if (SUCCEEDED(hr))
		{
			IPropertyDescriptionList* pdl = NULL;

			switch (uIndex)
			{
			case INDEX_PY:
				// When .doc is selected, let's ask for some arbitrary property, say Title.
				hr = PSGetPropertyDescriptionListFromString(L"prop:System.Title", IID_PPV_ARGS(&pdl));
				if (SUCCEEDED(hr))
				{
					// FALSE as second param == do not show default properties.
					hr = pfsd->SetCollectedProperties(pdl, FALSE);
					pdl->Release();
				}
				break;
			}
		}
		pfsd->Release();
	}
	return hr;
}
HRESULT CDialogEventHandler::OnItemSelected(IFileDialogCustomize* pfdc, DWORD dwIDCtl, DWORD dwIDItem)
{
	IFileDialog* pfd = NULL;
	HRESULT hr = pfdc->QueryInterface(&pfd);
	if (SUCCEEDED(hr))
	{
		pfd->Release();
	}
	return hr;
}
HRESULT CDialogEventHandler_CreateInstance(REFIID riid, void** ppv)
{
	*ppv = NULL;
	CDialogEventHandler* pDialogEventHandler = new (std::nothrow) CDialogEventHandler();
	HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
	if (SUCCEEDED(hr))
	{
		hr = pDialogEventHandler->QueryInterface(riid, ppv);
		pDialogEventHandler->Release();
	}
	return hr;
}
#pragma endregion

HRESULT FileOpenSelect()
{
	// CoCreate the File Open Dialog object.
	IFileDialog* pfd = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
	if (SUCCEEDED(hr))
	{
		// Create an event handling object, and hook it up to the dialog.
		IFileDialogEvents* pfde = NULL;
		hr = CDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde));
		if (SUCCEEDED(hr))
		{
			// Hook up the event handler.
			DWORD dwCookie;
			hr = pfd->Advise(pfde, &dwCookie);
			if (SUCCEEDED(hr))
			{
				// Set the options on the dialog.
				DWORD dwFlags;

				// Before setting, always get the options first in order not to override existing options.
				hr = pfd->GetOptions(&dwFlags);
				if (SUCCEEDED(hr))
				{
					// In this case, get shell items only for file system items.
					hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
					if (SUCCEEDED(hr))
					{
						// Set the file types to display only. Notice that, this is a 1-based array.
						hr = pfd->SetFileTypes(ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
						if (SUCCEEDED(hr))
						{
							// Set the selected file type index to Word Docs for this example.
							hr = pfd->SetFileTypeIndex(INDEX_PY);
							if (SUCCEEDED(hr))
							{
								// Set the default extension to be ".doc" file.
								hr = pfd->SetDefaultExtension(L"doc");
								if (SUCCEEDED(hr))
								{
									// Show the dialog
									hr = pfd->Show(NULL);
									if (SUCCEEDED(hr))
									{
										// Obtain the result, once the user clicks the 'Open' button.
										// The result is an IShellItem object.
										IShellItem* psiResult;
										hr = pfd->GetResult(&psiResult);
										if (SUCCEEDED(hr))
										{
											// We are just going to print out the name of the file for sample sake.
											PWSTR pszFilePath = NULL;
											hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
											if (SUCCEEDED(hr))
											{
												TaskDialog(NULL,
													NULL,
													L"CommonFileDialogApp",
													pszFilePath,
													NULL,
													TDCBF_OK_BUTTON,
													TD_INFORMATION_ICON,
													NULL);
												CoTaskMemFree(pszFilePath);
											}
											psiResult->Release();
										}
									}
								}
							}
						}
					}
				}
				// Unhook the event handler.
				pfd->Unadvise(dwCookie);
			}
			pfde->Release();
		}
		pfd->Release();
	}
	return hr;
}

HRESULT FolderSelect(int i)
{
	// CoCreate the File Open Dialog object.
	IFileDialog* pfd = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
	if (SUCCEEDED(hr))
	{
		// Create an event handling object, and hook it up to the dialog.
		IFileDialogEvents* pfde = NULL;
		hr = CDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde));
		DWORD dwCookie;
		hr = pfd->Advise(pfde, &dwCookie);
		hr = pfd->SetOptions(FOS_PICKFOLDERS);
		hr = pfd->Show(NULL);

		if (SUCCEEDED(hr))
		{
			IShellItem* psiResult;
			hr = pfd->GetResult(&psiResult);
			if (SUCCEEDED(hr))
			{
				PWSTR pszFilePath = NULL;
				hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
				if (pszFilePath != NULL) 
				{
					std::wstring wstr(pszFilePath);
					using convert_type = std::codecvt_utf8<wchar_t>;
					std::wstring_convert<convert_type, wchar_t> converter;
					std::string str = converter.to_bytes(wstr);
					if (i == 1) { dumpPath = str; }
					if (i == 2) { parsePath = str; }
					if (i == 3) { pakPath = str; }
					SaveFolderPaths();
				}

				psiResult->Release();
			}
		}
		pfd->Unadvise(dwCookie);
		pfde->Release();
		pfd->Release();
	}
	return hr;
}

void Parse() 
{
	if (dumpPath == "") 
	{
		OutputDebugString("No game dump selected");
	}
	if (parsePath == "")
	{
		OutputDebugString("No output folder selected");
	}
	OutputDebugString("Parse Possible");
}

void Rebuild()
{
	if (dumpPath == "")
	{
		OutputDebugString("No game dump selected");
	}
	if (parsePath == "")
	{
		OutputDebugString("No output folder selected");
	}
	if (pakPath == "") 
	{
		OutputDebugString("No pak folder selected");
	}
	OutputDebugString("Rebuild Possible");

}



// Application entry point
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
	LoadFolderPaths();
	VerifyTools();


	//Parse();
	//Rebuild();
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{

		TASKDIALOGCONFIG taskDialogParams = { sizeof(taskDialogParams) };
		taskDialogParams.dwFlags = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;

		TASKDIALOG_BUTTON const buttons[] =
		{
			{ DBE_SelectDumpFolder,						L"Select - Umodel Dump of DBFZ" },
			{ DBE_ParseCharaFile,						L"Parse - DBFZ Character Data" },
			{ DBE_RebuildCharaFile,						L"Rebuild - Edited Character Data" },
			{ DBE_PakFiles,								L"Pak - File Folder" },
		};

		taskDialogParams.pButtons = buttons;
		taskDialogParams.cButtons = ARRAYSIZE(buttons);
		taskDialogParams.pszMainInstruction = L"Pick the file dialog sample you want to try";
		taskDialogParams.pszWindowTitle = L"DB Edit Wizard";

		while (SUCCEEDED(hr))
		{
			int selectedId;
			hr = TaskDialogIndirect(&taskDialogParams, &selectedId, NULL, NULL);
			if (SUCCEEDED(hr))
			{
				if (selectedId == IDCANCEL)
				{
					break;
				}
				else if (selectedId == DBE_SelectDumpFolder)
				{
					FolderSelect(1);
				}
				else if (selectedId == DBE_ParseCharaFile)
				{
					FolderSelect(2);
					FixParser();
				}
				else if (selectedId == DBE_RebuildCharaFile)
				{
					FolderSelect(3);
				}
				else if (selectedId == DBE_PakFiles)
				{
					FolderSelect(3);
				}
			}
		}
		CoUninitialize();
	}
	return 0;
}