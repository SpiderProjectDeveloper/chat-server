Gdiplus::Bitmap* GDIPlusImageProcessor::ResizeClone(Bitmap *bmp, INT width, INT height)
{     
	UINT o_height = bmp->GetHeight();     
	UINT o_width = bmp->GetWidth();     
	INT n_width = width;     
	INT n_height = height;     
	double ratio = ((double)o_width) / ((double)o_height);     
	if (o_width > o_height) {         // Resize down by width         
		n_height = static_cast<UINT>(((double)n_width) / ratio);     
	} 
	else {         
		n_width = static_cast<UINT>(n_height * ratio);     
	}     
	
	Gdiplus::Bitmap* newBitmap = new Gdiplus::Bitmap(n_width, n_height, bmp->GetPixelFormat());     
	Gdiplus::Graphics graphics(newBitmap);     
	graphics.DrawImage(bmp, 0, 0, n_width, n_height);     
	return newBitmap;
}


CLSID encId; 
std::wstring extension = getExtension(filename); 
std::wstring mimetype = getMimeTypeFrom(extension);
if (GetEncoderClsid(mimetype, &encId) > -1) {     
	resizedBmp->Save(std::wstring(L"Output") + extension, &encId);
}


// If we were able to determine the filetype, save it to a stream         
IStream *buffer;         
CreateStreamOnHGlobal(NULL, true, &buffer);         
resizedBmp->Save(buffer, &encId);           // Find the size of the resulting buffer         
STATSTG statstg;         
buffer->Stat(&statstg, STATFLAG_DEFAULT);         
ULONG bmpBufferSize = (ULONG)statstg.cbSize.LowPart;         
int length = bmpBufferSize;           // Seek to the beginning of the stream         
LARGE_INTEGER li = {0};         
buffer->Seek(li, STREAM_SEEK_SET, NULL);           // Copy the resulting data from the stream into our result object         
char *data = new char[bmpBufferSize];         
res.content_type = content_type;         
ULONG bytesRead;         
buffer->Read(data, bmpBufferSize, &bytesRead);         
buffer->Release();





