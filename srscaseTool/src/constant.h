#ifndef CONSTANT_H_
#define CONSTANT_H_
#include <windows.h>
constexpr int ww = 1800;
constexpr int wh = 1000;

constexpr int searchInputStyle = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NOHIDESEL | ES_LEFT;

constexpr int xs = 20;           // 水平间距
constexpr int ys = 12;           // 垂直间距
constexpr int eh = 28;           // 输入框高度
constexpr int lh = 24;           // label高度
constexpr int yt = 20;           // 顶部Y
// 第一行控件位置
constexpr int row1Y = 20;
constexpr int splitterW = 6;
constexpr int pageLabelW = 220;
int row2Y = row1Y + eh + ys;
// 顶部搜索区域高度
int topSectionHeight = row2Y + eh + ys + 10;


const int SPLITTER_WIDTH = 4;
const int MIN_PANEL_WIDTH = 80;
struct SplitterState
{
    int splitterX = 0;        // Initial splitter position
    bool dragging = false;
    int dragOffset = 0;         // Mouse offset from edge
};

constexpr int pageSize = 40;      // 每页记录数
enum {
    IDC_STATIC_SUITE = 2000,
    IDC_STATIC_NAME,
    IDC_STATIC_ID,
    IDC_STATIC_SCRIPTID,
    IDC_STATIC_MODULE,
    IDC_STATIC_TEXT,
    IDC_BTN_PREV,
    IDC_BTN_NEXT,
    IDC_BTN_SEARCH,
    IDC_EDIT_SUITE,
    IDC_EDIT_NAME,
    IDC_EDIT_ID,
    IDC_EDIT_SCRIPTID,
    IDC_COMBO_MODULE,
    IDC_EDIT_TEXT,
    IDC_LISTVIEW,
    IDC_EDIT_CONTENT,
};
#endif // CONSTANT_H_
