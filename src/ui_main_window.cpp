#include "ui_main_window.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Tree_Item.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/fl_ask.H>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwctype>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <unordered_set>

#include "scanner.h"
#include "theme.h"

namespace fs = std::filesystem;

// Fl_Tree that forwards right-clicks to a context-menu handler
class FileTree : public Fl_Tree {
public:
    FileTree(int X, int Y, int W, int H) : Fl_Tree(X, Y, W, H) {}
    std::function<void(Fl_Tree_Item*)> on_rclick;
    int handle(int event) override {
        if (event == FL_PUSH && Fl::event_button() == FL_RIGHT_MOUSE) {
            Fl_Tree_Item* it = find_clicked();
            if (it && it != root() && on_rclick) {
                select_only(it, 0);
                on_rclick(it);
            }
            return 1;
        }
        return Fl_Tree::handle(event);
    }
};

namespace {
constexpr int W = 680;
constexpr int M = 12;                  // window margin
constexpr int ADV_H = 164;
constexpr int DH = ADV_H + 8;          // height delta when advanced panel toggles
constexpr int BOTTOM_Y0 = 398;         // bottom group y when collapsed
constexpr int AY = BOTTOM_Y0;          // advanced panel y offsets are relative to this
constexpr int H0 = BOTTOM_Y0 + 70 + M;

std::wstring lc_key(const std::string& utf8) {
    std::wstring w = utf8_to_wide(utf8);
    std::transform(w.begin(), w.end(), w.begin(),
                   [](wchar_t c) { return (wchar_t)std::towlower(c); });
    return w;
}

Fl_Color status_color(JobStatus s) {
    switch (s) {
        case JobStatus::Running: return kAccent;
        case JobStatus::Done: return fl_rgb_color(24, 128, 56);
        case JobStatus::Failed: return fl_rgb_color(217, 48, 37);
        case JobStatus::Cancelled: return fl_rgb_color(107, 114, 128);
        default: return FL_FOREGROUND_COLOR;
    }
}

// leaf items carry index+1 in user_data; folder nodes carry null
int leaf_index(const Fl_Tree_Item* it) { return (int)(intptr_t)it->user_data() - 1; }

void collect_leaves(Fl_Tree_Item* it, std::vector<int>& out) {
    int idx = leaf_index(it);
    if (idx >= 0) { out.push_back(idx); return; }
    for (int i = 0; i < it->children(); ++i) collect_leaves(it->child(i), out);
}

void open_in_explorer(const std::string& path_utf8) {
    std::wstring p = utf8_to_path(path_utf8).make_preferred().native();
    ShellExecuteW(nullptr, L"open", L"explorer.exe",
                  (L"/select,\"" + p + L"\"").c_str(), nullptr, SW_SHOWNORMAL);
}
}  // namespace

MainWindow::MainWindow() : Fl_Double_Window(W, H0, "WebP 批量轉換") {
    toolbar_ = new Fl_Group(0, 0, W, 48);
    auto* add_files = new HoverButton(M, 9, 92, 30, "加入檔案");
    auto* add_dir = new HoverButton(110, 9, 104, 30, "加入資料夾");
    auto* remove = new HoverButton(220, 9, 92, 30, "移除勾選");
    auto* clear = new HoverButton(318, 9, 64, 30, "清空");
    auto* sel_all = new HoverButton(388, 9, 56, 30, "全選");
    auto* sel_none = new HoverButton(450, 9, 68, 30, "全不選");
    recursive_ = new Fl_Check_Button(526, 9, 130, 30, "含子資料夾");
    recursive_->value(1);
    toolbar_->end();

    tree_ = new FileTree(M, 52, W - 2 * M, 264);
    tree_->showroot(0);
    tree_->item_reselect_mode(FL_TREE_SELECTABLE_ALWAYS);
    tree_->color(FL_BACKGROUND2_COLOR);
    tree_->selection_color(fl_rgb_color(224, 234, 254));
    tree_->connectorstyle(FL_TREE_CONNECTOR_NONE);
    tree_->linespacing(6);
    tree_->marginleft(8);
    tree_->margintop(6);
    tree_->tooltip("點 ☐ 勾選/取消;右鍵有更多操作;雙擊失敗項目看錯誤訊息;可直接拖入檔案或資料夾");

    quality_ = new Fl_Value_Slider(M + 74, 328, W - 2 * M - 74, 26, "品質");
    quality_->type(FL_HOR_SLIDER);
    quality_->align(FL_ALIGN_LEFT);
    quality_->bounds(0, 100);
    quality_->step(1);
    quality_->value(90);
    quality_->textsize(12);
    quality_->color(fl_rgb_color(233, 236, 241));
    quality_->selection_color(kAccent);   // knob

    adv_toggle_ = new Fl_Toggle_Button(M, 362, 110, 26, "詳細參數 ▸");
    adv_toggle_->box(FL_NO_BOX);
    adv_toggle_->labelcolor(kAccent);
    adv_toggle_->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

    adv_group_ = new Fl_Group(M, AY, W - 2 * M, ADV_H);
    adv_group_->box(FL_UP_BOX);
    adv_group_->color(fl_rgb_color(238, 241, 246));
    lossless_ = new Fl_Check_Button(26, AY + 8, 100, 24, "-lossless");
    nl_chk_ = new Fl_Check_Button(136, AY + 8, 132, 24, "-near_lossless");
    nl_val_ = new Fl_Spinner(270, AY + 8, 64, 24);
    nl_val_->range(0, 100);
    nl_val_->value(60);
    z_choice_ = new Fl_Choice(402, AY + 8, 80, 24, "-z");
    z_choice_->add("停用|0|1|2|3|4|5|6|7|8|9");
    z_choice_->value(0);
    m_choice_ = new Fl_Choice(60, AY + 40, 64, 24, "-m");
    m_choice_->add("0|1|2|3|4|5|6");
    m_choice_->value(4);
    preset_choice_ = new Fl_Choice(202, AY + 40, 110, 24, "-preset");
    preset_choice_->add("無|photo|picture|drawing|icon|text");
    preset_choice_->value(0);
    meta_choice_ = new Fl_Choice(422, AY + 40, 100, 24, "-metadata");
    meta_choice_->add("none|all|exif|icc|xmp");
    meta_choice_->value(0);
    resize_chk_ = new Fl_Check_Button(26, AY + 72, 84, 24, "-resize");
    rw_ = new Fl_Int_Input(142, AY + 72, 72, 24, "寬");
    rw_->value("0");
    rh_ = new Fl_Int_Input(252, AY + 72, 72, 24, "高");
    rh_->value("0");
    auto* hint = new Fl_Box(332, AY + 72, 160, 24, "(0 = 等比例)");
    hint->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    hint->labelsize(12);
    hint->labelcolor(fl_rgb_color(107, 114, 128));
    mt_ = new Fl_Check_Button(26, AY + 104, 64, 24, "-mt");
    mt_->value(1);
    sharp_ = new Fl_Check_Button(100, AY + 104, 110, 24, "-sharp_yuv");
    aq_chk_ = new Fl_Check_Button(220, AY + 104, 96, 24, "-alpha_q");
    aq_val_ = new Fl_Spinner(318, AY + 104, 64, 24);
    aq_val_->range(0, 100);
    aq_val_->value(100);
    extra_ = new Fl_Input(112, AY + 136, W - 112 - M - 20, 24, "額外參數");
    extra_->tooltip("以空白切分後直接附加給 cwebp;請勿放含空白的路徑");
    adv_group_->end();
    adv_group_->hide();

    bottom_group_ = new Fl_Group(0, BOTTOM_Y0, W, 70);
    out_same_ = new Fl_Round_Button(M, BOTTOM_Y0 + 2, 134, 24, "來源同資料夾");
    out_same_->type(FL_RADIO_BUTTON);
    out_same_->value(1);
    out_custom_ = new Fl_Round_Button(152, BOTTOM_Y0 + 2, 106, 24, "自訂資料夾");
    out_custom_->type(FL_RADIO_BUTTON);
    out_dir_ = new Fl_Output(262, BOTTOM_Y0 + 2, 318, 24);
    out_browse_ = new HoverButton(586, BOTTOM_Y0 + 2, 82, 24, "瀏覽…");
    start_btn_ = new HoverButton(M, BOTTOM_Y0 + 34, 112, 32, "開始轉換");
    start_btn_->color(kAccent);
    start_btn_->labelcolor(FL_WHITE);
    start_btn_->labelfont(FL_HELVETICA_BOLD);
    cancel_btn_ = new HoverButton(132, BOTTOM_Y0 + 34, 76, 32, "取消");
    cancel_btn_->deactivate();
    progress_ = new Fl_Progress(216, BOTTOM_Y0 + 34, 200, 32);
    progress_->minimum(0);
    progress_->maximum(1);
    progress_->color(fl_rgb_color(229, 233, 240));
    progress_->selection_color(kAccent);
    status_box_ = new Fl_Box(426, BOTTOM_Y0 + 34, W - 426 - M, 32);
    status_box_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    bottom_group_->end();

    end();
    style_modern(this);
    resizable(tree_);
    size_range(W, 440, W, 0);   // fixed width, vertical resize only

    add_files->callback([](Fl_Widget*, void* d) {
        auto* w = (MainWindow*)d;
        Fl_Native_File_Chooser fc(Fl_Native_File_Chooser::BROWSE_MULTI_FILE);
        fc.title("加入檔案");
        fc.filter("影像檔\t*.{png,jpg,jpeg,tif,tiff,webp}");
        if (fc.show() != 0) return;
        std::vector<std::string> paths;
        for (int i = 0; i < fc.count(); ++i) paths.push_back(fc.filename(i));
        w->add_paths(paths);
    }, this);
    add_dir->callback([](Fl_Widget*, void* d) {
        auto* w = (MainWindow*)d;
        Fl_Native_File_Chooser fc(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
        fc.title("加入資料夾");
        if (fc.show() != 0) return;
        w->add_paths({fc.filename()});
    }, this);
    remove->callback([](Fl_Widget*, void* d) {
        auto* w = (MainWindow*)d;
        if (w->converter_.running()) return;
        std::vector<int> idxs;
        for (int i = 0; i < (int)w->items_.size(); ++i)
            if (w->items_[i].checked) idxs.push_back(i);
        w->remove_items(idxs);
    }, this);
    clear->callback([](Fl_Widget*, void* d) {
        auto* w = (MainWindow*)d;
        if (w->converter_.running() || w->items_.empty()) return;
        if (fl_choice("確定要清空全部 %d 個項目?", "取消", "清空", nullptr,
                      (int)w->items_.size()) != 1)
            return;
        w->items_.clear();
        w->keys_.clear();
        w->rebuild_tree();
    }, this);
    sel_all->callback([](Fl_Widget*, void* d) { ((MainWindow*)d)->set_all_checked(true); }, this);
    sel_none->callback([](Fl_Widget*, void* d) { ((MainWindow*)d)->set_all_checked(false); }, this);
    tree_->callback([](Fl_Widget*, void* d) { ((MainWindow*)d)->on_tree_event(); }, this);
    tree_->on_rclick = [this](Fl_Tree_Item* it) { show_context_menu(it); };
    adv_toggle_->callback([](Fl_Widget*, void* d) { ((MainWindow*)d)->toggle_advanced(); }, this);
    z_choice_->callback([](Fl_Widget*, void* d) { ((MainWindow*)d)->apply_z_state(); }, this);
    out_browse_->callback([](Fl_Widget*, void* d) { ((MainWindow*)d)->browse_output_dir(); }, this);
    start_btn_->callback([](Fl_Widget*, void* d) { ((MainWindow*)d)->start_conversion(); }, this);
    cancel_btn_->callback([](Fl_Widget*, void* d) {
        auto* w = (MainWindow*)d;
        w->cancel_btn_->deactivate();
        w->converter_.cancel();
    }, this);
    callback([](Fl_Widget* w, void*) { ((MainWindow*)w)->on_close(); });

    cwebp_path_ = find_cwebp();
    if (cwebp_path_.empty()) {
        status_box_->labelcolor(FL_RED);
        status_box_->label("找不到 cwebp,請安裝並加入 PATH");
        start_btn_->deactivate();
    } else {
        status_box_->copy_tooltip(cwebp_path_.c_str());
        update_idle_status();
    }
}

int MainWindow::handle(int event) {
    switch (event) {
        case FL_DND_ENTER:
        case FL_DND_DRAG:
            // Bypassing Fl_Group routing skips belowmouse bookkeeping, but
            // FLTK delivers FL_DND_RELEASE/FL_PASTE straight to belowmouse
            Fl::belowmouse(this);
            return 1;
        case FL_DND_RELEASE:
            return 1;
        case FL_PASTE:
            // event_length() can be 0 for DnD paste; rely on NUL termination
            if (Fl::event_text()) add_dropped(Fl::event_text());
            return 1;
    }
    return Fl_Double_Window::handle(event);
}

std::string MainWindow::label_for(const FileItem& it) const {
    const char* st = "待轉";
    switch (it.status) {
        case JobStatus::Pending: break;
        case JobStatus::Running: st = "轉換中"; break;
        case JobStatus::Done: st = "✔ 完成"; break;
        case JobStatus::Failed: st = "✘ 失敗"; break;
        case JobStatus::Cancelled: st = "已取消"; break;
    }
    std::string name = it.root_utf8.empty()
        ? it.path_utf8
        : path_to_utf8(utf8_to_path(it.path_utf8).filename());
    return std::string(it.checked ? "☑ " : "☐ ") + "[" + st + "] " + name;
}

void MainWindow::add_dropped(const std::string& text) {
    std::vector<std::string> paths;
    std::stringstream ss(text);
    for (std::string line; std::getline(ss, line);) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.rfind("file:///", 0) == 0) line = line.substr(8);
        else if (line.rfind("file://", 0) == 0) line = line.substr(7);
        if (!line.empty()) paths.push_back(line);
    }
    if (!paths.empty()) add_paths(paths);
}

void MainWindow::add_paths(const std::vector<std::string>& paths_utf8) {
    if (converter_.running()) return;
    bool rec = recursive_->value() != 0;
    std::error_code ec;
    bool added = false;
    for (const auto& s : paths_utf8) {
        fs::path rp = utf8_to_path(s).lexically_normal();
        if (!rp.has_filename()) rp = rp.parent_path();   // strip trailing separator
        std::string root = fs::is_directory(rp, ec) ? path_to_utf8(rp) : std::string();
        for (const auto& f : collect_images({s}, rec)) {
            if (!keys_.insert(lc_key(f)).second) continue;
            FileItem it;
            it.path_utf8 = f;
            it.root_utf8 = root;
            items_.push_back(std::move(it));
            added = true;
        }
    }
    if (added) rebuild_tree();
    else update_idle_status();
}

void MainWindow::rebuild_tree() {
    // clear() would delete the root item itself, leaving root() NULL
    tree_->clear_children(tree_->root());
    leaf_.assign(items_.size(), nullptr);
    std::map<std::wstring, Fl_Tree_Item*> dirs;
    auto ensure_dir = [&](Fl_Tree_Item* parent, const fs::path& full,
                          const std::string& label) {
        std::wstring key = lc_key(path_to_utf8(full));
        auto found = dirs.find(key);
        if (found != dirs.end()) return found->second;
        Fl_Tree_Item* n = tree_->add(parent, label.c_str());
        dirs.emplace(key, n);
        return n;
    };
    for (size_t i = 0; i < items_.size(); ++i) {
        const FileItem& it = items_[i];
        Fl_Tree_Item* parent = tree_->root();
        if (!it.root_utf8.empty()) {
            fs::path root = utf8_to_path(it.root_utf8);
            parent = ensure_dir(parent, root, it.root_utf8);
            fs::path rel = utf8_to_path(it.path_utf8).parent_path().lexically_relative(root);
            fs::path acc = root;
            for (const auto& comp : rel) {
                if (comp.native() == L".") continue;
                if (comp.native() == L"..") break;   // unexpected; keep under root
                acc /= comp;
                parent = ensure_dir(parent, acc, path_to_utf8(comp));
            }
        }
        Fl_Tree_Item* li = tree_->add(parent, label_for(it).c_str());
        li->user_data((void*)(intptr_t)(i + 1));
        li->labelcolor(status_color(it.status));
        leaf_[i] = li;
    }
    tree_->redraw();
    update_idle_status();
}

void MainWindow::refresh_leaf(int idx) {
    if (idx < 0 || idx >= (int)leaf_.size() || !leaf_[idx]) return;
    leaf_[idx]->label(label_for(items_[idx]).c_str());
    leaf_[idx]->labelcolor(status_color(items_[idx].status));
}

void MainWindow::set_all_checked(bool checked) {
    if (converter_.running()) return;
    for (size_t i = 0; i < items_.size(); ++i) {
        items_[i].checked = checked;
        refresh_leaf((int)i);
    }
    tree_->redraw();
    update_idle_status();
}

void MainWindow::remove_items(const std::vector<int>& indices) {
    if (indices.empty()) return;
    std::unordered_set<int> del(indices.begin(), indices.end());
    std::vector<FileItem> keep;
    for (int i = 0; i < (int)items_.size(); ++i) {
        if (del.count(i)) keys_.erase(lc_key(items_[i].path_utf8));
        else keep.push_back(std::move(items_[i]));
    }
    items_ = std::move(keep);
    rebuild_tree();
}

void MainWindow::show_context_menu(Fl_Tree_Item* item) {
    if (converter_.running()) return;
    int idx = leaf_index(item);
    if (idx >= 0 && idx < (int)items_.size()) {
        Fl_Menu_Item menu[] = {{"勾選/取消勾選"}, {"開啟所在資料夾"}, {"移除"}, {nullptr}};
        const Fl_Menu_Item* pick = menu->popup(Fl::event_x(), Fl::event_y());
        if (pick == menu + 0) {
            items_[idx].checked = !items_[idx].checked;
            refresh_leaf(idx);
            tree_->redraw();
            update_idle_status();
        } else if (pick == menu + 1) {
            open_in_explorer(items_[idx].path_utf8);
        } else if (pick == menu + 2) {
            remove_items({idx});
        }
    } else if (idx < 0) {
        Fl_Menu_Item menu[] = {{"全部勾選"}, {"全部取消勾選"}, {"移除整個資料夾"}, {nullptr}};
        const Fl_Menu_Item* pick = menu->popup(Fl::event_x(), Fl::event_y());
        if (!pick) return;
        std::vector<int> idxs;
        collect_leaves(item, idxs);
        if (pick == menu + 2) {
            remove_items(idxs);
        } else {
            bool v = pick == menu + 0;
            for (int i : idxs) {
                items_[i].checked = v;
                refresh_leaf(i);
            }
            tree_->redraw();
            update_idle_status();
        }
    }
}

void MainWindow::update_idle_status() {
    if (cwebp_path_.empty()) return;
    int checked = 0;
    for (const auto& it : items_) checked += it.checked;
    char buf[64];
    snprintf(buf, sizeof(buf), "共 %d 項,勾選 %d", (int)items_.size(), checked);
    status_box_->labelcolor(FL_FOREGROUND_COLOR);
    status_box_->copy_label(buf);
}

void MainWindow::on_tree_event() {
    if (tree_->callback_reason() != FL_TREE_REASON_SELECTED &&
        tree_->callback_reason() != FL_TREE_REASON_RESELECTED)
        return;
    Fl_Tree_Item* item = tree_->callback_item();
    if (!item) return;
    int idx = leaf_index(item);
    if (idx < 0 || idx >= (int)items_.size()) return;
    FileItem& it = items_[idx];
    if (Fl::event_clicks() > 0) {
        if (it.status == JobStatus::Failed && !it.error_text.empty())
            fl_alert("%s", it.error_text.c_str());
        return;
    }
    if (converter_.running()) return;
    if (Fl::event_x() <= item->x() + 22) {   // click on the checkbox glyph
        it.checked = !it.checked;
        refresh_leaf(idx);
        tree_->redraw();
        update_idle_status();
    }
}

void MainWindow::toggle_advanced() {
    resizable(nullptr);
    if (adv_toggle_->value()) {
        size(w(), h() + DH);
        adv_group_->position(adv_group_->x(), bottom_group_->y());
        adv_group_->show();
        bottom_group_->position(bottom_group_->x(), bottom_group_->y() + DH);
        adv_toggle_->label("詳細參數 ▾");
    } else {
        adv_group_->hide();
        bottom_group_->position(bottom_group_->x(), bottom_group_->y() - DH);
        size(w(), h() - DH);
        adv_toggle_->label("詳細參數 ▸");
    }
    init_sizes();
    resizable(tree_);
    redraw();
}

void MainWindow::apply_z_state() {
    if (z_choice_->value() > 0) {
        quality_->deactivate();
        m_choice_->deactivate();
        lossless_->deactivate();
    } else {
        quality_->activate();
        m_choice_->activate();
        lossless_->activate();
    }
}

CwebpOptions MainWindow::read_options() const {
    CwebpOptions o;
    o.quality = (int)quality_->value();
    o.lossless = lossless_->value() != 0;
    o.near_lossless = nl_chk_->value() ? (int)nl_val_->value() : -1;
    o.z = z_choice_->value() > 0 ? z_choice_->value() - 1 : -1;
    o.method = m_choice_->value();
    static const char* presets[] = {"", "photo", "picture", "drawing", "icon", "text"};
    o.preset = presets[preset_choice_->value()];
    o.resize = resize_chk_->value() != 0;
    o.rw = std::atoi(rw_->value());
    o.rh = std::atoi(rh_->value());
    o.mt = mt_->value() != 0;
    o.sharp_yuv = sharp_->value() != 0;
    o.alpha_q = aq_chk_->value() ? (int)aq_val_->value() : -1;
    static const char* metas[] = {"none", "all", "exif", "icc", "xmp"};
    o.metadata = metas[meta_choice_->value()];
    o.extra = extra_->value();
    return o;
}

void MainWindow::set_running_ui(bool running) {
    Fl_Widget* opts[] = {toolbar_, quality_, adv_group_, out_same_, out_custom_,
                         out_dir_, out_browse_, start_btn_};
    for (auto* w : opts) running ? w->deactivate() : w->activate();
    running ? cancel_btn_->activate() : cancel_btn_->deactivate();
    if (!running) apply_z_state();
}

void MainWindow::start_conversion() {
    if (converter_.running() || cwebp_path_.empty()) return;
    std::vector<Converter::Job> jobs;
    for (size_t i = 0; i < items_.size(); ++i) {
        if (!items_[i].checked) continue;
        items_[i].status = JobStatus::Pending;
        items_[i].error_text.clear();
        refresh_leaf((int)i);
        jobs.push_back({(int)i, items_[i].path_utf8});
    }
    if (jobs.empty()) {
        fl_alert("清單中沒有勾選的檔案");
        return;
    }
    tree_->redraw();
    OutputSettings out{out_custom_->value() != 0, out_dir_utf8_};
    if (out.custom_dir) {
        if (out.dir_utf8.empty()) {
            fl_alert("請先選擇輸出資料夾");
            return;
        }
        std::error_code ec;
        std::filesystem::create_directories(utf8_to_path(out.dir_utf8), ec);
        if (ec) {
            fl_alert("無法建立輸出資料夾:\n%s", out.dir_utf8.c_str());
            return;
        }
    }
    if (resize_chk_->value() && std::atoi(rw_->value()) == 0 && std::atoi(rh_->value()) == 0) {
        fl_alert("-resize 的寬與高不可同時為 0");
        return;
    }
    progress_->maximum((float)jobs.size());
    progress_->value(0);
    progress_->copy_label(("0/" + std::to_string(jobs.size())).c_str());
    set_running_ui(true);
    converter_.start(std::move(jobs), read_options(), out, cwebp_path_, &MainWindow::awake_cb, this);
}

void MainWindow::browse_output_dir() {
    Fl_Native_File_Chooser fc(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
    fc.title("選擇輸出資料夾");
    if (fc.show() != 0) return;
    out_dir_utf8_ = fc.filename();
    out_dir_->value(out_dir_utf8_.c_str());
    out_custom_->setonly();
}

void MainWindow::awake_cb(void* p) {
    std::unique_ptr<Converter::Msg> m((Converter::Msg*)p);
    ((MainWindow*)m->user)->on_msg(*m);
}

void MainWindow::on_msg(const Converter::Msg& m) {
    if (m.index >= 0 && m.index < (int)items_.size()) {
        FileItem& it = items_[m.index];
        it.status = m.status;
        it.error_text = m.error;
        refresh_leaf(m.index);
        if (m.status == JobStatus::Running && leaf_[m.index] &&
            !tree_->displayed(leaf_[m.index]))
            tree_->show_item_middle(leaf_[m.index]);
    }
    progress_->value((float)m.processed);
    progress_->copy_label((std::to_string(m.processed) + "/" + std::to_string(m.total)).c_str());
    char buf[64];
    snprintf(buf, sizeof(buf), "成功 %d 失敗 %d", m.ok, m.failed);
    status_box_->labelcolor(m.failed ? FL_RED : FL_FOREGROUND_COLOR);
    status_box_->copy_label(buf);
    redraw();
    if (m.all_done) {
        converter_.join();
        set_running_ui(false);
    }
}

void MainWindow::on_close() {
    if (converter_.running()) {
        converter_.cancel();
        converter_.join();
    }
    hide();
}
