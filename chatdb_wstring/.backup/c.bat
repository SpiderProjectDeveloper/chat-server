cl /Fe"chatdb.dll" /EHsc /LD /DLL /W2 /O2 /GL /Gy /std:c++17 chatdb.cpp sqlite3.c Ws2_32.lib Shlwapi.lib
