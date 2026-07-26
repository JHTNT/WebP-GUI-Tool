#include "ui_main_window.h"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/fl_ask.H>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cwctype>
#include <filesystem>
#include <memory>
#include <sstream>

#include "scanner.h"

namespace {
constexpr int W = 640;
constexpr int ADV_H = 164;
constexpr int DH = ADV_H + 8;          // height delta when advanced panel toggles
constexpr int BOTTOM_Y0 = 362;         // bottom group y when collapsed
constexpr int H0 = BOTTOM_Y0 + 72 + 10;

std::wstring lc_key(const std::string& utf8) {
    std::wstring w = utf8_to_wide(utf8);
    std::transform(w.begin(), w.end(), w.begin(),
                   [](wchar_t c) { return (wchar_t)std::towlower(c); });
    return w;
}
}  // namespace

MainWindow::MainWindow() : Fl_Double_Window(W, H0, "WebP 批量轉換") {
    toolbar_ = new Fl_Group(0, 0, W, 44);
    auto* add_files = new Fl_Button(10, 8, 90, 28, "加入檔案");
    auto* add_dir = new Fl_Button(106, 8, 100, 28, "加入資料夾");
    auto* remove = new Fl_Button(212, 8, 90, 28, "移除勾選");
    auto* clear = new Fl_Button(308, 8, 64, 28, "清空");
    recursive_ = new Fl_Check_Button(498, 8, 132, 28, "含子資料夾");
    recursive_->value(1);
    toolbar_->end();

    browser_ = new Fl_Browser(10, 48, W - 20, 240);
    static const int widths[] = {24, 92, 0};
    browser_->column_widths(widths);
    browser_->column_char('\t');
    browser_->type(FL_HOLD_BROWSER);
    browser_->when(FL_WHEN_RELEASE_ALWAYS);
    browser_->tooltip("點最左欄勾選/取消;雙擊失敗項目看錯誤訊息;可直接拖入檔案或資料夾");

    quality_ = new Fl_Value_Slider(80, 296, W - 90, 24, "品質");
    quality_->type(FL_HOR_SLIDER);
    quality_->align(FL_ALIGN_LEFT);
    quality_->bounds(0, 100);
    quality_->step(1);
    quality_->value(90);

    adv_toggle_ = new Fl_Toggle_Button(10, 328, 110, 26, "詳細參數 ▸");

    adv_group_ = new Fl_Group(10, 362, W - 20, ADV_H);
    adv_group_->box(FL_ENGRAVED_FRAME);
    lossless_ = new Fl_Check_Button(24, 370, 100, 24, "-lossless");
    nl_chk_ = new Fl_Check_Button(134, 370, 132, 24, "-near_lossless");
    nl_val_ = new Fl_Spinner(268, 370, 64, 24);
    nl_val_->range(0, 100);
    nl_val_->value(60);
    z_choice_ = new Fl_Choice(400, 370, 80, 24, "-z");
    z_choice_->add("停用|0|1|2|3|4|5|6|7|8|9");
    z_choice_->value(0);
    m_choice_ = new Fl_Choice(58, 402, 64, 24, "-m");
    m_choice_->add("0|1|2|3|4|5|6");
    m_choice_->value(4);
    preset_choice_ = new Fl_Choice(200, 402, 110, 24, "-preset");
    preset_choice_->add("無|photo|picture|drawing|icon|text");
    preset_choice_->value(0);
    meta_choice_ = new Fl_Choice(420, 402, 100, 24, "-metadata");
    meta_choice_->add("none|all|exif|icc|xmp");
    meta_choice_->value(0);
    resize_chk_ = new Fl_Check_Button(24, 434, 84, 24, "-resize");
    rw_ = new Fl_Int_Input(140, 434, 72, 24, "寬");
    rw_->value("0");
    rh_ = new Fl_Int_Input(250, 434, 72, 24, "高");
    rh_->value("0");
    auto* hint = new Fl_Box(330, 434, 160, 24, "(0 = 等比例)");
    hint->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    hint->labelsize(12);
    hint->labelcolor(FL_DARK2);
    mt_ = new Fl_Check_Button(24, 466, 64, 24, "-mt");
    mt_->value(1);
    sharp_ = new Fl_Check_Button(98, 466, 110, 24, "-sharp_yuv");
    aq_chk_ = new Fl_Check_Button(218, 466, 96, 24, "-alpha_q");
    aq_val_ = new Fl_Spinner(316, 466, 64, 24);
    aq_val_->range(0, 100);
    aq_val_->value(100);
    extra_ = new Fl_Input(110, 498, W - 144, 24, "額外參數");
    extra_->tooltip("以空白切分後直接附加給 cwebp;請勿放含空白的路徑");
    adv_group_->end();
    adv_group_->hide();

    bottom_group_ = new Fl_Group(0, BOTTOM_Y0, W, 72);
    out_same_ = new Fl_Round_Button(10, BOTTOM_Y0 + 4, 132, 24, "來源同資料夾");
    out_same_->type(FL_RADIO_BUTTON);
    out_same_->value(1);
    out_custom_ = new Fl_Round_Button(150, BOTTOM_Y0 + 4, 104, 24, "自訂資料夾");
    out_custom_->type(FL_RADIO_BUTTON);
    out_dir_ = new Fl_Output(258, BOTTOM_Y0 + 4, 288, 24);
    out_browse_ = new Fl_Button(552, BOTTOM_Y0 + 4, 78, 24, "瀏覽…");
    start_btn_ = new Fl_Button(10, BOTTOM_Y0 + 36, 104, 30, "開始轉換");
    cancel_btn_ = new Fl_Button(122, BOTTOM_Y0 + 36, 72, 30, "取消");
    cancel_btn_->deactivate();
    progress_ = new Fl_Progress(204, BOTTOM_Y0 + 36, 190, 30);
    progress_->minimum(0);
    progress_->maximum(1);
    progress_->selection_color(FL_DARK_BLUE);
    status_box_ = new Fl_Box(402, BOTTOM_Y0 + 36, 228, 30);
    status_box_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    bottom_group_->end();

    end();
    resizable(browser_);
    size_range(W, 420, W, 0);   // fixed width, vertical resize only

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
        std::vector<FileItem> keep;
        for (auto& it : w->items_) {
            if (it.checked) w->keys_.erase(lc_key(it.path_utf8));
            else keep.push_back(std::move(it));
        }
        w->items_ = std::move(keep);
        w->rebuild_browser();
    }, this);
    clear->callback([](Fl_Widget*, void* d) {
        auto* w = (MainWindow*)d;
        if (w->converter_.running()) return;
        w->items_.clear();
        w->keys_.clear();
        w->rebuild_browser();
    }, this);
    browser_->callback([](Fl_Widget*, void* d) { ((MainWindow*)d)->on_browser_click(); }, this);
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

std::string MainWindow::line_for(const FileItem& it) const {
    const char* st = "待轉";
    char col[16] = "";
    switch (it.status) {
        case JobStatus::Pending: break;
        case JobStatus::Running: snprintf(col, sizeof(col), "@C%d", (int)FL_BLUE); st = "轉換中"; break;
        case JobStatus::Done: snprintf(col, sizeof(col), "@C%d", (int)FL_DARK_GREEN); st = "✔ 完成"; break;
        case JobStatus::Failed: snprintf(col, sizeof(col), "@C%d", (int)FL_RED); st = "✘ 失敗"; break;
        case JobStatus::Cancelled: snprintf(col, sizeof(col), "@C%d", (int)FL_DARK2); st = "已取消"; break;
    }
    std::string path;
    path.reserve(it.path_utf8.size());
    for (char c : it.path_utf8) {   // escape browser format char
        path += c;
        if (c == '@') path += '@';
    }
    return std::string(it.checked ? "☑" : "☐") + "\t" + col + st + "\t" + path;
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
    for (const auto& f : collect_images(paths_utf8, recursive_->value() != 0)) {
        if (!keys_.insert(lc_key(f)).second) continue;
        FileItem it;
        it.path_utf8 = f;
        items_.push_back(std::move(it));
        browser_->add(line_for(items_.back()).c_str());
    }
    update_idle_status();
}

void MainWindow::rebuild_browser() {
    browser_->clear();
    for (const auto& it : items_) browser_->add(line_for(it).c_str());
    update_idle_status();
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

void MainWindow::on_browser_click() {
    int line = browser_->value();
    if (line <= 0 || line > (int)items_.size()) return;
    FileItem& it = items_[line - 1];
    if (Fl::event_clicks() > 0) {
        if (it.status == JobStatus::Failed && !it.error_text.empty())
            fl_alert("%s", it.error_text.c_str());
        return;
    }
    if (converter_.running()) return;
    if (Fl::event_x() <= browser_->x() + 24) {
        it.checked = !it.checked;
        browser_->text(line, line_for(it).c_str());
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
    resizable(browser_);
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
        browser_->text((int)i + 1, line_for(items_[i]).c_str());
        jobs.push_back({(int)i, items_[i].path_utf8});
    }
    if (jobs.empty()) {
        fl_alert("清單中沒有勾選的檔案");
        return;
    }
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
        int line = m.index + 1;
        browser_->text(line, line_for(it).c_str());
        if (m.status == JobStatus::Running && !browser_->displayed(line))
            browser_->middleline(line);
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
