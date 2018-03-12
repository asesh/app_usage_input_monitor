//
//

#include "stdafx.h"

#include "file_writer.h"

CFileWriter::CFileWriter()
{

}

CFileWriter::~CFileWriter()
{

}

bool CFileWriter::init(const wchar_t *file_name)
{
	m_app_input_data = std::wofstream(file_name, std::ios::trunc);
	if (!m_app_input_data.is_open())
	{
		return false;
	}

	return true;
}

void CFileWriter::close()
{
	if (m_app_input_data.is_open())
	{
		m_app_input_data.close();
	}
}

void CFileWriter::write_data(const wchar_t *data_to_write)
{
	m_app_input_data << data_to_write;
}