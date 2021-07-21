#include <windows.h>
#include <gdiplus.h>
#include <stdio.h>
#include <combaseapi.h>
#include <string>
#include <iostream>
#pragma comment(lib,"gdiplus.lib")
#pragma comment(lib,"Ole32.lib")

int WINAPI WinMain(
   _In_ HINSTANCE hInstance,
   _In_opt_ HINSTANCE hPrevInstance,
   _In_ LPSTR     lpCmdLine,
   _In_ int       nCmdShow
)
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	CLSID jpgClsid;
	CLSIDFromString(L"{557cf401-1a04-11d3-9a73-0000f81ef32e}", &jpgClsid);

	// Читаем изображние из файла
	Gdiplus::Image *image = new Gdiplus::Image(L"test.jpg");
	unsigned int width = image->GetWidth();
	unsigned int height = image->GetHeight();
	float square = width*height;

	// **********************************************************
	// Из прочитанного из файла изображения (предполагаем, что оно может быть очень большим) создаем уменьшенное изображение и иконку


	// Вычисляем ширину и высоту уменьшенного изображения 
	float image_target_square = 1200*900;	// Максимально возможная площадь изображения 
	float image_k = (square > image_target_square) ? (sqrt(image_target_square / square)) : 1.0;
	int image_width = (int)(image_k * (float)width);		
	int image_height = (int)(image_k * (float)height);
 
	// Создаем изображение
	Gdiplus::Bitmap *image_bitmap = new Gdiplus::Bitmap(image_width, image_height, image->GetPixelFormat());
  Gdiplus::Graphics image_graphics(image_bitmap);
  image_graphics.DrawImage(image, 0, 0, image_width, image_height);
	// Сохраняем его в файл
	image_bitmap->Save(L"test_image.jpg", &jpgClsid, nullptr);

	// Вычисляем ширину и высоту иконки 
	float icon_target_square = 140*140;
	float icon_k = (square > icon_target_square) ? (sqrt(icon_target_square / square)) : 1.0;
	int icon_width = (int)(icon_k * (float)width);
	int icon_height = (int)(icon_k * (float)height);

	// Создаем иконку
	Gdiplus::Bitmap *icon_bitmap = new Gdiplus::Bitmap(icon_width, icon_height, image->GetPixelFormat());
  Gdiplus::Graphics icon_graphics(icon_bitmap);
  icon_graphics.DrawImage(image, 0, 0, icon_width, icon_height);
	// Сохраняем ее в файл
	icon_bitmap->Save(L"test_icon.jpg", &jpgClsid, nullptr);

	// **** Копируем изображение в буффер через поток
	IStream* oStream = NULL;
  CreateStreamOnHGlobal(NULL, TRUE, (LPSTREAM*)&oStream);
	image_bitmap->Save(oStream, &jpgClsid);

	ULARGE_INTEGER ulnSize;
  LARGE_INTEGER lnOffset;
  lnOffset.QuadPart = 0;
  oStream->Seek(lnOffset, STREAM_SEEK_END, &ulnSize);
  oStream->Seek(lnOffset, STREAM_SEEK_SET, NULL);

	char *pBuff = new char[(unsigned int)ulnSize.QuadPart]; 	// Буфер для хранения byte-array изображения
	ULONG ulBytesRead;
	oStream->Read(pBuff, (ULONG)ulnSize.QuadPart, &ulBytesRead);
	oStream->Release();

	// Копируем буфер в файл
	FILE* pFile;
  pFile = fopen("test_image.binary", "wb");
  fwrite(pBuff, 1, (unsigned int)ulnSize.QuadPart, pFile);
  fclose(pFile);

	delete pBuff;
	delete image_bitmap;
	delete icon_bitmap;
	delete image;
	Gdiplus::GdiplusShutdown(gdiplusToken);
	return 0;
}