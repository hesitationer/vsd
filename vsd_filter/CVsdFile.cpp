/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFile.cpp - CVsdFile class implementation
	
*****************************************************************************/

#include "StdAfx.h"

#include "CVsdFile.h"

/*** �t�@�C���I�[�v�� *******************************************************/

int CVsdFile::Open( LPCWSTR szFile, LPCWSTR szMode ){
	
	LPCWSTR pMode = szMode;
	
	// gz open
	if( *pMode == 'z' ){
		++pMode;
		char *pFile = NULL;
		char *pMode = NULL;
		
		StringNew( pFile, szFile );
		StringNew( pMode, szMode );
		
		m_gzfp = gzopen( pFile, pMode );
		delete [] pFile;
		delete [] pMode;
		return m_gzfp != NULL ? 0 : 1;
	}
	
	// ���� fopen
	m_fp = _wfopen( szFile, szMode );
	return m_fp != NULL ? 0 : 1;
}

/*** �t�@�C���N���[�Y *******************************************************/

void CVsdFile::Close( void ){
	if( m_gzfp ){
		gzclose( m_gzfp );
		m_gzfp = NULL;
	}
	
	if( m_fp ){
		fclose( m_fp );
		m_fp = NULL;
	}
}

/*** ���C�����[�h ***********************************************************/

char *CVsdFile::ReadLine( void ){
	*m_cBuf = '\0';
	
	if( m_gzfp ){
		gzgets( m_gzfp, m_cBuf, BUF_LEN );
	}else if( m_fp ){
		fgets( m_cBuf, BUF_LEN, m_fp );
	}

	return m_cBuf;
}

/*** eof �`�F�b�N ***********************************************************/

int CVsdFile::IsEOF( void ){
	if( m_gzfp ){
		return gzeof( m_gzfp );
	}else if( m_fp ){
		return feof( m_fp );
	}
	return 1;
}
