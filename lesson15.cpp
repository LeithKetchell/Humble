/// include some basic string support for test purposes
#include <iostream>
#include <map>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON FIFTEEN:
/// BUILDTIME SWITCHES AND MACROS
///
/// First build the app and run it... then,
/// Uncomment the line below... (WATCH THE CODE IN MAIN WHILE YOU DO SO!),
/// hit save, and rebuild the app

//#define doGood 1





/// Application Entrypoint (for a Console Application)
int main()
{

    // Example 1: buildtime switches determine what code gets used at buildtime

    #ifdef doGood
	cout << "I am good" << endl;
    #else
        cout << "I am bad" << endl;
    #endif

    // Example 2: macros are a way to define code in advance...
    // They look like functions, but really the compiler just "dumps a full copy" when the macro is "expressed"...
    // Whenever a mention of a macro is found at buildtime, it gets replaced with the content of the macro
    // Most modern compilers do not allow macros to contain buildtime switches or define nested macros
    // which is a real shame because in the past that was possible, and incredibly powerful
    // Notice that the macro does not need a proper line ending
    #define myMacro(something) cout << something

    myMacro("Help!") << endl;




    ///////////////////////////////////////////////////////////////////////////////////////////

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}

