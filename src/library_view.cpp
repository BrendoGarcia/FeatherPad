#include "app.h"
#include "library_view.h"
LibraryView libraryView=LibraryView::Carousel;
int libraryColumns=1;
void setLibraryView(LibraryView mode){const auto before=libraryView;libraryView=mode;if(!saveDatabase())libraryView=before;firstVisible=0;keyboardFocus=-1;InvalidateRect(win,nullptr,FALSE);}
bool navigateLibrary(int dx,int dy){
 if(collectionsHome||libraryView==LibraryView::Carousel||visibleGames.empty())return false;
 int current=keyboardFocus;
 if(current>=0&&current<static_cast<int>(hits.size())&&hits[current].game<0)return false;
 auto it=std::find(visibleGames.begin(),visibleGames.end(),selection);const int pos=it==visibleGames.end()?0:static_cast<int>(it-visibleGames.begin()),count=static_cast<int>(visibleGames.size());
 int delta=libraryView==LibraryView::Grid?(dy?dy*libraryColumns:dx):(dy?dy:dx);
 if((dy<0&&pos<libraryColumns)||(libraryView==LibraryView::List&&dx<0))return false; // Up/left reach toolbar/sidebar.
 const int next=std::clamp(pos+delta,0,count-1);selection=visibleGames[next];keyboardFocus=-1;InvalidateRect(win,nullptr,FALSE);return true;
}
