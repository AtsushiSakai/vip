// ae2019.cpp   Anthony's Editor 2019.
// compile: g++ -std=c++1z ae2019.cpp -lcurses
#include <curses.h>
#include <fstream>
#include <unordered_map>
#include <vector>

using namespace std;
enum {
  kESC = 27,
  kBS = 8,
  kDEL = 127,
  kCtrlD = 4,
  kCtrlU = 21,
  kCtrlR = 18,
  kCtrlS = 19,
  kCtrlQ = 17,
};
const char *gFileName;
vector<char> gBuf;
bool gDone = false;
int gIndex = 0 /* offset of cursor pos */, gPageStart = 0, gPageEnd = 0;
int gCol, gRow;

int lineTop(const int inOffset) {
  int offset = inOffset - 1;
  while (offset >= 0 && gBuf[offset] != '\n') {
    offset--;
  }
  return offset <= 0 ? 0 : ++offset;
}
int nextLineTop(const int inOffset) {
  int offset = inOffset;
  while (offset < gBuf.size() && gBuf[offset++] != '\n') { /* empty */
  }
  return offset < gBuf.size() ? offset : gBuf.size() - 1;
}

int adjust(const int inOffset, const int inCol) {
  int offset = inOffset;
  for (int i = 0; offset < gBuf.size() && gBuf[offset] != '\n' && i < inCol;
       offset++) {
    i += gBuf[offset] == '\t' ? 8 - (i & 7) : 1;
  }
  return offset;
}

void display() {
  if (gIndex < gPageStart) {
    gPageStart = lineTop(gIndex);
  }
  if (gPageEnd <= gIndex) {
    gPageStart = nextLineTop(gIndex);
    int n = gPageStart == gBuf.size() - 1 ? LINES - 2 : LINES;
    for (int i = 0; i < n; i++) {
      gPageStart = lineTop(gPageStart - 1);
    }
  }
  move(0, 0);
  int i = 0, j = 0;
  gPageEnd = gPageStart;
  for (auto p = gBuf.begin() + gPageEnd; /*empty */; gPageEnd++, p++) {
    if (gIndex == gPageEnd) {
      gRow = i;
      gCol = j;
    }  // update cursor pos.
    if (LINES <= i || gBuf.size() <= gPageEnd) {
      break;
    }
    if (*p != '\r') {
      addch(*p);
      j += *p == '\t' ? 8 - (j & 7) : 1;
    }
    if (*p == '\n' || COLS <= j) {
      ++i;
      j = 0;
    }
  }
  clrtobot();
  if (++i < LINES) {
    mvaddstr(i, 0, "<< EOF >>");
  }
  move(gRow, gCol);
  refresh();
}

void left() {
  if (0 < gIndex) {
    --gIndex;
  }
}
void right() {
  if (gIndex < gBuf.size()) {
    ++gIndex;
  }
}
void up() { gIndex = adjust(lineTop(lineTop(gIndex) - 1), gCol); }
void down() { gIndex = adjust(nextLineTop(gIndex), gCol); }
void lineBegin() { gIndex = lineTop(gIndex); }
void lineEnd() {
  gIndex = nextLineTop(gIndex);
  left();
}
void top() { gIndex = 0; }
void bottom() { gIndex = gBuf.size() - 1; }
void del() {
  if (gIndex < gBuf.size() - 1) {
    gBuf.erase(gBuf.begin() + gIndex);
  }
}
void quit() { gDone = true; }
void redraw() {
  clear();
  display();
}

static void insert() {
  for (int ch; (ch = getch()) != kESC; display()) {
    if (gIndex > 0 && (ch == kBS || ch == kDEL)) {
      --gIndex;
      gBuf.erase(gBuf.begin() + gIndex);
    } else {
      gBuf.insert(gBuf.begin() + gIndex, ch == '\r' ? '\n' : ch);
      gIndex++;
    }
  }
}

void wordLeft() {
  while (!isspace(gBuf[gIndex]) && 0 < gIndex) {
    --gIndex;
  }
  while (isspace(gBuf[gIndex]) && 0 < gIndex) {
    --gIndex;
  }
}
void wordRight() {
  while (!isspace(gBuf[gIndex]) && gIndex < gBuf.size()) {
    ++gIndex;
  }
  while (isspace(gBuf[gIndex]) && gIndex < gBuf.size()) {
    ++gIndex;
  }
}
void pageDown() {
  gPageStart = gIndex = lineTop(gPageEnd - 1);
  while (0 < gRow--) {
    down();
  }
  gPageEnd = gBuf.size() - 1;
}
void pageUp() {
  for (int i = LINES; 0 < --i; up()) {
    gPageStart = lineTop(gPageStart - 1);
  }
}

void save() {
  ofstream ofs(gFileName, ios::binary);
  ostream_iterator<char> output_iterator(ofs);
  copy(gBuf.begin(), gBuf.end(), output_iterator);
}

unordered_map<char, void (*)()> gAction = {
    {'h', left},      {'l', right},     {'k', up},          {'j', down},
    {'b', wordLeft},  {'w', wordRight}, {kCtrlD, pageDown}, {kCtrlU, pageUp},
    {'0', lineBegin}, {'$', lineEnd},   {'t', top},         {'G', bottom},
    {'i', insert},    {'x', del},       {kCtrlQ, quit},     {kCtrlR, redraw},
    {kCtrlS, save},
};

int main(int argc, char **argv) {
  if (argc < 2) {
    return 2;
  }
  initscr();
  raw();
  noecho();
  idlok(stdscr, true);  // init screen.
  gFileName = argv[1];
  ifstream ifs(gFileName, ios::binary);
  gBuf.assign(istreambuf_iterator<char>(ifs), istreambuf_iterator<char>());
  while (!gDone) {
    display();
    char ch = getch();
    if (gAction.count(ch) > 0) {
      gAction[ch]();
    }
  }
  endwin();
  return 0;
}
