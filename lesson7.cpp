/// include some basic string support for test purposes
#include <iostream>
#include <vector>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON SEVEN:
/// COMMON CONTAINER TYPES: ARRAYS AND VECTORS

/// Application Entrypoint (for a Console Application)
int main()
{

    /// Create a local array of ints
    int values[5] = {111,222,333,444,555};

    /// Iterate the array
    for(unsigned int i=0;i<5; i++){
        cout << values[i]<<endl;
    }
    cout << endl;

    /// Create a local vector of strings
    vector<string> mystrings = {"aaa", "bbb", "ccc"};

    /// Vector iterator: forwards, via std iterator methods
    for(auto it=mystrings.begin(); it!=mystrings.end(); it++){
        cout << *it <<endl;
    }
    cout << endl;

    /// Let's iterate backwards, via array operator
    for(int i=2; i>=0;i--){
        cout << mystrings[i] <<endl;
    }
    cout << endl;

    /// Let's treat our vector as a Stack
    while(!mystrings.empty()){
        cout << mystrings.back() <<endl;
        mystrings.pop_back();
    }

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}
