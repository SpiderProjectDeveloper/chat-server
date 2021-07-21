cl /Fe"chatdb.dll" /EHsc /LD /DLL /W2 /O2 /GL /Gy chatdb.cpp chatdb_aux.cpp chatdb_img.cpp sqlite3.c Ws2_32.lib Shlwapi.lib user32.lib shell32.lib gdiplus.lib Ole32.lib
