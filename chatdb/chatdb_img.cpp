#include <windows.h>
#include <gdiplus.h>
#include <combaseapi.h>
#include <math.h>

/*
#include <stdio.h>
void log(char *msg ) {
	FILE *fp = fopen("chatdb_img.log", "a");
	fprintf(fp, "\nmsg::\n");
	fprintf(fp, msg);
	fclose(fp);
}
*/

#define CHAT_DB_DLL_EXPORT 1
#include "chatdb.h"


static int getEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

   Gdiplus::GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }    
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}


class ChatDbImage {
	private:
		int error;
		ULONG_PTR gdiplusToken;
		bool gdiplusTokenIsValid;

		Gdiplus::Image *image;
		Gdiplus::Bitmap *imageBitmap;
		Gdiplus::Bitmap *iconBitmap;

		int imageWidth, imageHeight; 

		IStream* imageStream;
		IStream* iconStream;

		char *imageBuffer;
		unsigned int imageBufferLength;
		char *iconBuffer;
		unsigned int iconBufferLength;
	
		void free() {
			if( this->iconStream != nullptr ) {
				this->iconStream->Release();
				this->iconStream = nullptr;
			}
			if( this->imageStream != nullptr ) {
				this->imageStream->Release();
				this->imageStream = nullptr;
			}
			if( this->iconBitmap != nullptr ) {
				delete this->iconBitmap;
				this->iconBitmap = nullptr;
			}
			if( this->imageBitmap != nullptr ) {
				delete this->imageBitmap;
				this->imageBitmap = nullptr;
			}
			if( this->image != nullptr ) {
				delete this->image;
				this->image = nullptr;
			}
			if( this->gdiplusTokenIsValid ) {
				Gdiplus::GdiplusShutdown(this->gdiplusToken);
				this->gdiplusTokenIsValid = false;
			}
		}

	public:
		ChatDbImage( wchar_t *src, int imw, int imh, int icw, int ich );
		~ChatDbImage();

		char *getImageBuffer( unsigned int& length, int& w, int& h ) {
			length = this->imageBufferLength;
			w = this->imageWidth;
			h = this->imageHeight;
			return this->imageBuffer;
		}

		char *getIconBuffer( unsigned int& length ) {
			length = this->iconBufferLength;
			return this->iconBuffer;
		}

		int getError() { return this->error; }
};

ChatDbImage::~ChatDbImage() {
	if( this->imageBuffer != nullptr ) {
		delete this->imageBuffer;
		this->imageBuffer = nullptr;
	}
	if( this->iconBuffer != nullptr ) {
		delete this->iconBuffer;
		this->iconBuffer = nullptr;
	}
	this->free();
}

ChatDbImage::ChatDbImage( wchar_t *src, int imw, int imh, int icw, int ich ) {
	this->image = nullptr;
	this->imageBitmap = nullptr;
	this->iconBitmap = nullptr;
	this->imageBuffer = nullptr;
	this->iconBuffer = nullptr;
	this->imageStream = nullptr;
	this->iconStream = nullptr;
	this->error = 0;
	this->gdiplusTokenIsValid = false;

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::Status status = Gdiplus::GdiplusStartup(&this->gdiplusToken, &gdiplusStartupInput, NULL);
	if( status != Gdiplus::Status::Ok ) {
		this->error = 1;
		return;
	}
	this->gdiplusTokenIsValid = true;

	// Getting clsid for "image/jpeg"
	CLSID  jpgClsid;
  int result = getEncoderClsid(L"image/jpeg", &jpgClsid);
  if(result < 0) {
		this->error = 1;
		return;
  }
	//CLSID jpgClsid;
	//CLSIDFromString(L"{557cf401-1a04-11d3-9a73-0000f81ef32e}", &jpgClsid);

	// Читаем изображние из файла
	this->image = new Gdiplus::Image(src);
	if( this->image->GetLastStatus() != Gdiplus::Status::Ok ) {
		this->error = 1;
		return;
	}
	
	double width = this->image->GetWidth();
	double height = this->image->GetHeight();
	double square = width * height;

	// **********************************************************
	// Из прочитанного из файла изображения (предполагаем, что оно может быть очень большим) создаем уменьшенное изображение и иконку

	// Вычисляем ширину и высоту уменьшенного изображения 
	double image_target_square = imw*imh;	// Максимально возможная площадь изображения 
	double image_k = (square > image_target_square) ? (sqrt(image_target_square / square)) : 1.0;
	int image_width = (int)(image_k * width);		
	int image_height = (int)(image_k * height);

	this->imageWidth = image_width;
	this->imageHeight = image_height;
	 
	// Создаем изображение
	this->imageBitmap = new Gdiplus::Bitmap(image_width, image_height, this->image->GetPixelFormat());
  Gdiplus::Graphics image_graphics(this->imageBitmap);
  image_graphics.DrawImage(this->image, 0, 0, image_width, image_height);
	// Сохраняем его в файл
	// imageBitmap->Save(L"test_image.jpg", &jpgClsid, nullptr);

	// Вычисляем ширину и высоту иконки 
	double icon_target_square = icw*ich;
	double icon_k = (square > icon_target_square) ? (sqrt(icon_target_square / square)) : 1.0;
	int icon_width = (int)(icon_k * width);
	int icon_height = (int)(icon_k * height);

	// Создаем иконку
	this->iconBitmap = new Gdiplus::Bitmap(icon_width, icon_height, this->image->GetPixelFormat());
  Gdiplus::Graphics icon_graphics(this->iconBitmap);
  icon_graphics.DrawImage(this->image, 0, 0, icon_width, icon_height);
	// Сохраняем ее в файл
	// iconBitmap->Save(L"test_icon.jpg", &jpgClsid, nullptr);

	// **** Копируем изображение и иконку в буферы через потоки
	ULARGE_INTEGER ulnSize;
  LARGE_INTEGER lnOffset;
	ULONG ulBytesRead;

	// Копируем изображение
  CreateStreamOnHGlobal(NULL, TRUE, (LPSTREAM*)&this->imageStream);
	this->imageBitmap->Save(this->imageStream, &jpgClsid);

  lnOffset.QuadPart = 0;
  this->imageStream->Seek(lnOffset, STREAM_SEEK_END, &ulnSize);
  this->imageStream->Seek(lnOffset, STREAM_SEEK_SET, NULL);

	unsigned int image_size = (unsigned int)ulnSize.QuadPart;
	this->imageBuffer = new char[image_size+1]; 	// Буфер для хранения byte-array изображения
	this->imageBufferLength = image_size;
	this->imageStream->Read(this->imageBuffer, (ULONG)ulnSize.QuadPart, &ulBytesRead);

	// Копируем иконку
  CreateStreamOnHGlobal(NULL, TRUE, (LPSTREAM*)&this->iconStream);
	this->iconBitmap->Save(this->iconStream, &jpgClsid);

  lnOffset.QuadPart = 0;
  this->iconStream->Seek(lnOffset, STREAM_SEEK_END, &ulnSize);
  this->iconStream->Seek(lnOffset, STREAM_SEEK_SET, NULL);

	unsigned int icon_size = (unsigned int)ulnSize.QuadPart;
	this->iconBuffer = new char[icon_size+1]; 	// Буфер для хранения byte-array изображения
	this->iconBufferLength = icon_size;
	this->iconStream->Read(this->iconBuffer, (ULONG)ulnSize.QuadPart, &ulBytesRead);

	//this->free();
}


void *chatDbCreateImage_( wchar_t *src, int imw, int imh, int icw, int ich ) {
	ChatDbImage *im = new ChatDbImage(src, imw, imh, icw, ich );
	return (void*)im;
}

void chatDbFreeImage_( void *im ) {
	delete (ChatDbImage *)im;
}

char *chatDbGetImageBuffer_( void *im, unsigned int& length, int& w, int& h ) {
	ChatDbImage *chatDbImage = (ChatDbImage *)im;
	return chatDbImage->getImageBuffer( length, w, h );
} 

char *chatDbGetIconBuffer_( void *im, unsigned int& length ) {
	ChatDbImage *chatDbImage = (ChatDbImage *)im;
	return chatDbImage->getIconBuffer( length );
} 

int chatDbGetImageStatus_( void *im ) {
	ChatDbImage *chatDbImage = (ChatDbImage *)im;
	return chatDbImage->getError();
}