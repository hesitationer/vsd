/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CScript.h - JavaScript
	
*****************************************************************************/

#pragma once

class CVsdFilter;

class CScript {
  public:
	CScript( CVsdFilter *pVsd );
	~CScript( void );
	
	void Dispose( void );
	void Initialize( void );
	UINT RunFile( LPCWSTR szFileName );
	UINT Run( LPCWSTR szFunc, BOOL bNoFunc = FALSE );
	UINT Run_s( LPCWSTR szFunc, LPCWSTR str0, BOOL bNoFunc = FALSE );
	UINT RunArg( LPCWSTR szFunc, int iArgNum, v8::Handle<v8::Value> Args[], BOOL bNoFunc = FALSE );
	
	void ReportException( v8::TryCatch* try_catch );
	
	static CVsdFilter	*m_pVsd;	// �G�c
	
	v8::Persistent<v8::Context> m_Context;
	v8::Isolate	*m_pIsolate;
	
	LPWSTR m_szErrorMsg;
	UINT m_uError;
	
	LPCWSTR GetErrorMessage( void ){
		return m_szErrorMsg ? m_szErrorMsg : m_szErrorMsgID[ m_uError ];
	}
	
  private:
	
	static LPCWSTR m_szErrorMsgID[];
};
