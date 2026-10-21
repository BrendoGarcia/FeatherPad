#include "app.h"

namespace {
 constexpr COLORREF bg=RGB(12,17,25),sidebar=RGB(16,23,33),surface=RGB(23,33,47);
 constexpr COLORREF line=RGB(41,56,75),white=RGB(239,245,250),muted=RGB(148,165,185);
 constexpr COLORREF accent=RGB(100,229,193),ink=RGB(8,36,32),gold=RGB(255,207,112);
 HFONT smallFont=nullptr,bodyFont=nullptr,mediumFont=nullptr,titleFont=nullptr,heroFont=nullptr;
 HBRUSH bgBrush=nullptr,inputBrush=nullptr;
 HWND fields[6]{},profileField=nullptr,profilePathField=nullptr;
 bool favoriteDraft=false;
 HWND gameModeCombo=nullptr,defaultModeCombo=nullptr,sourceCombo=nullptr,collectionCombo=nullptr;
 std::vector<std::wstring> collectionValues;
 HWND profileCombo=nullptr;
 std::vector<std::wstring> profileKeys,profileLabels;
 std::wstring sourceSignature;
 std::vector<std::wstring> sourceGuids;
 int hovered=-1;
 std::wstring editingCover;
 std::wstring captureProfileKey;
 int px(int x) {return x;} // Logical canvas: Windows scales it on high-DPI screens.
 RECT box(int x,int y,int w,int h){return RECT{px(x),px(y),px(x+w),px(y+h)};}
 void fill(HDC dc,RECT r,COLORREF color) {
  SetDCBrushColor(dc,color);FillRect(dc,&r,static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
 }
 void round(HDC dc,RECT r,COLORREF color,COLORREF border=CLR_INVALID,int radius=12) {
  HBRUSH brush=CreateSolidBrush(color);HPEN pen=CreatePen(PS_SOLID,1,border==CLR_INVALID?color:border);
  HGDIOBJ oldBrush=SelectObject(dc,brush),oldPen=SelectObject(dc,pen);
  RoundRect(dc,r.left,r.top,r.right,r.bottom,radius,radius);
  SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(brush);DeleteObject(pen);
 }
 void label(HDC dc,const std::wstring& s,RECT r,HFONT font,COLORREF color=white,UINT flags=DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS) {
  auto old=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);
  DrawTextW(dc,s.c_str(),static_cast<int>(s.size()),&r,flags);SelectObject(dc,old);
 }
 void lineAt(HDC dc,int x,int y,int width,COLORREF color=line){fill(dc,box(x,y,width,1),color);}
 // A white ring with a dark separator remains visible on mint primary buttons,
 // dark surfaces and arbitrary artwork. Native controls use an inset ring so
 // their child-window clipping cannot cut off the focus indicator.
 void focusRing(HDC dc,RECT r,bool inset=false) {
  InflateRect(&r,inset?-4:4,inset?-4:4);
  auto oldBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
  HPEN separator=CreatePen(PS_SOLID,7,RGB(3,8,14));
  HPEN outline=CreatePen(PS_SOLID,3,RGB(255,255,255));
  auto oldPen=SelectObject(dc,separator);
  RoundRect(dc,r.left,r.top,r.right,r.bottom,12,12);
  SelectObject(dc,outline);RoundRect(dc,r.left,r.top,r.right,r.bottom,12,12);
  SelectObject(dc,oldPen);SelectObject(dc,oldBrush);
  DeleteObject(separator);DeleteObject(outline);
 }
 void button(HDC dc,const std::wstring& text,RECT r,int id,bool primary=false,int game=-1) {
  const int hit=static_cast<int>(hits.size());
  const bool focus=keyboardFocus==hit,hover=hovered==hit;
  round(dc,r,primary?accent:(hover?RGB(42,58,77):RGB(31,44,60)),CLR_INVALID,10);
  label(dc,text,r,mediumFont,primary?ink:white,DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
  if(focus)focusRing(dc,r);
  hits.push_back({r,id,game});
 }
 void cover(HDC dc,const Game& g,RECT r,int index) {
  const auto* art=artFor(g.cover);
  if(art) {int saved=SaveDC(dc);
   // GDI regions use device coordinates; a path transforms with the canvas.
   BeginPath(dc);RoundRect(dc,r.left,r.top,r.right+1,r.bottom+1,10,10);EndPath(dc);SelectClipPath(dc,RGN_AND);
   drawArt(dc,*art,r);RestoreDC(dc,saved);}
  else {
   const COLORREF colors[]={RGB(36,73,91),RGB(69,51,95),RGB(78,67,42),RGB(36,77,66),RGB(71,45,63)};
   round(dc,r,colors[static_cast<size_t>(std::max(index,0))%5]);
   // A typographic fallback keeps the library useful before covers are imported.
   const int w=r.right-r.left,h=r.bottom-r.top;
   lineAt(dc,r.left+18,r.top+22,w-36,RGB(112,137,154));
   label(dc,L"FEATHERPAD",box(r.left+18,r.top+34,w-36,16),smallFont,RGB(190,205,218));
   label(dc,g.name.substr(0,1),box(r.left+16,r.top+h/4,w-32,h/3),heroFont,white,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
   label(dc,g.name,box(r.left+18,r.bottom-65,w-36,46),mediumFont,white,DT_LEFT|DT_WORDBREAK|DT_END_ELLIPSIS);
  }
 }
 HWND control(HWND owner,const wchar_t* type,const std::wstring& text,DWORD style,RECT r,int id) {
  HWND c=CreateWindowExW(0,type,text.c_str(),WS_CHILD|WS_VISIBLE|style,r.left,r.top,r.right-r.left,r.bottom-r.top,owner,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),instance,nullptr);
  SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(bodyFont),TRUE);return c;
 }
 HWND edit(HWND owner,const std::wstring& value,RECT r,int id,int limit=8000) {
  HWND h=control(owner,L"EDIT",value,WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,r,id);
  SendMessageW(h,EM_SETLIMITTEXT,limit,0);return h;
 }
 HWND nativeButton(HWND owner,const std::wstring& value,RECT r,int id) {
  return control(owner,L"BUTTON",value,WS_TABSTOP|BS_OWNERDRAW,r,id);
 }
 void staticText(HWND owner,const std::wstring& value,RECT r,COLORREF /*color*/=muted) {
  control(owner,L"STATIC",value,0,r,0);
 }
 HWND ownedWindow(const wchar_t* className,const wchar_t* title,int width,int height,HWND owner) {
  RECT r{};GetWindowRect(owner,&r);RECT area{0,0,width,height};
  AdjustWindowRectEx(&area,WS_CAPTION|WS_SYSMENU,FALSE,WS_EX_DLGMODALFRAME);
  RECT desktop{};SystemParametersInfoW(SPI_GETWORKAREA,0,&desktop,0);
  const int w=area.right-area.left,h=area.bottom-area.top;
  const int x=std::max(static_cast<int>(desktop.left),std::min(static_cast<int>(desktop.right)-w,static_cast<int>(r.left+(r.right-r.left-w)/2)));
  const int y=std::max(static_cast<int>(desktop.top),std::min(static_cast<int>(desktop.bottom)-h,static_cast<int>(r.top+(r.bottom-r.top-h)/2)));
  HWND hwin=CreateWindowExW(WS_EX_DLGMODALFRAME,className,title,WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN,x,y,w,h,owner,nullptr,instance,nullptr);
  darkTitle(hwin);return hwin;
 }
 bool chooseImage(HWND owner,std::wstring& result) {
  wchar_t path[32768]{};OPENFILENAMEW of{};of.lStructSize=sizeof(of);of.hwndOwner=owner;of.lpstrFile=path;of.nMaxFile=32768;
  of.lpstrFilter=L"Imagens (PNG, JPG, BMP)\0*.png;*.jpg;*.jpeg;*.bmp\0\0";of.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
  if(!GetOpenFileNameW(&of))return false;
  if(!validateArt(path)){MessageBoxW(owner,L"Imagem inválida, corrompida ou maior que 2048 × 2048 pixels / 8 MB.",L"Escolha outra imagem",MB_ICONWARNING);return false;}
  result=path;return true;
 }
 void browseGame() {
  wchar_t path[32768]{};OPENFILENAMEW of{};of.lStructSize=sizeof(of);of.hwndOwner=panel;of.lpstrFile=path;of.nMaxFile=32768;
  of.lpstrFilter=L"Jogos e atalhos\0*.exe;*.lnk;*.url\0Todos os arquivos\0*.*\0\0";of.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
  if(!GetOpenFileNameW(&of))return;
  SetWindowTextW(fields[1],path);std::wstring p=path;const auto slash=p.find_last_of(L"\\/");
  if(slash!=std::wstring::npos){SetWindowTextW(fields[3],p.substr(0,slash).c_str());if(textOf(fields[0]).empty())SetWindowTextW(fields[0],p.substr(slash+1).c_str());}
 }
 void saveEditor() {
  Game g;g.name=textOf(fields[0]);g.categories=textOf(fields[6]);g.path=textOf(fields[1]);g.args=textOf(fields[2]);g.dir=textOf(fields[3]);g.note=textOf(fields[4]);g.cover=editingCover;
  g.favorite=favoriteDraft;
  if(editorIndex>=0&&editorIndex<static_cast<int>(games.size())){g.id=games[editorIndex].id;g.launches=games[editorIndex].launches;g.lastLaunch=games[editorIndex].lastLaunch;g.playedSeconds=games[editorIndex].playedSeconds;}
  const auto chosen=SendMessageW(gameModeCombo,CB_GETCURSEL,0,0);
  g.controllerMode=chosen>=0&&chosen<=3?static_cast<ControllerMode>(chosen):ControllerMode::Auto;
  if(g.name.empty()||g.path.empty()){MessageBoxW(panel,L"Preencha o nome e escolha o executável ou atalho do jogo.",L"Faltam informações",MB_ICONINFORMATION);return;}
  for(const auto* s:{&g.name,&g.path,&g.args,&g.dir,&g.note,&g.cover,&g.categories})if(!validField(*s)){MessageBoxW(panel,L"Os campos não podem conter quebras de linha nem ultrapassar 8000 caracteres.",L"Confira os campos",MB_ICONWARNING);return;}
  if(g.path.front()==L'"'||g.path.back()==L'"'){MessageBoxW(panel,L"Informe o caminho do jogo sem aspas externas. Use aspas somente nos argumentos, se necessário.",L"Confira o caminho",MB_ICONWARNING);return;}
  if(!g.cover.empty()&&((editorIndex<0)||g.cover!=games[editorIndex].cover))if(!importArt(g.cover,panel))return;
  const auto backup=games;int index=editorIndex;
  if(index<0){if(games.size()>=2000){MessageBoxW(panel,L"Limite de 2000 jogos atingido.",L"FeatherPad",MB_ICONWARNING);return;}games.push_back(g);index=static_cast<int>(games.size())-1;}
  else games[index]=g;
  if(!saveDatabase(panel)){games=backup;return;}
  closePanel();rebuildLibrary(index);status(L"Jogo salvo na sua biblioteca.");
 }
 void paintCapture(HDC dc) {
  RECT client{};GetClientRect(captureWin,&client);fill(dc,client,bg);
  round(dc,box(28,26,54,54),surface);
  label(dc,captureCandidate?L"✓":L"…",box(28,26,54,54),titleFont,accent,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  std::wstring title;
  if(captureAction==A_CAPTURE_OPEN)title=L"Abrir o launcher";
  else if(captureAction==A_CAPTURE_OK)title=L"Confirmar / jogar";
  else if(captureAction==A_CAPTURE_BACK)title=L"Voltar / ocultar";
  else title=L"Favoritar jogo";
  label(dc,title,box(100,22,392,30),titleFont);
  label(dc,captureCandidate?L"Botão identificado":L"Aguardando seu controle",box(100,54,392,24),bodyFont,accent);
  label(dc,captureNotice,box(28,106,478,105),bodyFont,white,DT_LEFT|DT_WORDBREAK);
  label(dc,captureCandidate?L"Clique em Salvar botão ou pressione Enter. Esc cancela.":L"Solte os botões. Depois, pressione somente o botão desejado.",box(28,218,478,48),smallFont,muted,DT_LEFT|DT_WORDBREAK);
 }
}
void initTheme() {
 auto make=[](int size,int weight){return CreateFontW(-size,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");};
 smallFont=make(12,FW_NORMAL);bodyFont=make(15,FW_NORMAL);mediumFont=make(15,FW_SEMIBOLD);titleFont=make(23,FW_SEMIBOLD);heroFont=make(38,FW_BOLD);
 bgBrush=CreateSolidBrush(bg);inputBrush=CreateSolidBrush(surface);
}
void destroyTheme(){for(HFONT f:{smallFont,bodyFont,mediumFont,titleFont,heroFont})if(f)DeleteObject(f);if(bgBrush)DeleteObject(bgBrush);if(inputBrush)DeleteObject(inputBrush);}
void darkTitle(HWND h){BOOL dark=TRUE;DwmSetWindowAttribute(h,20,&dark,sizeof(dark));DwmSetWindowAttribute(h,19,&dark,sizeof(dark));}
void status(const std::wstring& message){notice=message;if(panel&&GetDlgItem(panel,451))SetWindowTextW(GetDlgItem(panel,451),message.c_str());if(win&&IsWindowVisible(win))InvalidateRect(win,nullptr,FALSE);}
void show(){
 bridgeSuspendForUI();
 // SW_RESTORE also unmaximizes a visible window. Only restore minimized ones;
 // SW_SHOW keeps both maximized geometry and the user's normal window size.
 ShowWindow(win,IsIconic(win)?SW_RESTORE:SW_SHOW);
 SetForegroundWindow(win);SetFocus(captureWin?captureWin:(panel?panel:win));InvalidateRect(win,nullptr,FALSE);
}
void hide(){if(panel||captureWin)return;bridgeSuspendForUI();ShowWindow(win,SW_HIDE);clearArt();}

void paintMain(HDC dc,RECT client) {
 const int w=client.right,h=client.bottom;hits.clear();
 fill(dc,client,bg);fill(dc,box(0,0,208,h),sidebar);
 round(dc,box(26,28,34,34),accent,CLR_INVALID,9);
 label(dc,L"F",box(26,28,34,34),titleFont,ink,DT_CENTER|DT_SINGLELINE|DT_VCENTER);
 label(dc,L"FeatherPad",box(72,26,126,38),mediumFont);
 label(dc,L"SEU ESPAÇO DE JOGOS",box(27,78,166,22),smallFont,muted);
 button(dc,L"Biblioteca",box(18,129,172,45),A_ALL,!favoritesOnly);
 button(dc,L"★   Favoritos",box(18,184,172,45),A_FAVORITES,favoritesOnly);
 const auto favs=std::count_if(games.begin(),games.end(),[](const Game& g){return g.favorite;});
 label(dc,std::to_wstring(games.size())+L" jogos   ·   "+std::to_wstring(favs)+L" favoritos",box(27,246,166,22),smallFont,muted);
 button(dc,L"+  Adicionar jogo",box(18,285,172,43),A_ADD);
 button(dc,searchQuery.empty()?L"Pesquisar jogos":L"Busca: "+searchQuery,box(18,340,172,43),A_SEARCH);
 button(dc,fullscreen?L"Sair da tela cheia":L"Tela cheia / TV",box(18,395,172,43),A_FULLSCREEN);
 button(dc,collectionFilter.empty()?L"Coleções":collectionFilter,box(18,450,172,36),A_COLLECTIONS);
 if(!searchQuery.empty())button(dc,L"Limpar pesquisa",box(18,495,172,36),A_CLEAR_SEARCH);
 label(dc,L"SEM PRESSA. SÓ JOGUE.",box(27,h-200,166,22),smallFont,muted);
 button(dc,L"Configurações",box(18,h-158,172,43),A_SETTINGS);
 lineAt(dc,18,h-99,172);
 RECT avatar=box(25,h-77,42,42);round(dc,avatar,surface);
 if(const auto* a=artFor(avatarPath))drawArt(dc,*a,avatar);
 else label(dc,profileName.substr(0,1),avatar,mediumFont,accent,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 label(dc,profileName,box(78,h-81,108,28),mediumFont);
 label(dc,L"Perfil local  ›",box(78,h-53,108,20),smallFont,muted);
 if(keyboardFocus==static_cast<int>(hits.size()))focusRing(dc,box(18,h-88,172,65));
 hits.push_back({box(18,h-88,172,65),A_PROFILE});
 const int x=242,cw=w-x-34;
 label(dc,L"BIBLIOTECA PESSOAL",box(x,25,cw-250,22),smallFont,accent);
 label(dc,L"Hora de jogar.",box(x,55,cw-250,52),heroFont);
 const bool connected=!devices.empty()||bridgeOwnsUI();
 round(dc,box(w-250,30,216,34),surface);
 label(dc,connected?L"●  Controle conectado":L"○  Conecte seu controle",box(w-240,30,196,34),smallFont,connected?accent:muted);
 label(dc,systemMetrics(),box(x,106,cw,20),smallFont,muted);
 const int heroY=130,heroH=230;
 RECT hero=box(x,heroY,cw,heroH);round(dc,hero,surface,line,18);
 if(selection>=0&&selection<static_cast<int>(games.size())) {
  const auto& g=games[selection];
  label(dc,g.favorite?L"★  ENTRE OS SEUS FAVORITOS":L"PRONTO PARA A PRÓXIMA PARTIDA",box(x+26,heroY+22,cw-210,22),smallFont,g.favorite?gold:accent);
  label(dc,g.name,box(x+26,heroY+55,cw-224,56),heroFont);
  const auto subtitle=g.note.empty()?L"Sua biblioteca, seu controle. Escolha e comece a jogar.":g.note;
  label(dc,subtitle,box(x+26,heroY+112,cw-232,44),bodyFont,muted,DT_LEFT|DT_WORDBREAK|DT_END_ELLIPSIS);
  button(dc,L"▶  Jogar agora",box(x+26,heroY+173,154,38),A_PLAY,true);
  button(dc,g.favorite?L"★ Favorito":L"☆ Favoritar",box(x+192,heroY+173,124,38),A_STAR);
  button(dc,L"Editar",box(x+328,heroY+173,82,38),A_EDIT);
  button(dc,L"Detalhes",box(x+420,heroY+173,96,38),A_GAME_DETAILS);
  cover(dc,g,box(w-202,heroY+20,142,190),selection);
 } else {
  label(dc,favoritesOnly?L"Seus favoritos ficam aqui.":L"Sua próxima aventura começa aqui.",box(x+28,heroY+38,cw-56,62),titleFont);
  label(dc,favoritesOnly?L"Marque uma estrela na biblioteca para encontrar seus jogos preferidos.":L"Adicione seu primeiro jogo e escolha uma capa para deixar tudo com a sua cara.",box(x+28,heroY+109,cw-56,42),bodyFont,muted,DT_WORDBREAK);
  button(dc,favoritesOnly?L"Ver biblioteca":L"+ Adicionar jogo",box(x+28,heroY+173,180,38),favoritesOnly?A_ALL:A_ADD,true);
 }
 const int rowTitle=386,rowY=434;
 label(dc,favoritesOnly?L"Seus favoritos":L"Todos os jogos",box(x,rowTitle,320,30),titleFont);
 label(dc,std::to_wstring(visibleGames.size())+L" na coleção",box(x+330,rowTitle,cw-440,30),smallFont,muted);
 button(dc,L"‹",box(w-116,rowTitle,36,30),A_PREV);button(dc,L"›",box(w-70,rowTitle,36,30),A_NEXT);
 const int columns=std::max(3,std::min(5,(cw+16)/160));
 const int cardW=(cw-(columns-1)*16)/columns;
 const int cardH=std::min(220,std::max(126,h-rowY-115));
 auto position=std::find(visibleGames.begin(),visibleGames.end(),selection);
 const int pos=position==visibleGames.end()?0:static_cast<int>(position-visibleGames.begin());
 if(pos<firstVisible)firstVisible=pos;
 if(pos>=firstVisible+columns)firstVisible=pos-columns+1;
 firstVisible=std::max(0,std::min(firstVisible,std::max(0,static_cast<int>(visibleGames.size())-columns)));
 for(int j=0;j<columns&&firstVisible+j<static_cast<int>(visibleGames.size());++j) {
  const int index=visibleGames[firstVisible+j],left=x+j*(cardW+16);
  RECT r=box(left,rowY,cardW,cardH);const auto& g=games[index];
  if(index==selection){RECT border=r;InflateRect(&border,2,2);round(dc,border,accent,accent,12);}
  cover(dc,g,r,index);
  if(g.favorite){round(dc,box(left+cardW-32,rowY+8,24,24),sidebar);label(dc,L"★",box(left+cardW-32,rowY+8,24,24),bodyFont,gold,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
  if(keyboardFocus==static_cast<int>(hits.size())||(keyboardFocus<0&&index==selection))focusRing(dc,r);
  hits.push_back({r,0,index});
  label(dc,g.name,box(left,rowY+cardH+12,cardW,24),mediumFont,index==selection?accent:white);
 }
 if(visibleGames.empty())label(dc,L"Nenhum jogo encontrado. Limpe a pesquisa ou adicione um jogo.",box(x,rowY,cw,120),bodyFont,muted,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 lineAt(dc,x,h-55,cw);
 label(dc,L"← →  Jogos     ↑ ↓  Ações     Enter  Confirmar     F  Favoritar     S  Configurações     Esc  Ocultar",box(x,h-47,cw,22),smallFont,muted);
 label(dc,notice.empty()?L"Seu launcher pessoal. Sem distrações.":notice,box(x,h-23,cw,18),smallFont,muted);
}
void drawButton(const DRAWITEMSTRUCT* item) {
 const bool toggle=item->CtlID==A_STARTUP||item->CtlID==A_AUTOOPEN||item->CtlID==A_DRAFT_FAVORITE||item->CtlID==A_COMPAT_TOGGLE;
 if(toggle) {
  const bool checked=item->CtlID==A_STARTUP?startupEnabled():item->CtlID==A_AUTOOPEN?autoOpen:item->CtlID==A_COMPAT_TOGGLE?controllerCompatibility:favoriteDraft;
  round(item->hDC,item->rcItem,surface,CLR_INVALID,8);
  const int middle=(item->rcItem.top+item->rcItem.bottom)/2;
  RECT mark=box(item->rcItem.left+10,middle-9,18,18);
  round(item->hDC,mark,checked?accent:bg,checked?accent:muted,4);
  if(checked)label(item->hDC,L"✓",mark,smallFont,ink,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  RECT caption=item->rcItem;caption.left+=40;
  label(item->hDC,textOf(item->hwndItem),caption,bodyFont,white);
  if(item->itemState&ODS_FOCUS)focusRing(item->hDC,item->rcItem,true);
  return;
 }

 const bool primary=item->CtlID==A_PLAY||item->CtlID==A_SAVE_GAME||item->CtlID==A_SAVE_PROFILE||item->CtlID==A_CAPTURE_SAVE;
 const bool disabled=(item->itemState&ODS_DISABLED)!=0;
 const bool pressed=(item->itemState&ODS_SELECTED)!=0;
 round(item->hDC,item->rcItem,disabled?surface:(primary?accent:(pressed?line:RGB(34,48,65))),CLR_INVALID,10);
 HFONT font=GetParent(item->hwndItem)==keyboardWindow?reinterpret_cast<HFONT>(SendMessageW(item->hwndItem,WM_GETFONT,0,0)):mediumFont;
 label(item->hDC,textOf(item->hwndItem),item->rcItem,font,disabled?muted:(primary?ink:white),DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
 if(!disabled&&(item->itemState&ODS_FOCUS))focusRing(item->hDC,item->rcItem,true);
}
void openPage(Page requested,int gameIndex) {
 if(panel){SetForegroundWindow(captureWin?captureWin:panel);return;}
 show();page=requested;editorIndex=gameIndex;std::fill(std::begin(fields),std::end(fields),nullptr);
 const wchar_t* title=requested==Page::Editor?(gameIndex<0?L"Adicionar jogo":L"Editar jogo"):requested==Page::Profile?L"Seu perfil":requested==Page::Diagnostic?L"Controles e jogadores":requested==Page::Compatibility?L"Compatibilidade dos jogos":requested==Page::Collections?L"Coleções":requested==Page::Statistics?L"Estatísticas do jogo":L"Configurações";
 const int height=requested==Page::Editor?730:requested==Page::Profile?340:requested==Page::Diagnostic?300:requested==Page::Compatibility?580:640;
 panel=ownedWindow(L"FeatherPadPanel",title,650,height,win);if(!panel){page=Page::None;return;}
 EnableWindow(win,FALSE);
 if(requested==Page::Editor) {
  Game g;if(gameIndex>=0&&gameIndex<static_cast<int>(games.size()))g=games[gameIndex];editingCover=g.cover;
  staticText(panel,gameIndex<0?L"Um novo jogo na sua coleção":L"Deixe este jogo com a sua cara",box(26,20,598,30));
  const wchar_t* labels[]={L"Nome do jogo",L"Executável, atalho ou URI",L"Argumentos (opcional)",L"Pasta de trabalho (opcional)",L"Descrição curta (opcional)"};
  const std::wstring* values[]={&g.name,&g.path,&g.args,&g.dir,&g.note};
  for(int i=0;i<5;++i){const int y=64+i*59;staticText(panel,labels[i],box(26,y,598,22));fields[i]=edit(panel,*values[i],box(26,y+26,i==1?462:598,30),400+i,i==4?160:8000);}
  nativeButton(panel,L"Procurar...",box(500,154,124,30),A_BROWSE_GAME);
  staticText(panel,L"Capa do jogo (PNG, JPG ou BMP)",box(26,365,598,22));
  fields[6]=edit(panel,g.categories,box(26,391,598,30),406,240);
  staticText(panel,L"Categorias / coleções (separe por ponto e vírgula)",box(26,430,598,22));
  fields[5]=control(panel,L"EDIT",editingCover.empty()?L"Sem capa — será usada uma arte com o nome do jogo":editingCover,WS_BORDER|ES_AUTOHSCROLL|ES_READONLY,box(26,454,598,30),405);
  nativeButton(panel,L"Escolher capa...",box(26,496,160,33),A_BROWSE_COVER);nativeButton(panel,L"Remover capa",box(198,496,148,33),A_CLEAR_COVER);
  favoriteDraft=g.favorite;nativeButton(panel,L"Adicionar aos favoritos",box(365,496,259,33),A_DRAFT_FAVORITE);
  staticText(panel,L"Controle para este jogo",box(26,542,204,24));
  gameModeCombo=control(panel,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,box(242,538,382,150),440);
  for(const wchar_t* text:{L"Usar padrão global",L"Nativo — sem conversão",L"Xbox 360 virtual",L"PlayStation 4 virtual"})SendMessageW(gameModeCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
  SendMessageW(gameModeCombo,CB_SETCURSEL,static_cast<WPARAM>(g.controllerMode),0);
  staticText(panel,L"O padrão é aplicado sem configurar cada jogo. Ative a compatibilidade nas configurações. A capa é copiada para o perfil.",box(26,576,598,44));
  if(gameIndex>=0)nativeButton(panel,L"Excluir da lista",box(26,648,148,34),A_DELETE_GAME);
  nativeButton(panel,L"Cancelar",box(350,648,124,34),A_CLOSE_PANEL);nativeButton(panel,L"Salvar jogo",box(486,648,138,34),A_SAVE_GAME);
 } else if(requested==Page::Collections) {
  staticText(panel,L"Filtre a biblioteca por uma categoria já usada nos seus jogos.",box(26,24,598,38));
  collectionCombo=control(panel,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,box(26,90,598,230),460);
  collectionValues={L"",L"Corrida",L"Multiplayer",L"Campanha",L"Emuladores"};
  for(const auto& g:games)for(const auto& c:splitCollections(g.categories))if(std::none_of(collectionValues.begin(),collectionValues.end(),[&](const auto& v){return CompareStringOrdinal(v.c_str(),-1,c.c_str(),-1,TRUE)==CSTR_EQUAL;}))collectionValues.push_back(c);
  SendMessageW(collectionCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Todas as coleções"));int selected=0;
  for(size_t i=1;i<collectionValues.size();++i){SendMessageW(collectionCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(collectionValues[i].c_str()));if(collectionValues[i]==collectionFilter)selected=static_cast<int>(i);}
  SendMessageW(collectionCombo,CB_SETCURSEL,selected,0);
  staticText(panel,L"Adicione ou edite coleções no campo Categorias ao editar cada jogo. Um jogo pode pertencer a várias coleções.",box(26,350,598,58));
  nativeButton(panel,L"Cancelar",box(350,590,124,34),A_CLOSE_PANEL);nativeButton(panel,L"Aplicar filtro",box(486,590,138,34),A_SAVE_COLLECTION);
 } else if(requested==Page::Statistics) {
  staticText(panel,L"ESTATÍSTICAS DO JOGO",box(26,24,598,28));
  staticText(panel,gameStatistics(static_cast<size_t>(std::max(0,gameIndex))),box(26,68,598,280));
  nativeButton(panel,L"Concluir",box(486,590,138,34),A_CLOSE_PANEL);
 } else if(requested==Page::Settings) {
  profileKeys.clear();profileLabels.clear();
  for(const auto& item:bridgeSources()){if(profileKeys.size()>=4)break;profileKeys.push_back(item.key);profileLabels.push_back(item.name);}
  for(const auto& entry:devices){if(profileKeys.size()>=4)break;const auto& key=entry.second.path;if(key.empty()||std::find(profileKeys.begin(),profileKeys.end(),key)!=profileKeys.end())continue;const auto slash=key.find_last_of(L"\\/#");profileKeys.push_back(key);profileLabels.push_back(slash==std::wstring::npos?key:key.substr(slash+1));}
  if(!profileKeys.empty()){const auto current=currentControllerProfile();auto it=std::find(profileKeys.begin(),profileKeys.end(),current);useControllerProfile(it==profileKeys.end()?profileKeys.front():*it);}
  staticText(panel,L"CONTROLE",box(26,20,598,24));
  staticText(panel,L"Escolha uma ação. Um pop-up vai aguardar o botão que você quiser usar.",box(26,52,598,42));
  const wchar_t* labels[]={L"Abrir / ocultar launcher",L"Confirmar / jogar",L"Voltar / ocultar",L"Favoritar jogo"};
  const unsigned bindings[]={*profileBinding(A_CAPTURE_OPEN),*profileBinding(A_CAPTURE_OK),*profileBinding(A_CAPTURE_BACK),*profileBinding(A_CAPTURE_STAR)};
  for(int i=0;i<4;++i){const int y=106+i*58;staticText(panel,labels[i],box(26,y,306,34));nativeButton(panel,L"Botão "+std::to_wstring(bindings[i])+L"  ·  Alterar",box(350,y,274,34),A_CAPTURE_OPEN+i);}
  staticText(panel,L"Perfil físico",box(26,338,132,30));
  profileCombo=control(panel,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,box(154,336,262,150),A_PLAYER_PROFILE);
  if(profileKeys.empty()){SendMessageW(profileCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Conecte um controle"));SendMessageW(profileCombo,CB_SETCURSEL,0,0);}
  else {for(const auto& name:profileLabels)SendMessageW(profileCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name.c_str()));auto selected=std::find(profileKeys.begin(),profileKeys.end(),currentControllerProfile());SendMessageW(profileCombo,CB_SETCURSEL,selected==profileKeys.end()?0:static_cast<int>(selected-profileKeys.begin()),0);}
  nativeButton(panel,L"Zona morta: "+std::to_wstring(profileDeadzone()),box(430,336,194,34),A_CALIBRATE);EnableWindow(GetDlgItem(panel,A_CALIBRATE),!profileKeys.empty());
  nativeButton(panel,L"Controles conectados e jogadores locais",box(26,382,598,34),A_DIAG);
  nativeButton(panel,L"Compatibilidade dos jogos...",box(26,426,598,34),A_COMPAT);
  nativeButton(panel,L"Iniciar com o Windows (em segundo plano)",box(26,470,598,28),A_STARTUP);
  nativeButton(panel,L"Mostrar a biblioteca ao conectar o controle",box(26,504,598,28),A_AUTOOPEN);
  nativeButton(panel,L"Buscar atualizações",box(26,545,278,34),A_UPDATES);
  nativeButton(panel,L"Tela cheia / TV (F11)",box(318,545,306,34),A_FULLSCREEN);
  control(panel,L"STATIC",L"RAM, bateria e rede aparecem na tela principal. Acompanhe o tempo em Detalhes do jogo.",0,box(26,585,440,40),451);
  nativeButton(panel,L"Concluir",box(486,585,138,34),A_CLOSE_PANEL);
 } else if(requested==Page::Compatibility) {
  staticText(panel,L"Seu controle físico pode aparecer para os jogos como Xbox ou PlayStation. Você escolhe um padrão; exceções ficam salvas no jogo.",box(26,22,598,48));
  nativeButton(panel,L"Ativar compatibilidade de controles",box(26,82,598,38),A_COMPAT_TOGGLE);
  control(panel,L"STATIC",bridgeStatus(),0,box(26,138,598,68),430);
  staticText(panel,L"Controle de origem (um jogador)",box(26,222,598,22));
  sourceCombo=control(panel,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,box(26,250,598,150),A_COMPAT_SOURCE);
  staticText(panel,L"Formato padrão enviado aos jogos",box(26,298,598,22));
  defaultModeCombo=control(panel,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,box(26,326,598,150),A_COMPAT_DEFAULT);
  for(const wchar_t* text:{L"Nativo — usa o controle físico",L"Xbox 360 — compatibilidade XInput",L"PlayStation 4 — DualShock 4 virtual"})SendMessageW(defaultModeCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
  SendMessageW(defaultModeCombo,CB_SETCURSEL,static_cast<int>(defaultControllerMode)-1,0);
  nativeButton(panel,L"Verificar novamente",box(26,389,190,34),A_COMPAT_RETRY);
  nativeButton(panel,L"Driver necessário",box(228,389,190,34),A_COMPAT_DRIVER);
  nativeButton(panel,L"Evitar entrada dupla",box(430,389,194,34),A_COMPAT_HIDHIDE);
  nativeButton(panel,L"Importar mapeamentos SDL...",box(26,438,300,34),A_COMPAT_MAPPINGS);
  staticText(panel,L"Saídas virtuais exigem ViGEmBus 1.22.0 (projeto aposentado). Instalação única pelo link oficial. Recursos avançados de DualSense não são emulados.",box(26,492,430,70));
  nativeButton(panel,L"Concluir",box(486,530,138,34),A_CLOSE_PANEL);
  sourceSignature.clear();refreshBridgePanel();
 } else if(requested==Page::Profile) {
  staticText(panel,L"Seu perfil fica somente neste computador. Sem conta ou login.",box(26,24,598,28));
  staticText(panel,L"Como você quer aparecer?",box(26,76,598,22));profileField=edit(panel,profileName,box(26,105,598,32),410,40);
  staticText(panel,L"Imagem do perfil (opcional)",box(26,158,598,22));
  editingCover=avatarPath;profilePathField=control(panel,L"EDIT",avatarPath.empty()?L"Sem imagem — usando a sua inicial":avatarPath,WS_BORDER|ES_READONLY|ES_AUTOHSCROLL,box(26,186,598,30),411);
  nativeButton(panel,L"Escolher imagem",box(26,229,162,34),A_BROWSE_AVATAR);nativeButton(panel,L"Remover",box(200,229,110,34),A_CLEAR_AVATAR);
  nativeButton(panel,L"Cancelar",box(350,288,124,34),A_CLOSE_PANEL);nativeButton(panel,L"Salvar perfil",box(486,288,138,34),A_SAVE_PROFILE);
 } else {
  staticText(panel,L"Controles conectados e posições dos jogadores",box(26,24,598,26));
  staticText(panel,bridgePlayerSummary(),box(26,62,598,26));
  std::wstring names;unsigned player=1;for(const auto& item:bridgeSources()){if(player>4)break;if(!names.empty())names+=L"\r\n";names+=L"P"+std::to_wstring(player++)+L" · "+item.name;}
  staticText(panel,names.empty()?L"Conecte os controles com compatibilidade ativa para ver as posições.":names,box(26,94,598,54));
  staticText(panel,L"Pressione um controle por vez para conferir as entradas. No modo de compatibilidade, até quatro saídas virtuais independentes podem ser encaminhadas aos jogos.",box(26,156,598,48));
  // The diagnostic output has its own ID, separate from decorative labels.
  control(panel,L"STATIC",L"Aguardando uma entrada do controle…",0,box(26,132,598,64),420);
  nativeButton(panel,L"Concluir",box(486,246,138,34),A_CLOSE_PANEL);
 }
 ShowWindow(panel,SW_SHOW);SetForegroundWindow(panel);SetFocus(GetNextDlgTabItem(panel,nullptr,FALSE));
}
void closePanel() {
 if(keyboardWindow)closeVirtualKeyboard(false);
 if(captureWin)closeCapture();
 if(!panel)return;
 HWND closing=panel;panel=nullptr;page=Page::None;
 EnableWindow(win,TRUE);DestroyWindow(closing);clearArt();SetForegroundWindow(win);SetFocus(win);InvalidateRect(win,nullptr,FALSE);
}
void beginCapture(int requested) {
 if(captureWin)return;
 captureAction=requested;captureCandidate=0;captureArmed.clear();captureProfileKey.clear();
 if(page==Page::Settings&&!profileKeys.empty()){const auto selected=SendMessageW(profileCombo,CB_GETCURSEL,0,0);if(selected>=0&&selected<static_cast<LRESULT>(profileKeys.size()))captureProfileKey=profileKeys[static_cast<size_t>(selected)];}
 if(captureProfileKey.empty())captureProfileKey=currentControllerProfile();
 captureNotice=L"Pressione no controle o botão que vai "+std::wstring(requested==A_CAPTURE_OPEN?L"abrir o launcher.":requested==A_CAPTURE_OK?L"confirmar a seleção e iniciar um jogo.":requested==A_CAPTURE_BACK?L"voltar ou ocultar o launcher.":L"adicionar ou remover um jogo dos favoritos.");
 if(devices.empty()&&!selfTesting)captureNotice+=L"\n\nNenhum controle detectado. Conecte por Bluetooth ou USB para continuar.";
 captureWin=ownedWindow(L"FeatherPadCapture",L"Configurar botão do controle",534,326,panel?panel:win);
 if(!captureWin){captureAction=0;return;}
 EnableWindow(panel?panel:win,FALSE);
 nativeButton(captureWin,L"Cancelar",box(232,274,124,34),A_CAPTURE_CANCEL);
 HWND save=nativeButton(captureWin,L"Salvar botão",box(368,274,138,34),A_CAPTURE_SAVE);EnableWindow(save,FALSE);
 ShowWindow(captureWin,SW_SHOW);SetForegroundWindow(captureWin);SetFocus(GetDlgItem(captureWin,A_CAPTURE_CANCEL));
}
void closeCapture() {
 if(!captureWin)return;
 HWND closing=captureWin;captureWin=nullptr;captureAction=0;captureCandidate=0;captureArmed.clear();captureProfileKey.clear();
 EnableWindow(panel?panel:win,TRUE);DestroyWindow(closing);SetForegroundWindow(panel?panel:win);SetFocus(panel?panel:win);
}
void commitCapture() {
 if(!captureWin||!captureCandidate)return;
 if(!captureProfileKey.empty())useControllerProfile(captureProfileKey);
 unsigned* binding=profileBinding(captureAction);
 const unsigned previous=*binding;*binding=captureCandidate;
 if(!saveDatabase(captureWin)){*binding=previous;return;}
 if(panel)SetWindowTextW(GetDlgItem(panel,captureAction),(L"Botão "+std::to_wstring(*binding)+L"  ·  Alterar").c_str());
 closeCapture();status(L"Botão configurado e salvo.");
}
void action(int id) {
 switch(id) {
 case A_OPEN:show();break;
 case A_EXIT:if(panel)closePanel();DestroyWindow(win);break;
 case A_SETTINGS:openPage(Page::Settings);break;
 case A_COMPAT:closePanel();openPage(Page::Compatibility);break;
 case A_SEARCH:openVirtualKeyboard();break;
 case A_CLEAR_SEARCH:searchQuery.clear();rebuildLibrary(selection);break;
 case A_FULLSCREEN:toggleFullscreen();break;
 case A_COLLECTIONS:openPage(Page::Collections);break;
 case A_GAME_DETAILS:if(selection>=0)openPage(Page::Statistics,selection);break;
 case A_SAVE_COLLECTION:{const auto chosen=SendMessageW(collectionCombo,CB_GETCURSEL,0,0);if(chosen>=0&&chosen<static_cast<LRESULT>(collectionValues.size())){collectionFilter=collectionValues[static_cast<size_t>(chosen)];closePanel();rebuildLibrary();}break;}
 case A_UPDATES:checkForUpdates();break;
 case A_ALL:setFilter(false);break;case A_FAVORITES:setFilter(true);break;
 case A_ADD:openPage(Page::Editor);break;
 case A_EDIT:if(selection>=0)openPage(Page::Editor,selection);break;
 case A_PLAY:launch();break;case A_STAR:toggleFavorite();break;
 case A_PROFILE:openPage(Page::Profile);break;
 case A_PREV:moveSelection(-1);break;case A_NEXT:moveSelection(1);break;
 case A_CLOSE_PANEL:closePanel();break;
 case A_SAVE_GAME:saveEditor();break;
 case A_BROWSE_GAME:browseGame();break;
 case A_BROWSE_COVER:if(chooseImage(panel,editingCover))SetWindowTextW(fields[5],editingCover.c_str());break;
 case A_CLEAR_COVER:editingCover.clear();SetWindowTextW(fields[5],L"Sem capa — será usada uma arte com o nome do jogo");break;
 case A_DELETE_GAME:
  if(editorIndex>=0&&MessageBoxW(panel,L"Remover este jogo da biblioteca? Os arquivos do jogo serão preservados.",L"Remover jogo",MB_YESNO|MB_ICONQUESTION)==IDYES) {
   auto backup=games;games.erase(games.begin()+editorIndex);
   if(saveDatabase(panel)){closePanel();rebuildLibrary();}else games=std::move(backup);
  }break;
 case A_STARTUP:toggleStartup();InvalidateRect(GetDlgItem(panel,A_STARTUP),nullptr,FALSE);break;
 case A_AUTOOPEN:autoOpen=!autoOpen;if(!saveDatabase(panel))autoOpen=!autoOpen;InvalidateRect(GetDlgItem(panel,A_AUTOOPEN),nullptr,FALSE);break;
 case A_DRAFT_FAVORITE:favoriteDraft=!favoriteDraft;InvalidateRect(GetDlgItem(panel,A_DRAFT_FAVORITE),nullptr,FALSE);break;
 case A_CAPTURE_OPEN:case A_CAPTURE_OK:case A_CAPTURE_BACK:case A_CAPTURE_STAR:beginCapture(id);break;
 case A_CAPTURE_SAVE:commitCapture();break;case A_CAPTURE_CANCEL:closeCapture();break;
 case A_DIAG:closePanel();openPage(Page::Diagnostic);break;
 case A_CALIBRATE:{const auto selected=SendMessageW(profileCombo,CB_GETCURSEL,0,0);if(selected<0||selected>=static_cast<LRESULT>(profileKeys.size()))break;useControllerProfile(profileKeys[static_cast<size_t>(selected)]);const int old=profileDeadzone();const int next=old<4000?8000:old<8000?12000:old<12000?16000:0;setProfileDeadzone(next);if(!saveDatabase(panel)){setProfileDeadzone(old);}else {SetWindowTextW(GetDlgItem(panel,A_CALIBRATE),(L"Zona morta: "+std::to_wstring(next)).c_str());status(L"Zona morta salva para este controle físico.");}break;}
 case A_PLAYER_PROFILE:{const auto selected=SendMessageW(profileCombo,CB_GETCURSEL,0,0);if(selected<0||selected>=static_cast<LRESULT>(profileKeys.size()))break;useControllerProfile(profileKeys[static_cast<size_t>(selected)]);for(int i=0;i<4;++i)SetWindowTextW(GetDlgItem(panel,A_CAPTURE_OPEN+i),(L"Botão "+std::to_wstring(*profileBinding(A_CAPTURE_OPEN+i))+L"  ·  Alterar").c_str());SetWindowTextW(GetDlgItem(panel,A_CALIBRATE),(L"Zona morta: "+std::to_wstring(profileDeadzone())).c_str());break;}
 case A_COMPAT_TOGGLE: {
  controllerCompatibility=!controllerCompatibility;
  if(!saveDatabase(panel))controllerCompatibility=!controllerCompatibility;
  bridgeReconfigure();InvalidateRect(GetDlgItem(panel,A_COMPAT_TOGGLE),nullptr,FALSE);refreshBridgePanel();break;
 }
 case A_COMPAT_RETRY:bridgeReconfigure();refreshBridgePanel();break;
 case A_COMPAT_DRIVER:ShellExecuteW(panel,L"open",L"https://github.com/nefarius/ViGEmBus/releases/tag/v1.22.0",nullptr,nullptr,SW_SHOWNORMAL);break;
 case A_COMPAT_HIDHIDE:ShellExecuteW(panel,L"open",L"https://docs.nefarius.at/projects/HidHide/Simple-Setup-Guide/",nullptr,nullptr,SW_SHOWNORMAL);break;
 case A_COMPAT_DEFAULT: {
  const auto selected=SendMessageW(defaultModeCombo,CB_GETCURSEL,0,0);if(selected<0||selected>2)break;
  const auto old=defaultControllerMode;defaultControllerMode=static_cast<ControllerMode>(selected+1);
  if(!saveDatabase(panel)){defaultControllerMode=old;SendMessageW(defaultModeCombo,CB_SETCURSEL,static_cast<int>(old)-1,0);}bridgeReconfigure();break;
 }
 case A_COMPAT_SOURCE: {
  const auto selected=SendMessageW(sourceCombo,CB_GETCURSEL,0,0);if(selected<0||selected>=static_cast<LRESULT>(sourceGuids.size()))break;
  const auto old=preferredControllerGuid;preferredControllerGuid=sourceGuids[static_cast<size_t>(selected)];
  if(!saveDatabase(panel))preferredControllerGuid=old;
  bridgeReconfigure();refreshBridgePanel();break;
 }
 case A_COMPAT_MAPPINGS: {
  wchar_t file[32768]{};OPENFILENAMEW of{};of.lStructSize=sizeof(of);of.hwndOwner=panel;of.lpstrFile=file;of.nMaxFile=32768;of.lpstrFilter=L"Mapeamentos SDL (texto)\0*.txt\0\0";of.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
  if(!GetOpenFileNameW(&of))break;
  WIN32_FILE_ATTRIBUTE_DATA info{};if(!GetFileAttributesExW(file,GetFileExInfoStandard,&info)||info.nFileSizeHigh||info.nFileSizeLow>4*1024*1024){MessageBoxW(panel,L"Escolha um arquivo de mapeamentos SDL em texto, de até 4 MB.",L"Confira o arquivo",MB_ICONWARNING);break;}
  const auto destination=dataFolder+L"\\gamecontrollerdb.txt";
  if(lstrcmpiW(file,destination.c_str())!=0&&!CopyFileW(file,destination.c_str(),FALSE)){MessageBoxW(panel,L"Não foi possível importar o arquivo.",L"FeatherPad",MB_ICONERROR);break;}
  bridgeReconfigure();refreshBridgePanel();break;
 }

 case A_SAVE_PROFILE: {
  const auto name=textOf(profileField);if(name.empty()||!validField(name,40)){MessageBoxW(panel,L"Informe um nome de 1 a 40 caracteres, sem quebras de linha.",L"Confira o nome",MB_ICONINFORMATION);break;}
  auto image=editingCover;if(image!=avatarPath&&!importArt(image,panel))break;
  const auto oldName=profileName,oldAvatar=avatarPath;profileName=name;avatarPath=image;
  if(saveDatabase(panel)){closePanel();status(L"Perfil atualizado.");}else{profileName=oldName;avatarPath=oldAvatar;}
  break; }
 case A_BROWSE_AVATAR:if(chooseImage(panel,editingCover))SetWindowTextW(profilePathField,editingCover.c_str());break;
 case A_CLEAR_AVATAR:editingCover.clear();SetWindowTextW(profilePathField,L"Sem imagem — usando a sua inicial");break;
 }
}
void trayMenu() {
 HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING,A_OPEN,L"Abrir FeatherPad");AppendMenuW(menu,MF_STRING,A_SETTINGS,L"Configurações");
 AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,A_EXIT,L"Encerrar monitor");
 POINT p{};GetCursorPos(&p);SetForegroundWindow(win);int id=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,win,nullptr);DestroyMenu(menu);if(id)action(id);PostMessageW(win,WM_NULL,0,0);
}
LRESULT CALLBACK panelProc(HWND h,UINT m,WPARAM w,LPARAM l) {
 switch(m) {
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:{PAINTSTRUCT ps{};HDC dc=BeginPaint(h,&ps);RECT r{};GetClientRect(h,&r);fill(dc,r,bg);EndPaint(h,&ps);return 0;}
 case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:SetTextColor(reinterpret_cast<HDC>(w),white);SetBkColor(reinterpret_cast<HDC>(w),bg);return reinterpret_cast<LRESULT>(bgBrush);
 case WM_CTLCOLORLISTBOX:SetTextColor(reinterpret_cast<HDC>(w),white);SetBkColor(reinterpret_cast<HDC>(w),surface);return reinterpret_cast<LRESULT>(inputBrush);
 case WM_CTLCOLOREDIT:SetTextColor(reinterpret_cast<HDC>(w),white);SetBkColor(reinterpret_cast<HDC>(w),surface);return reinterpret_cast<LRESULT>(inputBrush);
 case WM_DRAWITEM:drawButton(reinterpret_cast<const DRAWITEMSTRUCT*>(l));return TRUE;
 case WM_COMMAND:if(HIWORD(w)==BN_CLICKED||HIWORD(w)==CBN_SELCHANGE)action(LOWORD(w));return 0;
 case WM_CLOSE:closePanel();return 0;
 }
 return DefWindowProcW(h,m,w,l);
}
LRESULT CALLBACK captureProc(HWND h,UINT m,WPARAM w,LPARAM l) {
 switch(m) {
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:{PAINTSTRUCT ps{};HDC dc=BeginPaint(h,&ps);paintCapture(dc);EndPaint(h,&ps);return 0;}
 case WM_DRAWITEM:drawButton(reinterpret_cast<const DRAWITEMSTRUCT*>(l));return TRUE;
 case WM_COMMAND:if(HIWORD(w)==BN_CLICKED)action(LOWORD(w));return 0;
 case WM_CLOSE:closeCapture();return 0;
 }
 return DefWindowProcW(h,m,w,l);
}
LRESULT CALLBACK windowProc(HWND h,UINT m,WPARAM w,LPARAM l) {
 switch(m) {
 case WM_ERASEBKGND:return 1;
 case WM_PAINT: {
  PAINTSTRUCT ps{};HDC dc=BeginPaint(h,&ps);RECT r{};GetClientRect(h,&r);
  // A transient backbuffer prevents flicker. No fullscreen bitmap survives a paint.
  HDC memory=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,std::max(1L,r.right),std::max(1L,r.bottom));
  if(memory&&bitmap){auto old=SelectObject(memory,bitmap);paintCanvas(memory,r);BitBlt(dc,0,0,r.right,r.bottom,memory,0,0,SRCCOPY);SelectObject(memory,old);}
  else paintCanvas(dc,r);
  if(bitmap)DeleteObject(bitmap);
  if(memory)DeleteDC(memory);
  EndPaint(h,&ps);return 0; }
 case WM_SIZE:if(w==SIZE_MINIMIZED){clearArt();return 0;}InvalidateRect(h,nullptr,FALSE);return 0;
 case WM_GETMINMAXINFO:{auto info=reinterpret_cast<MINMAXINFO*>(l);if(!fullscreen){info->ptMinTrackSize.x=1000;info->ptMinTrackSize.y=760;}return 0;}
 case WM_LBUTTONUP:case WM_LBUTTONDBLCLK: {
  POINT p=canvasPoint(POINT{GET_X_LPARAM(l),GET_Y_LPARAM(l)});
  for(const auto& hit:hits)if(PtInRect(&hit.r,p)) {const auto copy=hit;if(copy.game>=0){selection=copy.game;keyboardFocus=-1;InvalidateRect(h,nullptr,FALSE);if(m==WM_LBUTTONDBLCLK)launch();}else action(copy.action);break;}
  return 0; }
 case WM_MOUSEMOVE: {
  POINT p=canvasPoint(POINT{GET_X_LPARAM(l),GET_Y_LPARAM(l)});int index=-1;
  for(size_t i=0;i<hits.size();++i)if(PtInRect(&hits[i].r,p)){index=static_cast<int>(i);break;}
  if(index!=hovered){hovered=index;InvalidateRect(h,nullptr,FALSE);}SetCursor(LoadCursorW(nullptr,index<0?IDC_ARROW:IDC_HAND));return 0; }
 case WM_MOUSEWHEEL:moveSelection(GET_WHEEL_DELTA_WPARAM(w)>0?-1:1);return 0;
 case WM_TIMER:if(w==LIBRARY_TIMER){libraryTick();return 0;}if(w==GAMEPAD_TIMER){bridgeTick();return 0;}break;
 case WM_INPUT:rawInput(l);return DefWindowProcW(h,m,w,l);
 case WM_INPUT_DEVICE_CHANGE:deviceChange(w,l);return 0;
 case WM_COMMAND:action(LOWORD(w));return 0;
 case UPDATE_READY:handleUpdateResult();return 0;
 case FINISH_CAPTURE:commitCapture();return 0;
 case WM_CLOSE:hide();return 0;
 case TRAY:if(l==WM_LBUTTONUP)show();else if(l==WM_RBUTTONUP)trayMenu();return 0;
 case WM_DESTROY:finishSessions();bridgeShutdown();clearArt();Shell_NotifyIconW(NIM_DELETE,&tray);PostQuitMessage(0);return 0;
 }
 return DefWindowProcW(h,m,w,l);
}
bool routeKey(MSG& msg) {
 if(keyboardWindow)return keyboardRoute(msg);
 if(msg.message==WM_KEYDOWN&&msg.wParam==VK_F11){toggleFullscreen();return true;}
 HWND target=captureWin?captureWin:panel;
 if(target&&(msg.hwnd==target||IsChild(target,msg.hwnd))) {
  if(msg.message==WM_KEYDOWN) {
   if(msg.wParam==VK_ESCAPE){if(captureWin)closeCapture();else closePanel();return true;}
   if(captureWin&&msg.wParam==VK_RETURN){if(GetFocus()==GetDlgItem(captureWin,A_CAPTURE_CANCEL))closeCapture();else commitCapture();return true;}
   if(msg.wParam==VK_RETURN){HWND focus=GetFocus();wchar_t cls[32]{};GetClassNameW(focus,cls,32);if(lstrcmpiW(cls,L"BUTTON")==0)SendMessageW(focus,BM_CLICK,0,0);else if(page==Page::Editor)action(A_SAVE_GAME);else if(page==Page::Profile)action(A_SAVE_PROFILE);return true;}
  }
  return IsDialogMessageW(target,&msg)!=FALSE;
 }
 if(msg.message!=WM_KEYDOWN||msg.hwnd!=win)return false;
 switch(msg.wParam) {
 case VK_ESCAPE:hide();return true;
 case VK_LEFT:controllerDirection(-1,0);return true;case VK_RIGHT:controllerDirection(1,0);return true;
 case VK_UP:controllerDirection(0,-1);return true;case VK_DOWN:controllerDirection(0,1);return true;
 case VK_TAB:if(!hits.empty()){keyboardFocus=(keyboardFocus+((GetKeyState(VK_SHIFT)&0x8000)?-1:1)+static_cast<int>(hits.size()))%static_cast<int>(hits.size());InvalidateRect(win,nullptr,FALSE);}return true;
 case VK_RETURN:case VK_SPACE:activateMain();return true;
 case 'F':toggleFavorite();return true;case 'S':openPage(Page::Settings);return true;
 case 'A':openPage(Page::Editor);return true;case 'E':if(selection>=0)openPage(Page::Editor,selection);return true;
 case 'P':openPage(Page::Profile);return true;
 case VK_F3:case 'Q':openVirtualKeyboard();return true;
 }
 return false;
}
void activateMain() {
 if(keyboardFocus>=0&&keyboardFocus<static_cast<int>(hits.size())) {
  const auto hit=hits[keyboardFocus];
  if(hit.game>=0){selection=hit.game;launch();}else action(hit.action);
 } else launch();
}
void controllerDirection(int dx,int dy) {
 if(keyboardWindow){keyboardDirection(dx,dy);return;}
 if(captureWin)return;
 if(panel){HWND next=GetNextDlgTabItem(panel,GetFocus(),dx<0||dy<0);if(next)SetFocus(next);return;}
 int current=keyboardFocus;
 if(current<0||current>=static_cast<int>(hits.size())) {
  // Horizontal movement stays on the carousel and scrolls beyond visible cards.
  if(dx){moveSelection(dx);return;}
  auto it=std::find_if(hits.begin(),hits.end(),[](const Hit& hit){return hit.game==selection&&hit.game>=0;});
  if(it==hits.end()){keyboardFocus=0;InvalidateRect(win,nullptr,FALSE);return;}
  current=static_cast<int>(it-hits.begin());
 }
 const RECT from=hits[current].r;
 const int cx=(from.left+from.right)/2,cy=(from.top+from.bottom)/2;
 long best=0x7fffffff;int target=-1;
 for(size_t i=0;i<hits.size();++i) {
  if(static_cast<int>(i)==current)continue;
  const auto& r=hits[i].r;const int x=(r.left+r.right)/2-cx,y=(r.top+r.bottom)/2-cy;
  const int primary=dx?x*dx:y*dy,secondary=dx?abs(y):abs(x);
  if(primary<=3)continue;
  const long score=primary+3L*secondary;
  if(score<best){best=score;target=static_cast<int>(i);}
 }
 if(target>=0){keyboardFocus=target;if(hits[target].game>=0){selection=hits[target].game;keyboardFocus=-1;}InvalidateRect(win,nullptr,FALSE);}
}
void controllerButtons(HANDLE h,const std::set<USAGE>& now,const std::set<USAGE>& previous) {
 if(captureWin&&!captureProfileKey.empty())useControllerProfile(captureProfileKey);
 if(keyboardWindow){if(!selfTesting&&GetForegroundWindow()!=keyboardWindow)return;for(auto b:now)if(!previous.count(b)){if(b==*profileBinding(A_CAPTURE_BACK)){closeVirtualKeyboard(false);return;}if(b==*profileBinding(A_CAPTURE_OK)){keyboardConfirm();return;}}return;}
 if(captureWin) {
  if(!selfTesting&&GetForegroundWindow()!=captureWin)return;
  if(now.empty())captureArmed.insert(h);
  if(captureCandidate){
   for(auto b:now)if(!previous.count(b)){
    if(b==*profileBinding(A_CAPTURE_OK)){const int id=GetFocus()==GetDlgItem(captureWin,A_CAPTURE_CANCEL)?A_CAPTURE_CANCEL:A_CAPTURE_SAVE;PostMessageW(captureWin,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),0);return;}
    if(b==*profileBinding(A_CAPTURE_BACK)){closeCapture();return;}
   }
   return;
  }
  if(!captureArmed.count(h))return;
  for(auto b:now)if(!previous.count(b)) {
   const unsigned used[]={*profileBinding(A_CAPTURE_OPEN),*profileBinding(A_CAPTURE_OK),*profileBinding(A_CAPTURE_BACK),*profileBinding(A_CAPTURE_STAR)};
   const int slot=captureAction-A_CAPTURE_OPEN;bool collision=false;
   for(int i=0;i<4;++i)if(i!=slot&&used[i]==b)collision=true;
   if(collision){captureNotice=L"O botão "+std::to_wstring(b)+L" já está em uso por outra ação. Solte-o e escolha um botão diferente.";captureArmed.erase(h);InvalidateRect(captureWin,nullptr,FALSE);return;}
   captureCandidate=b;captureNotice=L"Botão "+std::to_wstring(b)+L" identificado.\n\nSalve para aplicar à ação escolhida, ou cancele para manter o botão anterior.";
   EnableWindow(GetDlgItem(captureWin,A_CAPTURE_SAVE),TRUE);SetFocus(GetDlgItem(captureWin,A_CAPTURE_SAVE));InvalidateRect(captureWin,nullptr,FALSE);return;
  }
  return;
 }
 if(panel) {
  if(!selfTesting&&GetForegroundWindow()!=panel)return;
  if(page==Page::Diagnostic){std::wstring s=now.empty()?L"Nenhum botão pressionado":L"Botões detectados:";for(auto b:now)s+=L"  "+std::to_wstring(b);HWND output=GetDlgItem(panel,420);if(textOf(output)!=s)SetWindowTextW(output,s.c_str());return;}
  for(auto b:now)if(!previous.count(b)) {
   if(b==*profileBinding(A_CAPTURE_BACK)){closePanel();return;}
   if(b==*profileBinding(A_CAPTURE_OK)){HWND focused=GetFocus();wchar_t cls[32]{};GetClassNameW(focused,cls,32);if(lstrcmpiW(cls,L"BUTTON")==0)SendMessageW(focused,BM_CLICK,0,0);else if(lstrcmpiW(cls,L"EDIT")==0&&!(GetWindowLongPtrW(focused,GWL_STYLE)&ES_READONLY))openVirtualKeyboard(focused);return;}
  }
  return;
 }
 for(auto b:now)if(!previous.count(b)) {
  if(b==*profileBinding(A_CAPTURE_OPEN)){if(IsWindowVisible(win)&&!IsIconic(win))hide();else show();return;}
  if(IsWindowVisible(win)&&GetForegroundWindow()==win) {
   if(b==*profileBinding(A_CAPTURE_OK)){activateMain();return;}if(b==*profileBinding(A_CAPTURE_BACK)){hide();return;}if(b==*profileBinding(A_CAPTURE_STAR)){toggleFavorite();return;}
  }
 }
}

void refreshBridgePanel() {
 if(!panel||page!=Page::Compatibility)return;
 HWND label=GetDlgItem(panel,430);const auto value=bridgeStatus();if(textOf(label)!=value)SetWindowTextW(label,value.c_str());
 const auto current=bridgeSources();std::wstring signature=preferredControllerGuid;
 for(const auto& entry:current)signature+=L"|"+entry.guid+L":"+entry.name;
 if(signature==sourceSignature&&SendMessageW(sourceCombo,CB_GETCOUNT,0,0)>0)return;
 sourceSignature=signature;sourceGuids.clear();sourceGuids.push_back(L"");
 SendMessageW(sourceCombo,CB_RESETCONTENT,0,0);SendMessageW(sourceCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Automático — primeiro controle reconhecido"));int selected=0;
 for(const auto& entry:current){SendMessageW(sourceCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(entry.name.c_str()));sourceGuids.push_back(entry.guid);if(entry.guid==preferredControllerGuid)selected=static_cast<int>(sourceGuids.size())-1;}
 if(!preferredControllerGuid.empty()&&!selected){SendMessageW(sourceCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Controle preferido (desconectado)"));sourceGuids.push_back(preferredControllerGuid);selected=static_cast<int>(sourceGuids.size())-1;}
 SendMessageW(sourceCombo,CB_SETCURSEL,selected,0);
}
