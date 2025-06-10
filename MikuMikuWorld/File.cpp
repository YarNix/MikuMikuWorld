﻿#include "File.h"
#include "IO.h"
#if defined(_WIN32)
#include <Windows.h>
#elif defined(__APPLE__)
#include "Mac.h"
#endif
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

namespace IO
{
	FileDialogFilter mmwsFilter{ "MikuMikuWorld Score", "*.mmws" };
	FileDialogFilter susFilter{ "Sliding Universal Score", "*.sus" };
	FileDialogFilter uscFilter{ "Sonolus Level", "*.usc" };
	FileDialogFilter imageFilter{ "Image Files", "*.jpg;*.jpeg;*.png" };
	FileDialogFilter audioFilter{ "Audio Files", "*.mp3;*.wav;*.flac;*.ogg" };
	FileDialogFilter allFilter{ "All Files", "*.*" };

	File::File(const std::string& filename, FileMode mode)
	{
		stream = std::make_unique<std::fstream>();
		open(filename, mode);
	}

	File::~File()
	{
		if (stream->is_open())
			stream->close();
	}

	std::ios::openmode File::getStreamMode(FileMode mode) const
	{
		switch (mode)
		{
		case FileMode::Read:
			return std::fstream::in;
		case FileMode::Write:
			return std::fstream::out;
		case FileMode::ReadBinary:
			return std::fstream::in | std::fstream::binary;
		case FileMode::WriteBinary:
			return std::fstream::out | std::fstream::binary;
		default:
			return std::ios::openmode(0);
		}
	}

	void File::open(const std::string& filename, FileMode mode)
	{
		openFilename = filename;
#if defined(_WIN32)
		stream->open(IO::mbToWideStr(filename), getStreamMode(mode));
#else
		stream->open(filename, getStreamMode(mode));
#endif
	}

	void File::close()
	{
		openFilename.clear();
		stream->close();
	}

	void File::flush()
	{
		stream->flush();
	}

	std::vector<uint8_t> File::readAllBytes()
	{
		if (!stream->is_open())
			return {};

		stream->seekg(0, std::ios_base::end);
		size_t length = stream->tellg();
		stream->seekg(0, std::ios_base::beg);

		std::vector<uint8_t> bytes;
		bytes.resize(length);
		stream->read((char*)bytes.data(), length);

		return bytes;
	}

	std::string File::readLine()
	{
		if (!stream->is_open())
			return {};

		std::string line{};
		std::getline(*stream, line);
#if !defined(_WIN32)
		if (const auto crPos = line.find('\r'); crPos != std::string::npos) {
			line = line.substr(0, crPos);
		}
#endif
		return line;
	}

	std::vector<std::string> File::readAllLines()
	{
		if (!stream->is_open())
			return {};

		std::vector<std::string> lines;
		while (!stream->eof())
			lines.push_back(readLine());

		return lines;
	}

	std::string File::readAllText()
	{
		if (!stream->is_open())
			return {};

		std::stringstream buffer;
		buffer << stream->rdbuf();
		return buffer.str();
	}

	bool File::isEndofFile()
	{
		return stream->is_open() ? stream->eof() : true;
	}

	void File::write(const std::string& str)
	{
		if (stream->is_open())
		{
			stream->write(str.c_str(), str.length());
		}
	}

	void File::writeLine(const std::string line)
	{
		write(line + "\n");
	}

	void File::writeAllLines(const std::vector<std::string>& lines)
	{
		if (stream->is_open())
		{
			std::stringstream ss{};
			for (const auto& line : lines)
				ss << line + '\n';

			std::string allLines{ ss.str() };
			stream->write(allLines.c_str(), allLines.size());
		}
	}

	void File::writeAllBytes(const std::vector<uint8_t>& bytes)
	{
		if (stream->is_open())
		{
			stream->write((char*)bytes.data(), bytes.size());
		}
	}

	std::string File::getFilename(const std::string& filename)
	{
		size_t start = filename.find_last_of("\\/");
		return filename.substr(start + 1, filename.size() - (start + 1));
	}

	std::string File::getFileExtension(const std::string& filename)
	{
		size_t end = filename.find_last_of(".");
		if (end == std::string::npos)
			return "";

		return filename.substr(end);
	}

	std::string File::getFilenameWithoutExtension(const std::string& filename)
	{
		std::string str = getFilename(filename);
		size_t end = str.find_last_of(".");

		return str.substr(0, end);
	}

	std::string File::getFullFilenameWithoutExtension(const std::string& filename)
	{
		size_t end = filename.find_last_of(".");
		return filename.substr(0, end);
	}

	std::string File::getFilepath(const std::string& filename)
	{
		size_t start = 0;
		size_t end = filename.find_last_of("\\/");

		return filename.substr(start, end - start + 1);
	}

	std::string File::fixPath(const std::string& path)
	{
		std::string result = path;
		int index = 0;
		while (true)
		{
			index = result.find("\\", index);
			if (index == result.npos)
				break;

			result.replace(index, 1, "/");
			index += 1;
		}

		return result;
	}

#if defined(_WIN32)
	bool File::exists(const std::string& path)
	{
		return fs::exists(IO::mbToWideStr(path));
	}

	bool File::createDirectory(const std::string& path)
	{
		return fs::create_directory(IO::mbToWideStr(path));
	}

	bool File::copyFile(const std::string& from, const std::string& to)
	{
		return fs::copy_file(IO::mbToWideStr(from), IO::mbToWideStr(to));
	}

	bool File::remove(const std::string& path)
	{
		return fs::remove(IO::mbToWideStr(path));
	}

	fs::directory_iterator File::directoryIterator(const std::string& path)
	{
		return fs::directory_iterator{ IO::mbToWideStr(path) };
	}

	std::ifstream File::ifstream(const std::string& path)
	{
		return std::ifstream{ IO::mbToWideStr(path) };
	}

	std::ofstream File::ofstream(const std::string& path)
	{
		return std::ofstream{ IO::mbToWideStr(path) };
	}
#else
	bool File::exists(const std::string& path)
	{
		return fs::exists(path);
	}

	bool File::createDirectory(const std::string& path)
	{
		return fs::create_directory(path);
	}

	bool File::copyFile(const std::string& from, const std::string& to)
	{
		return fs::copy_file(from, to);
	}

	bool File::remove(const std::string& path)
	{
		return fs::remove(path);
	}

	fs::directory_iterator File::directoryIterator(const std::string& path)
	{
		return fs::directory_iterator{ path };
	}

	std::ifstream File::ifstream(const std::string& path)
	{
		return std::ifstream{ path };
	}

	std::ofstream File::ofstream(const std::string& path)
	{
		return std::ofstream{ path };
	}
#endif

	FileDialogResult FileDialog::showFileDialog(DialogType type, DialogSelectType selectType)
	{
#if defined(_WIN32)
		std::wstring wTitle = mbToWideStr(title);

		OPENFILENAMEW ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = reinterpret_cast<HWND>(parentWindowHandle);
		ofn.lpstrTitle = wTitle.c_str();
		ofn.nFilterIndex = filterIndex + 1;
		ofn.nFileOffset = 0;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_LONGNAMES | OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

		std::wstring wDefaultExtension = mbToWideStr(defaultExtension);
		ofn.lpstrDefExt = wDefaultExtension.c_str();

		std::vector<std::wstring> ofnFilters;
		ofnFilters.reserve(filters.size());

		/*
			since '\0' terminates the string,
			we'll do a C# by using ' | ' then replacing it with '\0' when constructing the final wide string
		*/
		std::string filtersCombined;
		for (const auto& filter : filters)
		{
			filtersCombined
				.append(filter.filterName)
				.append(" (")
				.append(filter.filterType)
				.append(")|")
				.append(filter.filterType)
				.append("|");
		}

		std::wstring wFiltersCombined = mbToWideStr(filtersCombined);
		std::replace(wFiltersCombined.begin(), wFiltersCombined.end(), '|', '\0');
		ofn.lpstrFilter = wFiltersCombined.c_str();

		std::wstring wInputFilename = mbToWideStr(inputFilename);
		wchar_t ofnFilename[1024]{ 0 };

		// suppress return value not used warning
#pragma warning(suppress: 6031)
		lstrcpynW(ofnFilename, wInputFilename.c_str(), 1024);
		ofn.lpstrFile = ofnFilename;

		if (type == DialogType::Save)
		{
			ofn.Flags |= OFN_HIDEREADONLY;
			if (GetSaveFileNameW(&ofn))
			{
				outputFilename = wideStringToMb(ofn.lpstrFile);
			}
			else
			{
				// user canceled
				return FileDialogResult::Cancel;
			}
		}
		else if (GetOpenFileNameW(&ofn))
		{
			outputFilename = wideStringToMb(ofn.lpstrFile);
		}
		else
		{
			return FileDialogResult::Cancel;
		}

		if (outputFilename.empty())
			return FileDialogResult::Cancel;

		filterIndex = ofn.nFilterIndex - 1;
		return FileDialogResult::OK;
#elif defined(__APPLE__)
		return platform::showFileDialog(title, inputFilename, defaultExtension, filters, type, outputFilename);
#endif
	}

	FileDialogResult FileDialog::openFile()
	{
		return showFileDialog(DialogType::Open, DialogSelectType::File);
	}

	FileDialogResult FileDialog::saveFile()
	{
		return showFileDialog(DialogType::Save, DialogSelectType::File);
	}
}
