#pragma once
enum class LibraryView {Carousel,Grid,List};
extern LibraryView libraryView;
extern int libraryColumns;
void setLibraryView(LibraryView mode);
bool navigateLibrary(int dx,int dy);
