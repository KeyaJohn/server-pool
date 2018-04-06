/*************************************************************************
	> File Name: sougou.cpp
	> Author:kayejohn 
	> Mail:1572831416 
	> Created Time: Wed 04 Apr 2018 03:08:13 AM PDT
 ************************************************************************/

#include <iostream>
#include <string>
#include <stdio.h>
#include <algorithm>
#include <fstream>
using namespace std;

int main()
{
    ifstream  file("cgi/sougou.html");
    cout <<"HTTP/1.1 200 OK\r\n";
    cout << "Content-Type: text/html\r\n";
    cout <<"\r\n";
    string str;

    while(getline(file,str))
    {
        cout <<str<<endl;
    }
    return 0;
}
