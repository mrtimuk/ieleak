#include "stdafx.h"

#ifdef TEST
//  a lite test rig


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "test.h"  //  put our own header first over the implementation
#include <cassert>
#include <memory>

none_of_your_business:: dostream *none_of_your_business::p_dout;
none_of_your_business::wdostream *none_of_your_business::p_wdout;

TestCase *TestCase::head = NULL;


bool TestCase::stopAtError (true);

bool runAllTests()
{

        TestCase *aCase = TestCase::head;
        bool result (true);

        for (; result && aCase; aCase = aCase -> next)
                result = aCase -> runTests();

        if (!result)  //  TODO  report disabled test count
                INFORM("Tests failed!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        else
                INFORM("All tests passed!");

        return result;  //  pass failure to calling script

}


//  this is a tiny regex matcher lifted from:

// http://twistedmatrix.com/users/jh.twistd/cpp/moin.cgi/SmallestSimpleRegexMatchCompetition

// MH - MatchesRegEx helper function. I decided to abbreviate its name to keep the scroll down.
//      Especially, since this function likes to play with itself, a lot.
static
bool
MH(const char *inputString, const char *regEx, bool plusAsStar=false)
{

        // For each remaining character in the current sub-string and each remaining reg-ex character
        for (
                bool escape = false;
                *inputString || *regEx ;
                inputString++, regEx++
        ) {
                if (*regEx=='\\') {
                        escape = true;
                        if (!*++regEx)
                                return false;
                }
                if (!escape)
                        switch (*regEx) {
                        case '?': // Match zero or one characters
                                if (MH(inputString,regEx+1)) // Try ignoring the ? reg ex char and matching the rest of the reg-ex (matching zero chars case)
                                        return true;
                                // Continue processing as if '?' was '.'
                        case '.': // Match any one character
                                if (!*inputString) // If no more chars left in string, not a match
                                        return false;
                                break; // Skip current char
                        case '*': // Match zero or more characters
                                return MH(inputString,regEx+1) || // Try moving on to the next reg-ex char
                                       *inputString && (MH(inputString+1,regEx) || // Try skipping the current string char
                                                        MH(inputString+1,regEx+1)); // Try doing both
                                break;
                        case '+': // Match one or more characters
                                return *inputString && MH(inputString+1,regEx,true) || // Skip a single character, treating the + in the reg-ex as a *
                                       plusAsStar && (MH(inputString,regEx+1) || // Try moving on to the next reg-ex char
                                                      *inputString && MH(inputString+1,regEx+1)); // Try skipping the current string char and the + treated as * in the reg-ex
                                break;
                        default: // Match the next character, exactly
                                if (*inputString!=*regEx) // Test for "out of reg-ex" or a mismatch, if the reg-ex char is not special
                                        return false;
                        }
                else // Current char is escaped
                {
                        escape=false; // Turn off escape
                        if (*inputString!=*regEx) // Test for "out of reg-ex" (which means the reg-ex is invalid) or a mismatch
                                return false;
                }
        }

        return !*inputString; // No more characters in the inputString indicates success

}


bool
MatchesRegEx(std::string inputString, std::string regEx)
{
        return MH(inputString.c_str(),regEx.c_str());
}

#endif //#ifdef TEST