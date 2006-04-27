#ifdef TEST

#ifndef TEST_H_
#   define TEST_H_


  //  a lite test rig


#   include "debug.h"  //  for the wrappers on OutputDebugString
#   include <cmath>

#   define VC_EXTRALEAN
#   define WIN32_LEAN_AND_MEAN 

#include <windows.h>

    using std::endl;
    
                                            
#define SQUEAK_()                \
         do {  worked = false;   \
            if (stopAtError)  __asm { int 3 };  \
                } while(false)


//  TODO  oaoo with db?

#define CPPUNIT_SQUEAK(q)       \
         do {  dout << __FILE__ << '(' << __LINE__ << ") : Failed; " << q << endl;  \
            SQUEAK_();                               \
            } while (false)

#ifndef WIDEN
#   define WIDEN2(x) L##x
#   define WIDEN(x) WIDEN2(x)
#endif //! WIDEN


#define CPPUNIT_SQUEAKW(q)       \
         {  wdout << WIDEN(__FILE__) << L'(' << __LINE__ << L") : Failed; " << q << endl;  \
            SQUEAK_();                                \
            }


class disabled_test: public std::invalid_argument
{
public:
disabled_test(): std::invalid_argument("disabled_test")  {}

virtual char const * what() const 
    {
    return "disabled_test";  //  nobody uses this yet
    };
};

#define CPPUNIT_DISABLE(why) do { db(why);  throw disabled_test(); } while(false)

#define CPPUNIT_ASSERT(q)             \
            if (!(q))                 \
                CPPUNIT_SQUEAK(#q);


#define CPPUNIT_ASSERT_DOUBLES_EQUAL(alpha, omega, epsilon)      \
            if (fabs(alpha - omega) > epsilon)                   \
                CPPUNIT_SQUEAK("fabs(" << alpha << " - " << (omega) << ") > " << (epsilon));

    	
#define CPPUNIT_ASSERT_EQUAL(alpha, omega)            \
            if ((alpha) != (omega))                   \
                CPPUNIT_SQUEAK(#alpha " != " #omega << "; " << (alpha) << " != " << (omega));

#define CPPUNIT_ASSERT_GREATER(alpha, omega)            \
            if ((alpha) <= (omega))                   \
                CPPUNIT_SQUEAK(#alpha " <= " #omega << "; " << (alpha) << " <= " << (omega));

//  observe this only matches simple expressions

bool MatchesRegEx(std::string inputString, std::string regEx);

//  TODO  OAOO the several similar CPPUNIT_SQUEAK() calls

#define CPPUNIT_ASSERT_MATCH(regex, sample)            \
            if (!MatchesRegEx((sample), (regex)))                   \
                CPPUNIT_SQUEAK(#regex " !~ " #sample << ";\n" << (regex) << " !~ " << (sample));


#define CPPUNIT_ASSERT_NO_MATCH(regex, sample)            \
            if (MatchesRegEx((sample), (regex)))                   \
                CPPUNIT_SQUEAK(#regex " =~ " #sample << ";\n" << (regex) << " =~ " << (sample));


#define CPPUNIT_ASSERT_NOT_EQUAL(alpha, omega)            \
            if ((alpha) == (omega))                   \
                CPPUNIT_SQUEAK(#alpha " == " #omega << "; " << (alpha) << " == " << (omega));


#define CPPUNIT_ASSERT_EQUALW(alpha, omega)            \
            if ((alpha) != (omega))                   \
                CPPUNIT_SQUEAKW(L#alpha L" != " L#omega << L"; " << (alpha) << L" != " << (omega));


#define ATTENTION_ " -------------------------> "

#define INFORM(inform) \
            do { dout << inform << endl;  \
            } while(false)

#define CPPUNIT_TEST_SUITE(TestCase)    \
		    bool runTests() {            \
                worked = true;            \
                INFORM(#TestCase);         \
                classSetup();


#define CPPUNIT_TEST(callMe)       \
            INFORM("   " #callMe);  \
            try {                    \
             setUp();                 \
             (callMe)();               \
             }                          \
            catch (disabled_test & )     \
                {                         \
                INFORM("DISABLED");        \
                }                           \
            tearDown();                      
          //  The ExecuteAroundDesignPattern in a macro ;-)
          //  (but notice tearDown is forbidden to throw)


#define CPPUNIT_TEST_SUITE_END() \
            classTeardown();      \
		    return worked;		   \
		    }


	class TestCase
    {
    public:
        static bool stopAtError;
        
        TestCase ():  worked (true), next(NULL) {
            next = head;
            head = this;            
            }

      	void			setUp () {}
  	    void			tearDown () {}
        bool worked;  //  TODO  make me static
        static TestCase *head;  //  TODO  make us private
        TestCase *next;
        virtual bool runTests() = 0;
        virtual void classSetup()  {}
        virtual void classTeardown()  {}

    };

    bool runAllTests();

#define TEST_(suite, target)            \
    struct suite##target:  public suite \
    {     void runCase();               \
    bool runTests() {  setUp();         \
        runCase();  tearDown();         \
        return worked; }                \
    }                                   \
    a##suite##target;                   \
    void suite##target::runCase()

#endif // ! TEST_H_

#endif //#ifdef TEST