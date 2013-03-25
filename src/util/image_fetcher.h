#ifndef __IMAGE_FETCHER_H
#define __IMAGE_FETCHER_H

typedef void (*ImageFetchedCallback)(void* data, el_bool fetched);

void fetch_image(const char* url, const char* art_file, ImageFetchedCallback cb, void* data);  

#endif
