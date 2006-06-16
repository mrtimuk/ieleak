#ifdef TEST

#ifndef db

#ifndef _DEBUG
#error don't use this in release mode
#endif

#include <sstream>
#include <iostream>
#include <string>

#include <windows.h>


using std::cout;
using std::wcout;

namespace none_of_your_business {

//  from http://www.codeproject.com/debug/debugout.asp

template <class CharT, class TraitsT = std::char_traits<CharT> >
class basic_debugbuf : 
    public std::basic_stringbuf<CharT, TraitsT>
{
public:

    virtual ~basic_debugbuf()
    {
        sync();
    }

protected:

    int sync()
    {
        output_debug_string(str().c_str());
        str(std::basic_string<CharT>());    // Clear the string buffer

        return 0;
    }

    void output_debug_string(const CharT *text) {}
};

template<>
void basic_debugbuf<char>::output_debug_string(const char *text)
{
    cout << str();
    ::OutputDebugStringA(text);
}

template<>
void basic_debugbuf<wchar_t>::output_debug_string(const wchar_t *text)
{
     wcout << str();
    ::OutputDebugStringW(text);
}


template<class CharT, class TraitsT = std::char_traits<CharT> >
class basic_dostream : 
    public std::basic_ostream<CharT, TraitsT>
{
public:

    basic_dostream() : std::basic_ostream<CharT, TraitsT>
                (new basic_debugbuf<CharT, TraitsT>()) {}
    ~basic_dostream() 
    {
        delete rdbuf(); 
    }
};

typedef basic_dostream<char>    dostream;
typedef basic_dostream<wchar_t> wdostream;

//  externs construct in any order, but statics construct top-to-bottom

    extern  dostream *p_dout;
    extern wdostream *p_wdout;

    static struct oneShot 
	{
	public:
		oneShot()
        {
			if (!p_dout)   p_dout  = new  dostream;
			if (!p_wdout)  p_wdout = new wdostream;
        }
		~oneShot()
		{
			delete p_dout;
			p_dout = NULL;
			delete p_wdout;
			p_wdout = NULL;
		}
	} s_oneShot;

} // namespace none_of_your_business


static none_of_your_business:: dostream & dout  = *none_of_your_business::p_dout ;
static none_of_your_business::wdostream & wdout = *none_of_your_business::p_wdout;


#   define wdb(x)  do { wdout << WIDEN(__FILE__) << L"(" << __LINE__ << \
                             L") : " L#x L" = " << x << endl; \
                            } while (0)

  //  warning! x has no (latex) around it. Don't confuse
  //  it with operators that don't precede << 

#   define db(x)  do { dout << __FILE__ << "(" << __LINE__ << \
                             ") : " #x " = " << x << endl; \
                            } while (0)


#   ifdef USES_CONVERSION
    inline
    std::ostream& 
operator<<(std::ostream& os, const GUID& guid)
{

    OLECHAR sbuff[100];
    StringFromGUID2(guid, sbuff, 100);
    OutputDebugStringW(sbuff);
    USES_CONVERSION;
    os << W2A(sbuff);
    return os;
}
#   endif


#   if defined(WM_QUIT)  //  only members of the <windows.h> club may use this
    static void
DoEvents()
{
  MSG msg;

  while( GetMessage( &msg, NULL, 0, 0) )
  {     
    if (msg.message == WM_QUIT)
        return ;

    TranslateMessage( &msg );
    DispatchMessage( &msg );
  }
}
#   endif


    inline
    std::string
getCompiledPath_(std::string path)
    {
    std::string::size_type at (path.rfind('\\'));
//        ATLASSERT(at != path.npos);
    return path.substr(0, at + 1);  //  includes the trailing "\\"
    }
#define getCompiledPath() getCompiledPath_(__FILE__)

#define CHANGE_CURRENT_DIRECTORY_TO_COMPILED_PATH()  \
    namespace { static struct ccdtcp_ { ccdtcp_() {               \
        std::string folder (getCompiledPath());        \
        _chdir(folder.c_str());                          \
        }} ccdtcp_now_;  }  //  who wants the name ccdtpc_ anyway? ;-)


#endif

#endif //#ifdef TEST