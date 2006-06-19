#ifdef NEVER
#ifdef __WIN32__
# include <stdio.h>

# include "system.h"
#include <imagehlp.h>

#define STACKWALK_MAXLEVEL	100
#if _M_IX86
#define STACKWALK_MACHINETYPE	IMAGE_FILE_MACHINE_I386
#else
#define STACKWALK_MACHINETYPE	IMAGE_FILE_MACHINE_ALPHA
#endif

#define WINST_UNKNOWN		0
#define WINST_BAAN		1	/* Baan winstation is currently active */
#define WINST_INTERACTIVE	2	/* Interactive winstation is currently active */
#define WINST_DEFAULT		3	/* Not running in baan-winstation */

typedef WINBASEAPI BOOL ( WINAPI * SetProcessAffinityMaskFun)( HANDLE hProcess,
								DWORD dwProcessAffinityMask );


typedef BOOL (__stdcall * SYMINITIALIZE)( HANDLE, LPSTR, BOOL );
typedef BOOL (__stdcall *SYMCLEANUP)( HANDLE );
typedef LPVOID (__stdcall *SYMFUNCTIONTABLEACCESS)( HANDLE, DWORD );
typedef DWORD (__stdcall *SYMGETMODULEBASE)( HANDLE, DWORD );
typedef BOOL (__stdcall *SYMGETSYMFROMADDR) ( HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL );
typedef BOOL (__stdcall * STACKWALK)( DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID,
				       PREAD_PROCESS_MEMORY_ROUTINE,PFUNCTION_TABLE_ACCESS_ROUTINE,
				       PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE );
typedef DWORD (__stdcall * SYMGETOPTIONS)( VOID );
typedef DWORD (__stdcall * SYMSETOPTIONS)( DWORD );

typedef struct 
{
	DWORD Code;
	char * Description;
}
EXCDESCRSTRUCT;

typedef struct
{
	char * Data;
	int CurSize;
	int BufSize;
	int Delta;
}
GROWBUFFER;

static GROWBUFFER * CreateBuffer( int initialSize, int delta );
static void FreeBuffer( GROWBUFFER * buffer );
static void AddToBuffer( GROWBUFFER * buffer, char * data, int size );
static char * GetBufferPtr( GROWBUFFER * buffer );


static void DoStackWalk( HANDLE hProcess, HANDLE hThread, PCONTEXT pContext );
static void GetExceptionDescription( DWORD code, char * buffer, int bufSize );
static BOOL GetLogicalAddress( PVOID addr, DWORD * section, DWORD * offset );
static BOOL LoadImageHelp( );

static SYMINITIALIZE	       pSymInitialize;
static SYMFUNCTIONTABLEACCESS  pSymFunctionTableAccess;
static SYMGETMODULEBASE	       pSymGetModuleBase;
static SYMGETSYMFROMADDR       pSymGetSymFromAddr;
static SYMCLEANUP	       pSymCleanup;
static STACKWALK 	       pStackWalk;
static SYMGETOPTIONS	       pSymGetOptions;
static SYMSETOPTIONS	       pSymSetOptions;


void LogStackTrace( PEXCEPTION_POINTERS pExceptionInfo )
{
	PEXCEPTION_RECORD pExceptionRecord;
	HANDLE		  hProcess;
	char		  buffer[ 256 ];
	char              symbolPath[ MAX_PATH ];
	char            * ptr;


	//pExceptionRecord = pExceptionInfo->ExceptionRecord;
	hProcess = GetCurrentProcess( );


	//GetExceptionDescription( pExceptionRecord->ExceptionCode, buffer, sizeof( buffer ) );
	//printf( "Exception %08X (%s)\n", pExceptionRecord->ExceptionCode, buffer );

	GetModuleFileName( 0, symbolPath, sizeof( symbolPath ) );
	ptr = strrchr( symbolPath, '\\' );
	if ( ptr )
	{
		*ptr = '\0';
	}

	if ( LoadImageHelp( ) )
	{
		pSymSetOptions ( pSymGetOptions( ) | SYMOPT_DEFERRED_LOADS );
		if ( pSymInitialize( hProcess, symbolPath, TRUE ) )
		{
			DoStackWalk( hProcess, GetCurrentThread( ), /*pExceptionInfo->ContextRecord*/ );

			pSymCleanup( hProcess );
		}
		else
		{
			printf( "\t Can't generate stacktrace: unable to initialize symbol table" );
		}
	}
	else
	{
		printf( "\t Can't generate stacktrace: unable to load imagehlp.dll" );
	}
}


static void DoStackWalk( HANDLE hProcess, HANDLE hThread, PCONTEXT pContext )
{
	int		  level;
	STACKFRAME	  sf;
	GROWBUFFER	* outputBuffer;
        BYTE		  symbolBuffer[ sizeof(IMAGEHLP_SYMBOL) + 512 ];
        DWORD		  symDisplacement = 0;
        PIMAGEHLP_SYMBOL  pSymbol;
	DWORD		  section;
	DWORD		  offset;
	char		  temp[ 256 ];

	pSymbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
	outputBuffer = CreateBuffer( 1024, 1024 );
	memset( &sf, 0, sizeof(sf) );

#if _M_IX86
	sf.AddrPC.Offset       = pContext->Eip;
	sf.AddrPC.Mode         = AddrModeFlat;
	sf.AddrStack.Offset    = pContext->Esp;
	sf.AddrStack.Mode      = AddrModeFlat;
	sf.AddrFrame.Offset    = pContext->Ebp;
	sf.AddrFrame.Mode      = AddrModeFlat;
#endif // _M_IX86

	level = 0;
	while ( level < STACKWALK_MAXLEVEL && 
		 pStackWalk( STACKWALK_MACHINETYPE, hProcess, hThread, &sf, pContext, 
			    0, pSymFunctionTableAccess, pSymGetModuleBase, 0 ) )
	{
		pSymbol->SizeOfStruct = sizeof(symbolBuffer);
		pSymbol->MaxNameLength = 512;
                       
	        if ( sf.AddrFrame.Offset == 0 )
			break; 

		GetLogicalAddress( (PVOID)sf.AddrPC.Offset, &section, &offset );

		if ( pSymGetSymFromAddr( hProcess, sf.AddrPC.Offset,
					&symDisplacement, pSymbol ) )
		{
			sprintf( temp, "\t %s+%X (%x:%08x)\n", pSymbol->Name, symDisplacement, 
				 section, offset );
		}
		else
		{
			sprintf( temp, "\t %x:%08x\n", section, offset );
		}

		AddToBuffer( outputBuffer, temp, strlen( temp ) );

		level++;
	}

	/* Add terminating zero */

	AddToBuffer( outputBuffer, "", 1 );
	
	log_printf( GetBufferPtr( outputBuffer ) );

	FreeBuffer( outputBuffer );
}

static void GetExceptionDescription( DWORD code, char * buffer, int bufSize )
{
	static EXCDESCRSTRUCT excDescr[ ] = 
	{
		EXCEPTION_ACCESS_VIOLATION, "Access violation",
		EXCEPTION_DATATYPE_MISALIGNMENT, "Datatype misalignment",
		EXCEPTION_BREAKPOINT, "Breakpoint",
		EXCEPTION_SINGLE_STEP, "Single step",
		EXCEPTION_ARRAY_BOUNDS_EXCEEDED, "Array bounds exceeded",
		EXCEPTION_FLT_DENORMAL_OPERAND, "Float denormal operand",
		EXCEPTION_FLT_DIVIDE_BY_ZERO, "Float divide by zero",
		EXCEPTION_FLT_INEXACT_RESULT, "Float inexact result",
		EXCEPTION_FLT_INVALID_OPERATION, "Float invalid operation",
		EXCEPTION_FLT_OVERFLOW, "Float overflow",
		EXCEPTION_FLT_STACK_CHECK, "Float stack check",
		EXCEPTION_FLT_UNDERFLOW, "Float underflow",
		EXCEPTION_INT_DIVIDE_BY_ZERO, "Integer divide by zero",
		EXCEPTION_INT_OVERFLOW, "Integer overflow",
		EXCEPTION_PRIV_INSTRUCTION, "Privileged instruction",
		EXCEPTION_IN_PAGE_ERROR, "In page error",
		EXCEPTION_ILLEGAL_INSTRUCTION, "Illegal instruction",
		EXCEPTION_NONCONTINUABLE_EXCEPTION, "Noncontinuable exception",
		EXCEPTION_STACK_OVERFLOW, "Stack overflow",
		EXCEPTION_INVALID_DISPOSITION, "Invalid disposition",
		EXCEPTION_GUARD_PAGE, "Guard page",
		EXCEPTION_INVALID_HANDLE, "Invalid handle",
		0, NULL 
	};
	int i;

	i = 0;
	while ( excDescr[ i ].Code != 0 )
	{
		if ( excDescr[ i ].Code == code )
		{
			strcpy( buffer, excDescr[ i ].Description );
			return;
		}

		i++;
	}

	if ( HAL_os( ) == OS_WINDOWS_95 )
	{
		strcpy( buffer, "Unknown error" );
	}
	else
	{
		FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, 
			       GetModuleHandle( "NTDLL.DLL"),
		       code, 0, buffer, sizeof( bufSize ), 0 );
	}
}

static BOOL GetLogicalAddress( PVOID addr, DWORD * section, DWORD * offset )
{
	MEMORY_BASIC_INFORMATION mbi;
	PIMAGE_DOS_HEADER	 pDosHdr;
	PIMAGE_NT_HEADERS	 pNtHdr;
	PIMAGE_SECTION_HEADER	 pSection;
	unsigned		 i;
	DWORD			 sectionStart;
	DWORD			 sectionEnd;
	DWORD			 hMod;
	DWORD			 rva;

	if ( ! VirtualQuery( addr, &mbi, sizeof(mbi) ) )
		return FALSE;

	hMod = (DWORD)mbi.AllocationBase;

	pDosHdr  = (PIMAGE_DOS_HEADER)hMod;
	pNtHdr   = (PIMAGE_NT_HEADERS)( hMod + pDosHdr->e_lfanew );
	pSection = IMAGE_FIRST_SECTION( pNtHdr );

	rva = (DWORD)addr - hMod; 

	for ( i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++ )
	{
		sectionStart = pSection->VirtualAddress;
		sectionEnd = sectionStart + max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);

	        if ( (rva >= sectionStart) && (rva <= sectionEnd) )
		{
			*section = i+1;
			*offset = rva - sectionStart;
			return TRUE;
		}

		pSection++;
	}

	return FALSE;
}


static BOOL LoadImageHelp( )
{
	HMODULE hModule;

	hModule = LoadLibrary( "IMAGEHLP.DLL" );
	if ( hModule == 0 )
	{
		return FALSE;
	}

	pSymInitialize		= (SYMINITIALIZE) GetProcAddress( hModule, "SymInitialize" );
	pSymFunctionTableAccess = (SYMFUNCTIONTABLEACCESS) GetProcAddress( hModule, "SymFunctionTableAccess" );
	pSymGetModuleBase	= (SYMGETMODULEBASE) GetProcAddress( hModule, "SymGetModuleBase" );
	pSymGetSymFromAddr	= (SYMGETSYMFROMADDR) GetProcAddress( hModule, "SymGetSymFromAddr" );
	pSymCleanup		= (SYMCLEANUP) GetProcAddress( hModule, "SymCleanup" );
	pSymSetOptions          = (SYMSETOPTIONS) GetProcAddress( hModule, "SymSetOptions" );
	pSymGetOptions          = (SYMGETOPTIONS) GetProcAddress( hModule, "SymGetOptions" );
	pStackWalk		= (STACKWALK) GetProcAddress( hModule, "StackWalk" );

	if ( pSymInitialize == NULL ||
	     pSymFunctionTableAccess == NULL ||
	     pSymGetModuleBase == NULL ||
	     pSymGetSymFromAddr == NULL ||
	     pSymCleanup == NULL ||
	     pSymSetOptions == NULL ||
	     pSymGetOptions == NULL ||
	     pStackWalk == NULL )
	{
		return FALSE;
	}

	return TRUE;
}


static GROWBUFFER * CreateBuffer( int initialSize, int delta )
{
	GROWBUFFER * newBuffer;

	newBuffer = (GROWBUFFER *) malloc( sizeof( GROWBUFFER ) );
	newBuffer->Data = malloc( initialSize );
	newBuffer->CurSize = 0;
	newBuffer->BufSize = initialSize;
	newBuffer->Delta   = delta;

	return newBuffer;
}


static void FreeBuffer( GROWBUFFER * buffer )
{
	free( buffer->Data );
	free( buffer );
}

static void AddToBuffer( GROWBUFFER * buffer, char * data, int size )
{
	int    newSize;
	int    newBlocks;
	char * ptr;

	newSize = buffer->CurSize + size;
	if ( newSize > buffer->BufSize )
	{
		newBlocks = 1 + ( newSize - buffer->BufSize ) / buffer->Delta;
		buffer->BufSize += buffer->Delta * newBlocks;
		buffer->Data = realloc( buffer->Data, buffer->BufSize );
	}

	ptr = buffer->Data + buffer->CurSize;
	memcpy( ptr, data, size );
	buffer->CurSize += size;
}

static char * GetBufferPtr( GROWBUFFER * buffer )
{
	return buffer->Data;
}



#endif	/* __WIN32__	*/


#pragma managed

#endif

#using <mscorlib.dll>
#using <System.dll>

using namespace System;
using namespace System::Diagnostics;

__gc class StackTraceSample {
private:
    __gc class MyInternalClass {
    public:
        void ThrowsException() {
            try {
                throw new Exception(S"A problem was encountered.");
            } 
            catch (Exception* e) {
                // Create a StackTrace that captures
                // filename, line number, and column
                // information for the current thread.
                StackTrace *st = new StackTrace(true);
                String *stackIndent = S"";
                for(int i =0; i< st->FrameCount; i++ )
                {
                    // Note that at this level, there are five
                    // stack frames, one for each method invocation.
                    StackFrame *sf = st->GetFrame(i);
                    Console::WriteLine();
                    Console::WriteLine(S"{0}Method: {1}",
                        stackIndent, sf->GetMethod() );
                    Console::WriteLine(S"{0}File: {1}", 
                        stackIndent, sf->GetFileName());
                    Console::WriteLine(S"{0}Line Number: {1}",
                        stackIndent, sf->GetFileLineNumber().ToString());
                    stackIndent = String::Concat(stackIndent,  S"  ");
                }
                throw e;
            }
        }
    };

protected:
    void MyProtectedMethod() {
        MyInternalClass* mic = new MyInternalClass();
        mic->ThrowsException();
    }

public:
    void MyPublicMethod () {
        MyProtectedMethod();
    }
};


int dumpstacktrace()
{
        StackTraceSample* sample = new StackTraceSample();
        try {
            sample->MyPublicMethod();
			return 0;
        } catch (Exception*) 
        {
            // Create a StackTrace that captures
            // filename, line number, and column
            // information for the current thread.
            StackTrace *st = new StackTrace(true);

            for(int i =0; i< st->FrameCount; i++ )
            {
                // For an executable built from C++, there
                // are two stack frames here: one for main,
                // and one for the _mainCRTStartup stub. 
                StackFrame *sf = st->GetFrame(i);
                Console::WriteLine();
                Console::WriteLine(S"High up the call stack, Method: {0}",
                    sf->GetMethod()->ToString());

                Console::WriteLine(S"High up the call stack, Line Number: {0}",
                    sf->GetFileLineNumber().ToString());
            }

			return i;
        }
		return -1;
}