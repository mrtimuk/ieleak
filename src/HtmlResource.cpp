#include "stdafx.h"
#include "HtmlResource.h"

// From http://www.codeproject.com/cpp/GetHTML.asp with slight modification
bool GetHTML(const int& idrHTML, CString& rString)
{
   bool retVal = false;
   try
   {      
      HRSRC hSrc = FindResource(NULL, MAKEINTRESOURCE(idrHTML), RT_HTML);
      if (hSrc != NULL)
      {
         HGLOBAL hHeader = LoadResource(NULL, hSrc);
         if (hHeader != NULL)
         {
            LPCSTR lpcHtml = static_cast<LPCSTR>(LockResource(hHeader));
            if (lpcHtml != NULL)
            {
               DWORD size = SizeofResource(NULL, hSrc);
               rString = CString(lpcHtml, size);
               retVal = true;
            }
            UnlockResource(hHeader);
         }
         FreeResource(hHeader);
      }
   }
   catch (CMemoryException* e)
   {
      SetLastError(ERROR_FUNCTION_FAILED);
      e->ReportError();
      e->Delete();
      retVal = false;
   }
   catch (CResourceException* e)
   {
      SetLastError(ERROR_FUNCTION_FAILED);
      e->ReportError();
      e->Delete();
      retVal = false;
   }
   catch (CException* e)
   {
      SetLastError(ERROR_FUNCTION_FAILED);
      e->ReportError();
      e->Delete();
      retVal = false;
   }
   catch (...)
   {
      SetLastError(ERROR_FUNCTION_FAILED);
      retVal = false;
   }
   return retVal;
}
