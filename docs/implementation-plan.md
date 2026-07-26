# webp-gui-tool 實作計畫

## 目標

精簡的 Windows GUI 工具,包裝外部 `cwebp`(只從 PATH 尋找)進行批量 WebP 轉換:

- 品質滑桿(-q)為主要操作,詳細參數面板可展開調整
- 拖放檔案/資料夾建立轉換清單,可逐項勾選/取消
- 顯示轉換進度與各項狀態,可中途取消,失敗項可查看錯誤訊息

技術基底:FLTK 1.4 + CMake/Ninja/MinGW-w64(已驗證可編譯),C++17。

## 已確認的需求決策

| 項目 | 決策 |
|---|---|
| cwebp 來源 | 只靠 PATH,啟動時偵測,找不到顯示提示並停用開始鈕 |
| 輸出位置 | 預設來源同資料夾(同名 .webp),可切換自訂輸出資料夾 |
| 資料夾掃描 | UI 提供「含子資料夾」遞迴開關;收 PNG / JPEG / TIFF / WebP |
| 詳細參數 | 無損(-lossless / -near_lossless / -z)、-m、-preset、-resize、常用(-mt / -sharp_yuv / -alpha_q / -metadata),加一個自由文字欄附加額外參數 |

## 架構

模組間單向依賴:UI → converter / scanner → model,方便日後替換或擴充。

```
src/
├── main.cpp            進入點(Fl::lock + 主視窗 + Fl::run)
├── job_model.h         資料模型:FileItem(路徑/勾選/狀態/錯誤)、CwebpOptions、OutputSettings
├── scanner.h/.cpp      路徑展開:UTF-8↔UTF-16、副檔名過濾、(遞迴)掃描資料夾
├── cwebp_runner.h/.cpp cwebp 偵測、參數組裝、子程序執行(擷取 stderr)、背景轉換執行緒
└── ui_main_window.h/.cpp 主視窗:版面、拖放、檔案清單、進度更新
```

## UI 概要

- 上:工具列(加入檔案/資料夾、移除勾選、清空、含子資料夾開關)
- 中:檔案清單(勾選框 + 檔名 + 狀態欄,自訂 Fl_Browser 子類別;雙擊失敗項看錯誤)
- 下:品質滑桿 → 可展開的詳細參數面板 → 輸出位置選擇 → 開始/取消 + 進度列

## 關鍵技術要點

- **拖放**:MainWindow 處理 FL_DND_* / FL_PASTE;Windows 上 `Fl::event_text()` 為 UTF-8 原生路徑、多檔換行分隔。
- **子程序**:`CreateProcessW` + `CREATE_NO_WINDOW`(不彈黑窗),pipe 擷取 stderr,exit code 判定成敗;取消時 `TerminateProcess`。路徑一律 UTF-8 → UTF-16 轉換後傳遞。
- **執行緒**:單一 worker thread 逐檔轉換(cwebp 自身可 -mt),`Fl::awake` 回主執行緒更新 UI;atomic flag 實作取消。
- **參數組裝**:`-preset` 最前(cwebp 規定);`-z` 隱含無損且與 -q/-m 互斥(UI 同步停用);其餘衝突不特別驗證,交給 cwebp 以 stderr 回報。額外參數欄以空白切分附加。
- **輸出路徑**:防自我覆寫(來源已是 .webp 且輸出同資料夾時,檔名加後綴)。

## 實作階段

1. **骨架**:改寫 CMakeLists(webp_gui 目標、WIN32 subsystem)、建 src/ 結構、靜態版面、cwebp 偵測提示
2. **清單與拖放**:scanner、檔案清單 widget、DnD、勾選、去重、遞迴開關
3. **轉換核心**:參數組裝、子程序執行與 stderr 擷取(先同步轉單檔驗證)
4. **批量與進度**:背景執行緒、進度/狀態即時更新、取消、關窗收尾
5. **收尾**:自訂輸出資料夾、防覆寫、刪除 demo/、git init + .gitignore(排除 build/)

## 驗證

用 PATH 中的 cwebp 實測:拖入含中文檔名與子資料夾的測試資料 → 取消勾選部分項目 → 調整品質與詳細參數 → 轉換 → 確認輸出正確、進度即時、可中途取消、失敗項可看 stderr。

## 未來擴充方向(不在本次範圍)

- 動圖支援(gif2webp)、dwebp 反向轉換
- 轉換前後檔案大小/預覽比較
- 設定持久化(記住上次參數)
- 並行轉換數量選項
