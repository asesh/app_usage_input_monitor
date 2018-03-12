#pragma once

class CFileWriter
{
public:
	CFileWriter();
	~CFileWriter();

	bool init(const wchar_t *file_name);
	void close();

	void write_data(const wchar_t *data_to_write);

private:
	std::wofstream m_app_input_data;
};