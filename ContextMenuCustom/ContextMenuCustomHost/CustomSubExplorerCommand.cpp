#include "pch.h"
#include "CustomSubExplorerCommand.h"
#include "PathHelper.hpp"
#include <regex>

using namespace winrt::Windows::Data::Json;

CustomSubExplorerCommand::CustomSubExplorerCommand(const winrt::hstring& configContent, ThemeType themeType)
{
	m_theme_type = themeType;
	try
	{
		const JsonObject result = JsonObject::Parse(configContent);
		_title = result.GetNamedString(L"title", L"Custom Menu");
		_exe = result.GetNamedString(L"exe", L"");
		_param = result.GetNamedString(L"param", L"");
		_icon = result.GetNamedString(L"icon", L"");
		_iconDark = result.GetNamedString(L"iconDark", L"");
		m_index = static_cast<int>(result.GetNamedNumber(L"index", 0));

		_accept_directory = result.GetNamedBoolean(L"acceptDirectory", false);
		_accept_directory_flag = static_cast<int>(result.GetNamedNumber(L"acceptDirectoryFlag", 0));

		_accept_file = result.GetNamedBoolean(L"acceptFile", false); //v3.6, next to remove
		_accept_file_flag = static_cast<int>(result.GetNamedNumber(L"acceptFileFlag", 0));
		_accept_file_regex = result.GetNamedString(L"acceptFileRegex", L"");
		_accept_exts = result.GetNamedString(L"acceptExts", L"");

		_accept_multiple_files_flag = static_cast<int>(result.GetNamedNumber(L"acceptMultipleFilesFlag", 0));
		_path_delimiter = result.GetNamedString(L"pathDelimiter", L"");
		_param_for_multiple_files = result.GetNamedString(L"paramForMultipleFiles", L"");

		//
		if (_accept_file_flag == 0 && _accept_file) {
			_accept_file_flag = FILE_EXT;
		}
		if (_accept_directory_flag == 0 && _accept_directory) {
			_accept_directory_flag = DIRECTORY_DIRECTORY | DIRECTORY_BACKGROUND | DIRECTORY_DESKTOP;
		}
	}
	catch (winrt::hresult_error const& e)
	{
		_title = _title + L" (config parse error)";
		OutputDebugStringW(std::format(L"CustomSubExplorerCommand::CustomSubExplorerCommand parse error,message={}, json={}", e.message(), configContent).c_str());
	}
}

bool CustomSubExplorerCommand::Accept(bool multipleFiles, FileType fileType, const std::wstring& name, const std::wstring& ext) {
	OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Accept menu={}, isMultipleFiles={}, fileType={}, fileName={}, fileExt={}", _title, multipleFiles, static_cast<int>(fileType), name, ext).c_str());

	if (multipleFiles) {
		OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Accept multiple_files_flag={}", _accept_multiple_files_flag).c_str());
		return _accept_multiple_files_flag == FILES_JOIN || _accept_multiple_files_flag == FILES_EACH;
	}

	//file
	if (fileType == FileType::File) {
		if (_accept_file_flag == FileMatchFlagEnum::FILE_ALL) {
			OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Accept menu={},file=all", _title).c_str());
			return true;
		}
		else if (_accept_file_flag == FileMatchFlagEnum::FILE_EXT) {
			OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Accept menu={}, file=ext like, ext={}", _title, _accept_exts).c_str());
			if (ext.empty() || _accept_exts.empty()) {
				return true;
			}
			if (_accept_exts.find(L'*') != std::wstring::npos) {
				return true;
			}
			return _accept_exts.find(ext) != std::wstring::npos;
		}
		else if (_accept_file_flag == FileMatchFlagEnum::FILE_EXT2) {
			OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Accept menu={}, file=ext list, ext={}", _title, _accept_exts).c_str());
			if (ext.empty() || _accept_exts.empty()) {
				return false;
			}

			const size_t position = _accept_exts.find(ext);
			if (position != std::string::npos) {
				//check for .c .cpp
				const bool isStart = position == 0;
				const bool isEnd = position + ext.size() == _accept_exts.size();
				const bool isPrevComma = !isStart && _accept_exts[position - 1] == L'|';
				const bool isNextComma = !isEnd && _accept_exts[position + ext.size()] == L'|';
				if ((isStart || isPrevComma) && (isEnd || isNextComma)) {
					return true;
				}
			}
			return false;
		}
		else if (_accept_file_flag == FileMatchFlagEnum::FILE_REGEX) {
			OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Accept menu={}, file=regex, ext={}", _title, _accept_file_regex).c_str());
			if (_accept_file_regex.empty()) {
				return false;
			}

			const std::wregex fileRegex(_accept_file_regex);
			return std::regex_match(name, fileRegex);
		}
	}
	//directory
	else if (fileType == FileType::Directory) {
		OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Accept menu={}, directory=Directory", _title).c_str());
		return (_accept_directory_flag & DIRECTORY_DIRECTORY) == DIRECTORY_DIRECTORY;
	}
	//background
	else if (fileType == FileType::Background) {
		OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Accept menu={}, directory=Background", _title).c_str());
		return  (_accept_directory_flag & DIRECTORY_BACKGROUND) == DIRECTORY_BACKGROUND;
	}
	//desktop
	else if (fileType == FileType::Desktop) {
		OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Accept menu={}, directory=Desktop", _title).c_str());
		return  (_accept_directory_flag & DIRECTORY_DESKTOP) == DIRECTORY_DESKTOP;
	}

	OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Accept skip, menu={}", _title).c_str());
	return false;
}

IFACEMETHODIMP CustomSubExplorerCommand::GetIcon(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* icon) {
	*icon = nullptr;

	if (m_theme_type == ThemeType::Dark && !_iconDark.empty()) {
		auto iconPath = wil::make_cotaskmem_string_nothrow(_iconDark.c_str());
		RETURN_IF_NULL_ALLOC(iconPath);
		*icon = iconPath.release();
		return S_OK;
	}

	//TODO light or defalut
	if (!_icon.empty()) {
		auto iconPath = wil::make_cotaskmem_string_nothrow(_icon.c_str());
		RETURN_IF_NULL_ALLOC(iconPath);
		*icon = iconPath.release();
		return S_OK;
	}

	return BaseExplorerCommand::GetIcon(items, icon);
}

IFACEMETHODIMP CustomSubExplorerCommand::GetTitle(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* name) {
	*name = nullptr;
	auto title = wil::make_cotaskmem_string_nothrow(_title.c_str());
	RETURN_IF_NULL_ALLOC(title);
	*name = title.release();
	return S_OK;
}

IFACEMETHODIMP CustomSubExplorerCommand::GetState(_In_opt_ IShellItemArray* selection, _In_ BOOL okToBeSlow, _Out_ EXPCMDSTATE* cmdState) {
	*cmdState = ECS_ENABLED;
	return S_OK;
}

IFACEMETHODIMP CustomSubExplorerCommand::Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) noexcept try {
	OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Invoke , menu={}", _title).c_str());

	HWND parent = nullptr;
	if (m_site) {
		RETURN_IF_FAILED(IUnknown_GetWindow(m_site.get(), &parent));
	}

	DWORD count = 0;
	if (selection) {
		selection->GetCount(&count);
	}

	OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Invoke  menu={}, selection size={}", _title, count).c_str());

	if (count > 1 && _accept_multiple_files_flag == FILES_JOIN) {
		if (const auto paths = PathHelper::getPaths(selection, _path_delimiter); !paths.empty()) {
			OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Invoke menu={}, join, paths={}", _title, paths).c_str());

			auto param = _param_for_multiple_files.empty() ? std::wstring{ _param } : std::wstring{ _param_for_multiple_files };
			// get parent from first path

			std::wstring parentPath;
			if (const auto firstPath = PathHelper::getPath(selection); !firstPath.empty()) {
				const std::filesystem::path file(firstPath);
				parentPath = file.parent_path().wstring();
			}

			if (param.find(PARAM_PARENT) != std::wstring::npos) {
				PathHelper::replaceAll(param, PARAM_PARENT, parentPath);
			}

			PathHelper::replaceAll(param, PARAM_PATH, paths);
			const auto exePath = wil::ExpandEnvironmentStringsW(_exe.c_str());
			OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Invoke menu={}, exePath={}, param={}", _title, exePath.get(), param).c_str());
			ShellExecute(parent, L"open", exePath.get(), param.c_str(), parentPath.data(), SW_SHOWNORMAL);
		}
	}
	else if (count > 1 && _accept_multiple_files_flag == FILES_EACH) {
		if (const auto paths = PathHelper::getPathList(selection); !paths.empty()) {
			OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Invoke menu={}, each", _title).c_str());

			for (auto& path : paths) {
				if (path.empty()) {
					continue;
				}
				Execute(parent, path);
			}
		}
	}
	else if (count > 0) {
		const auto path = PathHelper::getPath(selection);
		Execute(parent, path);
	}

	return S_OK;
}CATCH_RETURN();

void CustomSubExplorerCommand::Execute(HWND parent, const std::wstring& path) {
	OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Execute menu={}, path={}", _title, path).c_str());

	if (path.empty()) {
		return;
	}

	const std::filesystem::path file(path);
	/*
	//TODO use parser
	auto param = std::wstring{_param};
	PathHelper::replaceAll(param, PARAM_PARENT, file.parent_path().wstring());
	PathHelper::replaceAll(param, PARAM_NAME, file.filename().wstring());
	PathHelper::replaceAll (param, PARAM_NAME_NO_EXT, file.stem().wstring ());
	PathHelper::replaceAll(param, PARAM_PATH, path);
	*/
	std::map<std::wstring_view, std::wstring> replacements = {
		{PARAM_PARENT, file.parent_path().wstring()},
		{PARAM_NAME, file.filename().wstring()},
		{PARAM_EXT, file.extension().wstring()},
		{PARAM_NAME_NO_EXT, file.stem().wstring()},
		{PARAM_PATH, path}
	};

	std::wstring param;
	size_t index = 0;
	while (index < _param.size()) {
		bool replaced = false;
		for (const auto& [key, value] : replacements) {
			if (_param.substr(index, key.size()) == key) {
				param += value;
				index += key.size();
				replaced = true;
				break;
			}
		}
		if (!replaced) {
			param += _param[index];
			++index;
		}
	}

	/*
	std::wstring param;
	std::wstring tempKey;
	bool recording = false;

	for (const auto& ch : _param) {
		if (ch == L'{') {
			recording = true;
			tempKey.clear();
		} else if (ch == L'}') {
			recording = false;
			if (replacements.count(tempKey) > 0) {
				param += replacements[tempKey];
			} else {
				param += L"{" + tempKey + L"}";
			}
		} else if (recording) {
			tempKey += ch;
		} else {
			param += ch;
		}
	}
	*/

	const auto exePath = wil::ExpandEnvironmentStringsW(_exe.c_str());
	OutputDebugStringW(std::format(L"CustomSubExplorerCommand::Invoke menu={}, exe={}, param={}", _title, exePath.get(), param).c_str());
	ShellExecute(parent, L"open", exePath.get(), param.c_str(), file.parent_path().c_str(), SW_SHOWNORMAL);
}
